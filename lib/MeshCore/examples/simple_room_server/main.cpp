#include <Arduino.h>   // needed for PlatformIO
#include <Mesh.h>

#if defined(NRF52_PLATFORM)
  #include <InternalFileSystem.h>
#elif defined(ESP32)
  #include <SPIFFS.h>
#endif

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/StaticPoolPacketManager.h>
#include <helpers/SimpleMeshTables.h>
#include <helpers/IdentityStore.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/AdvertDataHelpers.h>
#include <helpers/TxtDataHelpers.h>
#include <helpers/CommonCLI.h>
#include <RTClib.h>

/* ------------------------------ Config -------------------------------- */

#ifndef FIRMWARE_BUILD_DATE
  #define FIRMWARE_BUILD_DATE   "9 Mar 2025"
#endif

#ifndef FIRMWARE_VERSION
  #define FIRMWARE_VERSION   "v1.2.2"
#endif

#ifndef LORA_FREQ
  #define LORA_FREQ   915.0
#endif
#ifndef LORA_BW
  #define LORA_BW     250
#endif
#ifndef LORA_SF
  #define LORA_SF     10
#endif
#ifndef LORA_CR
  #define LORA_CR      5
#endif
#ifndef LORA_TX_POWER
  #define LORA_TX_POWER  20
#endif

#ifndef ADVERT_NAME
  #define  ADVERT_NAME   "Test BBS"
#endif
#ifndef ADVERT_LAT
  #define  ADVERT_LAT  0.0
#endif
#ifndef ADVERT_LON
  #define  ADVERT_LON  0.0
#endif

#ifndef ADMIN_PASSWORD
  #define  ADMIN_PASSWORD  "password"
#endif

#ifndef MAX_CLIENTS
 #define MAX_CLIENTS           32
#endif

#ifndef MAX_UNSYNCED_POSTS
  #define MAX_UNSYNCED_POSTS    16
#endif

#if defined(HELTEC_LORA_V3)
  #include <helpers/HeltecV3Board.h>
  #include <helpers/CustomSX1262Wrapper.h>
  static HeltecV3Board board;
#elif defined(ARDUINO_XIAO_ESP32C3)
  #include <helpers/XiaoC3Board.h>
  #include <helpers/CustomSX1262Wrapper.h>
  #include <helpers/CustomSX1268Wrapper.h>
  static XiaoC3Board board;
#elif defined(SEEED_XIAO_S3)
  #include <helpers/ESP32Board.h>
  #include <helpers/CustomSX1262Wrapper.h>
  static ESP32Board board;
#elif defined(LILYGO_TLORA)
  #include <helpers/LilyGoTLoraBoard.h>
  #include <helpers/CustomSX1276Wrapper.h>
  static LilyGoTLoraBoard board;
#elif defined(STATION_G2)
  #include <helpers/StationG2Board.h>
  #include <helpers/CustomSX1262Wrapper.h>
  static StationG2Board board;
#elif defined(RAK_4631)
  #include <helpers/nrf52/RAK4631Board.h>
  #include <helpers/CustomSX1262Wrapper.h>
  static RAK4631Board board;
#elif defined(HELTEC_T114)
  #include <helpers/nrf52/T114Board.h>
  #include <helpers/CustomSX1262Wrapper.h>
  static T114Board board;
#elif defined(LILYGO_TECHO)
  #include <helpers/nrf52/TechoBoard.h>
  #include <helpers/CustomSX1262Wrapper.h>
  static TechoBoard board;
#else
  #error "need to provide a 'board' object"
#endif

#ifdef DISPLAY_CLASS
  #include <helpers/ui/SSD1306Display.h>

  static DISPLAY_CLASS  display;

  #include "UITask.h"
  static UITask ui_task(display);
#endif

/* ------------------------------ Code -------------------------------- */

struct ClientInfo {
  mesh::Identity id;
  uint32_t last_timestamp;  // by THEIR clock
  uint32_t last_activity;   // by OUR clock
  uint32_t sync_since;  // sync messages SINCE this timestamp (by OUR clock)
  uint32_t pending_ack;
  uint32_t push_post_timestamp;
  unsigned long ack_timeout;
  bool     is_admin;
  uint8_t  push_failures;
  uint8_t  secret[PUB_KEY_SIZE];
  int      out_path_len;
  uint8_t  out_path[MAX_PATH_SIZE];
};

#define MAX_POST_TEXT_LEN    (160-9)

struct PostInfo {
  mesh::Identity author;
  uint32_t post_timestamp;   // by OUR clock
  char text[MAX_POST_TEXT_LEN+1];
};

#define REPLY_DELAY_MILLIS         1500
#define PUSH_NOTIFY_DELAY_MILLIS   2000
#define SYNC_PUSH_INTERVAL         2000

#define PUSH_ACK_TIMEOUT_FLOOD    12000
#define PUSH_TIMEOUT_BASE          4000
#define PUSH_ACK_TIMEOUT_FACTOR    2000

#define CLIENT_KEEP_ALIVE_SECS   128

#define REQ_TYPE_GET_STATUS      0x01   // same as _GET_STATS
#define REQ_TYPE_KEEP_ALIVE      0x02

#define RESP_SERVER_LOGIN_OK      0   // response to ANON_REQ

class MyMesh : public mesh::Mesh, public CommonCLICallbacks {
  RadioLibWrapper* my_radio;
  FILESYSTEM* _fs;
  RADIO_CLASS* _phy;
  mesh::MainBoard* _board;
  unsigned long next_local_advert;
  NodePrefs _prefs;
  CommonCLI _cli;
  uint8_t reply_data[MAX_PACKET_PAYLOAD];
  int num_clients;
  ClientInfo known_clients[MAX_CLIENTS];
  unsigned long next_push;
  int next_client_idx;  // for round-robin polling
  int next_post_idx;
  PostInfo posts[MAX_UNSYNCED_POSTS];   // cyclic queue

  ClientInfo* putClient(const mesh::Identity& id) {
    for (int i = 0; i < num_clients; i++) {
      if (id.matches(known_clients[i].id)) return &known_clients[i];  // already known
    }
    ClientInfo* newClient;
    if (num_clients < MAX_CLIENTS) {
      newClient = &known_clients[num_clients++];
    } else {    // table is currently full
      // evict least active client
      uint32_t oldest_timestamp = 0xFFFFFFFF;
      newClient = &known_clients[0];
      for (int i = 0; i < num_clients; i++) {
        auto c = &known_clients[i];
        if (c->last_activity < oldest_timestamp) {
          oldest_timestamp = c->last_activity;
          newClient = c;
        }
      }
    }
    newClient->id = id;
    newClient->out_path_len = -1;  // initially out_path is unknown
    newClient->last_timestamp = 0;
    self_id.calcSharedSecret(newClient->secret, id);   // calc ECDH shared secret
    return newClient;
  }

  void  evict(ClientInfo* client) {
    client->last_activity = 0;  // this slot will now be re-used (will be oldest)
    memset(client->id.pub_key, 0, sizeof(client->id.pub_key));
    memset(client->secret, 0, sizeof(client->secret));
    client->pending_ack = 0;
  }

  void addPost(ClientInfo* client, const char* postData) {
    // TODO: suggested postData format: <title>/<descrption>
    posts[next_post_idx].author = client->id;    // add to cyclic queue
    StrHelper::strncpy(posts[next_post_idx].text, postData, MAX_POST_TEXT_LEN);

    posts[next_post_idx].post_timestamp = getRTCClock()->getCurrentTimeUnique();
    next_post_idx = (next_post_idx + 1) % MAX_UNSYNCED_POSTS;

    next_push = futureMillis(PUSH_NOTIFY_DELAY_MILLIS);
  }

  void pushPostToClient(ClientInfo* client, PostInfo& post) {
    int len = 0;
    memcpy(&reply_data[len], &post.post_timestamp, 4); len += 4;   // this is a PAST timestamp... but should be accepted by client
    reply_data[len++] = (TXT_TYPE_SIGNED_PLAIN << 2);  // 'signed' plain text

    // encode prefix of post.author.pub_key
    memcpy(&reply_data[len], post.author.pub_key, 4); len += 4;   // just first 4 bytes

    int text_len = strlen(post.text);
    memcpy(&reply_data[len], post.text, text_len); len += text_len;

    // calc expected ACK reply
    mesh::Utils::sha256((uint8_t *)&client->pending_ack, 4, reply_data, len, client->id.pub_key, PUB_KEY_SIZE);
    client->push_post_timestamp = post.post_timestamp;

    auto reply = createDatagram(PAYLOAD_TYPE_TXT_MSG, client->id, client->secret, reply_data, len);
    if (reply) {
      if (client->out_path_len < 0) {
        sendFlood(reply);
        client->ack_timeout = futureMillis(PUSH_ACK_TIMEOUT_FLOOD);
      } else {
        sendDirect(reply, client->out_path, client->out_path_len);
        client->ack_timeout = futureMillis(PUSH_TIMEOUT_BASE + PUSH_ACK_TIMEOUT_FACTOR * (client->out_path_len + 1));
      }
    } else {
      client->pending_ack = 0;
      MESH_DEBUG_PRINTLN("Unable to push post to client");
    }
  }

  bool processAck(const uint8_t *data) {
    for (int i = 0; i < num_clients; i++) {
      auto client = &known_clients[i];
      if (client->pending_ack && memcmp(data, &client->pending_ack, 4) == 0) {     // got an ACK from Client!
        client->pending_ack = 0;    // clear this, so next push can happen
        client->push_failures = 0;
        client->sync_since = client->push_post_timestamp;   // advance Client's SINCE timestamp, to sync next post
        return true;
      }
    }
    return false;
  }

  mesh::Packet* createSelfAdvert() {
    uint8_t app_data[MAX_ADVERT_DATA_SIZE];
    uint8_t app_data_len;
    {
      AdvertDataBuilder builder(ADV_TYPE_ROOM, _prefs.node_name, _prefs.node_lat, _prefs.node_lon);
      app_data_len = builder.encodeTo(app_data);
    }

   return createAdvert(self_id, app_data, app_data_len);
  }

protected:
  float getAirtimeBudgetFactor() const override {
    return _prefs.airtime_factor;
  }

  void logRxRaw(float snr, float rssi, const uint8_t raw[], int len) override {
    #if MESH_PACKET_LOGGING
      Serial.print(getLogDateTime());
      Serial.print(" RAW: ");
      mesh::Utils::printHex(Serial, raw, len);
      Serial.println();
    #endif
  }

  int calcRxDelay(float score, uint32_t air_time) const override {
    if (_prefs.rx_delay_base <= 0.0f) return 0;
    return (int) ((pow(_prefs.rx_delay_base, 0.85f - score) - 1.0) * air_time);
  }

  const char* getLogDateTime() override {
    static char tmp[32];
    uint32_t now = getRTCClock()->getCurrentTime();
    DateTime dt = DateTime(now);
    sprintf(tmp, "%02d:%02d:%02d - %d/%d/%d U", dt.hour(), dt.minute(), dt.second(), dt.day(), dt.month(), dt.year());
    return tmp;
  }

  uint32_t getRetransmitDelay(const mesh::Packet* packet) override {
    uint32_t t = (_radio->getEstAirtimeFor(packet->path_len + packet->payload_len + 2) * _prefs.tx_delay_factor);
    return getRNG()->nextInt(0, 6)*t;
  }
  uint32_t getDirectRetransmitDelay(const mesh::Packet* packet) override {
    uint32_t t = (_radio->getEstAirtimeFor(packet->path_len + packet->payload_len + 2) * _prefs.direct_tx_delay_factor);
    return getRNG()->nextInt(0, 6)*t;
  }

  bool allowPacketForward(const mesh::Packet* packet) override {
    return !_prefs.disable_fwd;
  }

  void onAnonDataRecv(mesh::Packet* packet, uint8_t type, const mesh::Identity& sender, uint8_t* data, size_t len) override {
    if (type == PAYLOAD_TYPE_ANON_REQ) {  // received an initial request by a possible admin client (unknown at this stage)
      uint32_t sender_timestamp, sender_sync_since;
      memcpy(&sender_timestamp, data, 4);
      memcpy(&sender_sync_since, &data[4], 4);  // sender's "sync messags SINCE x" timestamp

      bool is_admin;
      data[len] = 0;  // ensure null terminator
      if (strcmp((char *) &data[8], _prefs.password) == 0) {  // check for valid admin password
        is_admin = true;
      } else {
        is_admin = false;
        if (strcmp((char *) &data[8], _prefs.guest_password) != 0) {  // check the room/public password
          MESH_DEBUG_PRINTLN("Incorrect room password");
          return;   // no response. Client will timeout
        }
      }

      auto client = putClient(sender);  // add to known clients (if not already known)
      if (sender_timestamp <= client->last_timestamp) {
        MESH_DEBUG_PRINTLN("possible replay attack!");
        return;
      }

      MESH_DEBUG_PRINTLN("Login success!");
      client->is_admin = is_admin;
      client->last_timestamp = sender_timestamp;
      client->sync_since = sender_sync_since;
      client->pending_ack = 0;
      client->push_failures = 0;

      uint32_t now = getRTCClock()->getCurrentTime();
      client->last_activity = now;

      now = getRTCClock()->getCurrentTimeUnique();
      memcpy(reply_data, &now, 4);   // response packets always prefixed with timestamp
      // TODO: maybe reply with count of messages waiting to be synced for THIS client?
      reply_data[4] = RESP_SERVER_LOGIN_OK;
      reply_data[5] = (CLIENT_KEEP_ALIVE_SECS >> 4);  // NEW: recommended keep-alive interval (secs / 16)
      reply_data[6] = is_admin ? 1 : 0;
      reply_data[7] = 0;  // FUTURE: reserved
      memcpy(&reply_data[8], "OK", 2);  // REVISIT: not really needed

      next_push = futureMillis(PUSH_NOTIFY_DELAY_MILLIS);  // delay next push, give RESPONSE packet time to arrive first

      if (packet->isRouteFlood()) {
        // let this sender know path TO here, so they can use sendDirect(), and ALSO encode the response
        mesh::Packet* path = createPathReturn(sender, client->secret, packet->path, packet->path_len,
                                              PAYLOAD_TYPE_RESPONSE, reply_data, 8 + 2);
        if (path) sendFlood(path);
      } else {
        mesh::Packet* reply = createDatagram(PAYLOAD_TYPE_RESPONSE, sender, client->secret, reply_data, 8 + 2);
        if (reply) {
          if (client->out_path_len >= 0) {  // we have an out_path, so send DIRECT
            sendDirect(reply, client->out_path, client->out_path_len);
          } else {
            sendFlood(reply);
          }
        }
      }
    }
  }

  int  matching_peer_indexes[MAX_CLIENTS];

  int searchPeersByHash(const uint8_t* hash) override {
    int n = 0;
    for (int i = 0; i < num_clients; i++) {
      if (known_clients[i].id.isHashMatch(hash)) {
        matching_peer_indexes[n++] = i;  // store the INDEXES of matching contacts (for subsequent 'peer' methods)
      }
    }
    return n;
  }

  void getPeerSharedSecret(uint8_t* dest_secret, int peer_idx) override {
    int i = matching_peer_indexes[peer_idx];
    if (i >= 0 && i < num_clients) {
      // lookup pre-calculated shared_secret
      memcpy(dest_secret, known_clients[i].secret, PUB_KEY_SIZE);
    } else {
      MESH_DEBUG_PRINTLN("getPeerSharedSecret: Invalid peer idx: %d", i);
    }
  }

  void onPeerDataRecv(mesh::Packet* packet, uint8_t type, int sender_idx, const uint8_t* secret, uint8_t* data, size_t len) override {
    int i = matching_peer_indexes[sender_idx];
    if (i < 0 || i >= num_clients) {  // get from our known_clients table (sender SHOULD already be known in this context)
      MESH_DEBUG_PRINTLN("onPeerDataRecv: invalid peer idx: %d", i);
      return;
    }
    auto client = &known_clients[i];
    if (type == PAYLOAD_TYPE_TXT_MSG && len > 5) {   // a CLI command or new Post
      uint32_t sender_timestamp;
      memcpy(&sender_timestamp, data, 4);  // timestamp (by sender's RTC clock - which could be wrong)
      uint flags = (data[4] >> 2);   // message attempt number, and other flags

      if (!(flags == TXT_TYPE_PLAIN || flags == TXT_TYPE_CLI_DATA)) {
        MESH_DEBUG_PRINTLN("onPeerDataRecv: unsupported command flags received: flags=%02x", (uint32_t)flags);
      } else if (sender_timestamp >= client->last_timestamp) {  // prevent replay attacks, but send Acks for retries
        bool is_retry = (sender_timestamp == client->last_timestamp);
        client->last_timestamp = sender_timestamp;

        uint32_t now = getRTCClock()->getCurrentTimeUnique();
        client->last_activity = now;
        client->push_failures = 0;  // reset so push can resume (if prev failed)

        // len can be > original length, but 'text' will be padded with zeroes
        data[len] = 0; // need to make a C string again, with null terminator

        uint32_t ack_hash;    // calc truncated hash of the message timestamp + text + sender pub_key, to prove to sender that we got it
        mesh::Utils::sha256((uint8_t *) &ack_hash, 4, data, 5 + strlen((char *)&data[5]), client->id.pub_key, PUB_KEY_SIZE);

        uint8_t temp[166];
        bool send_ack;
        if (flags == TXT_TYPE_CLI_DATA) {
          if (client->is_admin) {
            if (is_retry) {
              temp[5] = 0;  // no reply
            } else {
              _cli.handleCommand(sender_timestamp, (const char *) &data[5], (char *) &temp[5]);
              temp[4] = (TXT_TYPE_CLI_DATA << 2);  // attempt and flags,  (NOTE: legacy was: TXT_TYPE_PLAIN)
            }
            send_ack = false;
          } else {
            temp[5] = 0;  // no reply
            send_ack = false;  // and no ACK...  user shoudn't be sending these
          }
        } else {   // TXT_TYPE_PLAIN
          if (!is_retry) {
            addPost(client, (const char *) &data[5]);
          }
          temp[5] = 0;  // no reply (ACK is enough)
          send_ack = true;
        }

        uint32_t delay_millis;
        if (send_ack) {
          mesh::Packet* ack = createAck(ack_hash);
          if (ack) {
            if (client->out_path_len < 0) {
              sendFlood(ack);
            } else {
              sendDirect(ack, client->out_path, client->out_path_len);
            }
          }
          delay_millis = REPLY_DELAY_MILLIS;
        } else {
          delay_millis = 0;
        }

        int text_len = strlen((char *) &temp[5]);
        if (text_len > 0) {
          if (now == sender_timestamp) {
            // WORKAROUND: the two timestamps need to be different, in the CLI view
            now++;
          }
          memcpy(temp, &now, 4);   // mostly an extra blob to help make packet_hash unique

          // calc expected ACK reply
          //mesh::Utils::sha256((uint8_t *)&expected_ack_crc, 4, temp, 5 + text_len, self_id.pub_key, PUB_KEY_SIZE);

          auto reply = createDatagram(PAYLOAD_TYPE_TXT_MSG, client->id, secret, temp, 5 + text_len);
          if (reply) {
            if (client->out_path_len < 0) {
              sendFlood(reply, delay_millis);
            } else {
              sendDirect(reply, client->out_path, client->out_path_len, delay_millis);
            }
          }
        }
      } else {
        MESH_DEBUG_PRINTLN("onPeerDataRecv: possible replay attack detected");
      }
    } else if (type == PAYLOAD_TYPE_REQ && len >= 5) {
      uint32_t sender_timestamp;
      memcpy(&sender_timestamp, data, 4);  // timestamp (by sender's RTC clock - which could be wrong)
      if (data[4] == REQ_TYPE_KEEP_ALIVE && packet->isRouteDirect()) {   // request type
        uint32_t forceSince = 0;
        if (len >= 9) {   // optional - last post_timestamp client received
          memcpy(&forceSince, &data[5], 4);    // NOTE: this may be 0, if part of decrypted PADDING!
        } else {
          memcpy(&data[5], &forceSince, 4);  // make sure there are zeroes in payload (for ack_hash calc below)
        }
        if (forceSince > 0) {
          client->sync_since = forceSince;    // force-update the 'sync since'
        }

        uint32_t now = getRTCClock()->getCurrentTime();
        client->last_activity = now;   // <-- THIS will keep client connection alive
        client->push_failures = 0;  // reset so push can resume (if prev failed)
        client->pending_ack = 0;

        // TODO: Throttle KEEP_ALIVE requests!
        // if client sends too quickly, evict()

        // RULE: only send keep_alive response DIRECT!
        if (client->out_path_len >= 0) {
          uint32_t ack_hash;    // calc ACK to prove to sender that we got request
          mesh::Utils::sha256((uint8_t *) &ack_hash, 4, data, 9, client->id.pub_key, PUB_KEY_SIZE);

          auto reply = createAck(ack_hash);
          if (reply) {
            sendDirect(reply, client->out_path, client->out_path_len);
          }
        }
      }
    }
  }

  bool onPeerPathRecv(mesh::Packet* packet, int sender_idx, const uint8_t* secret, uint8_t* path, uint8_t path_len, uint8_t extra_type, uint8_t* extra, uint8_t extra_len) override {
    // TODO: prevent replay attacks
    int i = matching_peer_indexes[sender_idx];

    if (i >= 0 && i < num_clients) {  // get from our known_clients table (sender SHOULD already be known in this context)
      MESH_DEBUG_PRINTLN("PATH to client, path_len=%d", (uint32_t) path_len);
      auto client = &known_clients[i];
      memcpy(client->out_path, path, client->out_path_len = path_len);  // store a copy of path, for sendDirect()
    } else {
      MESH_DEBUG_PRINTLN("onPeerPathRecv: invalid peer idx: %d", i);
    }

    if (extra_type == PAYLOAD_TYPE_ACK && extra_len >= 4) {
      // also got an encoded ACK!
      processAck(extra);
    }

    // NOTE: no reciprocal path send!!
    return false;
  }

  void onAckRecv(mesh::Packet* packet, uint32_t ack_crc) override {
    if (processAck((uint8_t *)&ack_crc)) {
      packet->markDoNotRetransmit();   // ACK was for this node, so don't retransmit
    }
  }

public:
  MyMesh(RADIO_CLASS& phy, mesh::MainBoard& board, RadioLibWrapper& radio, mesh::MillisecondClock& ms, mesh::RNG& rng, mesh::RTCClock& rtc, mesh::MeshTables& tables)
     : mesh::Mesh(radio, ms, rng, rtc, *new StaticPoolPacketManager(32), tables),
        _phy(&phy), _board(&board), _cli(board, this, &_prefs, this)
  {
    my_radio = &radio;
    next_local_advert = 0;

    // defaults
    memset(&_prefs, 0, sizeof(_prefs));
    _prefs.airtime_factor = 1.0;    // one half
    _prefs.rx_delay_base = 0.0f;   // off by default, was 10.0
    _prefs.tx_delay_factor = 0.5f;   // was 0.25f;
    StrHelper::strncpy(_prefs.node_name, ADVERT_NAME, sizeof(_prefs.node_name));
    _prefs.node_lat = ADVERT_LAT;
    _prefs.node_lon = ADVERT_LON;
    StrHelper::strncpy(_prefs.password, ADMIN_PASSWORD, sizeof(_prefs.password));
    _prefs.freq = LORA_FREQ;
    _prefs.sf = LORA_SF;
    _prefs.bw = LORA_BW;
    _prefs.cr = LORA_CR;
    _prefs.tx_power_dbm = LORA_TX_POWER;
    _prefs.disable_fwd = 1;
    _prefs.advert_interval = 1;  // default to 2 minutes for NEW installs
  #ifdef ROOM_PASSWORD
    StrHelper::strncpy(_prefs.guest_password, ROOM_PASSWORD, sizeof(_prefs.guest_password));
  #endif

    num_clients = 0;
    next_post_idx = 0;
    next_client_idx = 0;
    next_push = 0;
    memset(posts, 0, sizeof(posts));
  }

  CommonCLI* getCLI() { return &_cli; }

  void begin(FILESYSTEM* fs) {
    mesh::Mesh::begin();
    _fs = fs;
    // load persisted prefs
    _cli.loadPrefs(_fs);

    _phy->setFrequency(_prefs.freq);
    _phy->setSpreadingFactor(_prefs.sf);
    _phy->setBandwidth(_prefs.bw);
    _phy->setCodingRate(_prefs.cr);
    _phy->setOutputPower(_prefs.tx_power_dbm);

    updateAdvertTimer();
  }

  const char* getFirmwareVer() override { return FIRMWARE_VERSION; }
  const char* getBuildDate() override { return FIRMWARE_BUILD_DATE; }
  const char* getNodeName() { return _prefs.node_name; }

  void savePrefs() override {
    _cli.savePrefs(_fs);
  }

  bool formatFileSystem() override {
    #if defined(NRF52_PLATFORM)
      return InternalFS.format();
    #elif defined(ESP32)
      return SPIFFS.format();
    #else
      #error "need to implement file system erase"
      return false;
    #endif
  }

  void sendSelfAdvertisement(int delay_millis) override {
    mesh::Packet* pkt = createSelfAdvert();
    if (pkt) {
      sendFlood(pkt, delay_millis);
    } else {
      MESH_DEBUG_PRINTLN("ERROR: unable to create advertisement packet!");
    }
  }

  void updateAdvertTimer() override {
    if (_prefs.advert_interval > 0) {  // schedule local advert timer
      next_local_advert = futureMillis((uint32_t)_prefs.advert_interval * 2 * 60 * 1000);
    } else {
      next_local_advert = 0;  // stop the timer
    }
  }

  void setLoggingOn(bool enable) override { /* no-op */ }
  void eraseLogFile() override { /* no-op */ }
  void dumpLogFile() override { /* no-op */ }

  void setTxPower(uint8_t power_dbm) override {
    _phy->setOutputPower(power_dbm);
  }

  void loop() {
    mesh::Mesh::loop();

    if (millisHasNowPassed(next_push) && num_clients > 0) {
      // check for ACK timeouts
      for (int i = 0; i < num_clients; i++) {
        auto c = &known_clients[i];
        if (c->pending_ack && millisHasNowPassed(c->ack_timeout)) {
          c->push_failures++;
          c->pending_ack = 0;   // reset  (TODO: keep prev expected_ack's in a list, incase they arrive LATER, after we retry)
          MESH_DEBUG_PRINTLN("pending ACK timed out: push_failures: %d", (uint32_t)c->push_failures);
        }
      }
      // check next Round-Robin client, and sync next new post
      auto client = &known_clients[next_client_idx];
      if (client->pending_ack == 0 && client->last_activity != 0 && client->push_failures < 3) {  // not already waiting for ACK, AND not evicted, AND retries not max
        MESH_DEBUG_PRINTLN("loop - checking for client %02X", (uint32_t) client->id.pub_key[0]);
        for (int k = 0, idx = next_post_idx; k < MAX_UNSYNCED_POSTS; k++) {
          if (posts[idx].post_timestamp > client->sync_since   // is new post for this Client?
            && !posts[idx].author.matches(client->id)) {    // don't push posts to the author
            // push this post to Client, then wait for ACK
            pushPostToClient(client, posts[idx]);
            MESH_DEBUG_PRINTLN("loop - pushed to client %02X: %s", (uint32_t) client->id.pub_key[0], posts[idx].text);
            break;
          }
          idx = (idx + 1) % MAX_UNSYNCED_POSTS;   // wrap to start of cyclic queue
        }
      } else {
        MESH_DEBUG_PRINTLN("loop - skipping busy (or evicted) client %02X", (uint32_t) client->id.pub_key[0]);
      }
      next_client_idx = (next_client_idx + 1) % num_clients;  // round robin polling for each client

      next_push = futureMillis(SYNC_PUSH_INTERVAL);
    }

    if (next_local_advert && millisHasNowPassed(next_local_advert)) {
      mesh::Packet* pkt = createSelfAdvert();
      if (pkt) {
        sendZeroHop(pkt);
      }

      updateAdvertTimer();   // schedule next local advert
    }

  #ifdef DISPLAY_CLASS
    ui_task.loop();
  #endif

    // TODO: periodically check for OLD/inactive entries in known_clients[], and evict
  }
};

#if defined(NRF52_PLATFORM)
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI);
#elif defined(LILYGO_TLORA)
SPIClass spi;
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_0, P_LORA_RESET, P_LORA_DIO_1, spi);
#elif defined(P_LORA_SCLK)
SPIClass spi;
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif
StdRNG fast_rng;
SimpleMeshTables tables;

#ifdef ESP32
ESP32RTCClock fallback_clock;
#else
VolatileRTCClock fallback_clock;
#endif
AutoDiscoverRTCClock rtc_clock(fallback_clock);

MyMesh the_mesh(radio, board, *new WRAPPER_CLASS(radio, board), *new ArduinoMillis(), fast_rng, rtc_clock, tables);

void halt() {
  while (1) ;
}

static char command[MAX_POST_TEXT_LEN+1];

void setup() {
  Serial.begin(115200);
  delay(1000);

  board.begin();
#ifdef ESP32
  fallback_clock.begin();
#endif
  rtc_clock.begin(Wire);

#ifdef SX126X_DIO3_TCXO_VOLTAGE
  float tcxo = SX126X_DIO3_TCXO_VOLTAGE;
#else
  float tcxo = 1.6f;
#endif

#if defined(NRF52_PLATFORM)
  SPI.setPins(P_LORA_MISO, P_LORA_SCLK, P_LORA_MOSI);
  SPI.begin();
#elif defined(P_LORA_SCLK)
  spi.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI);
#endif
  int status = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, LORA_TX_POWER, 8, tcxo);
  if (status != RADIOLIB_ERR_NONE) {
    delay(5000);
    Serial.print("ERROR: radio init failed: ");
    Serial.println(status);
    halt();
  }

  radio.setCRC(1);

#ifdef SX126X_CURRENT_LIMIT
  radio.setCurrentLimit(SX126X_CURRENT_LIMIT);
#endif
#ifdef SX126X_DIO2_AS_RF_SWITCH
  radio.setDio2AsRfSwitch(SX126X_DIO2_AS_RF_SWITCH);
#endif
#ifdef SX126X_RX_BOOSTED_GAIN
  radio.setRxBoostedGainMode(SX126X_RX_BOOSTED_GAIN);
#endif

  fast_rng.begin(radio.random(0x7FFFFFFF));

  FILESYSTEM* fs;
#if defined(NRF52_PLATFORM)
  InternalFS.begin();
  fs = &InternalFS;
  IdentityStore store(InternalFS, "");
#elif defined(ESP32)
  SPIFFS.begin(true);
  fs = &SPIFFS;
  IdentityStore store(SPIFFS, "/identity");
#else
  #error "need to define filesystem"
#endif
  if (!store.load("_main", the_mesh.self_id)) {
    RadioNoiseListener rng(radio);
    the_mesh.self_id = mesh::LocalIdentity(&rng);  // create new random identity
    store.save("_main", the_mesh.self_id);
  }

  Serial.print("Room ID: ");
  mesh::Utils::printHex(Serial, the_mesh.self_id.pub_key, PUB_KEY_SIZE); Serial.println();

  command[0] = 0;

  the_mesh.begin(fs);

#ifdef DISPLAY_CLASS
  display.begin();
  ui_task.begin(the_mesh.getNodeName(), FIRMWARE_BUILD_DATE);
#endif

  // send out initial Advertisement to the mesh
  the_mesh.sendSelfAdvertisement(2000);
}

void loop() {
  int len = strlen(command);
  while (Serial.available() && len < sizeof(command)-1) {
    char c = Serial.read();
    if (c != '\n') {
      command[len++] = c;
      command[len] = 0;
    }
    Serial.print(c);
  }
  if (len == sizeof(command)-1) {  // command buffer full
    command[sizeof(command)-1] = '\r';
  }

  if (len > 0 && command[len - 1] == '\r') {  // received complete line
    command[len - 1] = 0;  // replace newline with C string null terminator
    char reply[160];
    the_mesh.getCLI()->handleCommand(0, command, reply);  // NOTE: there is no sender_timestamp via serial!
    if (reply[0]) {
      Serial.print("  -> "); Serial.println(reply);
    }

    command[0] = 0;  // reset command buffer
  }

  the_mesh.loop();
}
