#include "punkmesh.h"
#include <LittleFS.h>


// Test identity to stop spamming my local area
#define PRV_KEY "E0A362A3E88BD763BBAD96F0335BB490557446932F73D9E5EA8BE8AA4D7F0C6B569C81C66E5C557233159D930C2EBEFC92AC71C3F5A9A64907F4B17DCE7AFABD"
#define PUB_KEY "7F97BF0E8FFC8E4C0FF089FB3C881875A926FAEAB91A790A66E3E1E37FB29321"

/* ---------------------------------- CONFIGURATION ------------------------------------- */

#define FIRMWARE_VER_TEXT "v2 (build: 4 Feb 2025)"

#ifndef LORA_FREQ
#define LORA_FREQ 915.0
#endif
#ifndef LORA_BW
#define LORA_BW 250
#endif
#ifndef LORA_SF
#define LORA_SF 10
#endif
#ifndef LORA_CR
#define LORA_CR 5
#endif
#ifndef LORA_TX_POWER
#define LORA_TX_POWER 20
#endif

#ifndef MAX_CONTACTS
#define MAX_CONTACTS 100
#endif

#include <helpers/BaseChatMesh.h>

#define SEND_TIMEOUT_BASE_MILLIS 500
#define FLOOD_SEND_TIMEOUT_FACTOR 16.0f
#define DIRECT_SEND_PERHOP_FACTOR 6.0f
#define DIRECT_SEND_PERHOP_EXTRA_MILLIS 250

#define PUBLIC_GROUP_PSK "izOH6cXN6mrJ5e26oRXNcg=="

// Punk<->Lua bridge

void lua_mesh_push_message(lua_State* L, const mesh::GroupChannel &channel, mesh::Packet *pkt, uint32_t timestamp, const char *text) {
    lua_getglobal(L, "require");
    lua_pushstring(L, "lib/mesh/messages");

    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        Serial.printf("require failed: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return;
    }

    // module on stack
    lua_getfield(L, -1, "__dispatch");  // [module, __dispatch]
    if (!lua_isfunction(L, -1)) {
        Serial.println("❌ __dispatch not a function!");
        lua_pop(L, 2); // module + bad value
        return;
    }

    const char* leaked_text = strdup(text);  // YOLO 🔥
    
    // lua_pushvalue(L, -2);               // push module as self
    lua_pushstring(L, leaked_text);     // arg1: text
    lua_pushinteger(L, timestamp);      // arg2: timestamp
    lua_pushboolean(L, pkt->isRouteDirect()); // arg3: direct
    lua_pushinteger(L, pkt->path_len);  // arg4: hops

    if (lua_pcall(L, 4, 0, 0) != LUA_OK) {
        Serial.printf("❌ __dispatch failed: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    lua_pop(L, 1); // pop module
}

void PunkMesh::store_message(const char* from, const char* text, uint32_t timestamp, uint8_t hops, bool direct) {
    int idx = (msg_head + msg_count) % MAX_MESSAGES;

    if (msg_count == MAX_MESSAGES) {
        // overwrite oldest
        idx = msg_head;
        msg_head = (msg_head + 1) % MAX_MESSAGES;
    } else {
        msg_count++;
    }

    strncpy(message_history[idx].from, from, sizeof(message_history[idx].from) - 1);
    strncpy(message_history[idx].text, text, sizeof(message_history[idx].text) - 1);
    message_history[idx].timestamp = timestamp;
    message_history[idx].hops = hops;
    message_history[idx].direct = direct;
}

// Meshcore...

// Believe it or not, this std C function is busted on some platforms!
static uint32_t _atoi(const char *sp)
{
    uint32_t n = 0;
    while (*sp && *sp >= '0' && *sp <= '9')
    {
        n *= 10;
        n += (*sp++ - '0');
    }
    return n;
}

const char *PunkMesh::getTypeName(uint8_t type) const
{
    if (type == ADV_TYPE_CHAT)
        return "Chat";
    if (type == ADV_TYPE_REPEATER)
        return "Repeater";
    if (type == ADV_TYPE_ROOM)
        return "Room";
    return "??"; // unknown
}

void PunkMesh::loadContacts()
{
    if (LittleFS.exists("/contacts"))
    {
#if defined(RP2040_PLATFORM)
        File file = LittleFS.open("/contacts", "r");
#else
        File file = LittleFS.open("/contacts");
#endif
        if (file)
        {
            bool full = false;
            while (!full)
            {
                ContactInfo c;
                uint8_t pub_key[32];
                uint8_t unused;
                uint32_t reserved;

                bool success = (file.read(pub_key, 32) == 32);
                success = success && (file.read((uint8_t *)&c.name, 32) == 32);
                success = success && (file.read(&c.type, 1) == 1);
                success = success && (file.read(&c.flags, 1) == 1);
                success = success && (file.read(&unused, 1) == 1);
                success = success && (file.read((uint8_t *)&reserved, 4) == 4);
                success = success && (file.read((uint8_t *)&c.out_path_len, 1) == 1);
                success = success && (file.read((uint8_t *)&c.last_advert_timestamp, 4) == 4);
                success = success && (file.read(c.out_path, 64) == 64);
                c.gps_lat = c.gps_lon = 0; // not yet supported

                if (!success)
                    break; // EOF

                c.id = mesh::Identity(pub_key);
                c.lastmod = 0;
                if (!addContact(c))
                    full = true;
            }
            file.close();
        }
    }
}

void PunkMesh::saveContacts()
{
#if defined(NRF52_PLATFORM)
    _fs->remove("/contacts");
    File file = _fs->open("/contacts", FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
    File file = LittleFS.open("/contacts", "w");
#else
    File file = LittleFS.open("/contacts", "w", true);
#endif
    if (file)
    {
        ContactsIterator iter;
        ContactInfo c;
        uint8_t unused = 0;
        uint32_t reserved = 0;

        while (iter.hasNext(this, c))
        {
            bool success = (file.write(c.id.pub_key, 32) == 32);
            success = success && (file.write((uint8_t *)&c.name, 32) == 32);
            success = success && (file.write(&c.type, 1) == 1);
            success = success && (file.write(&c.flags, 1) == 1);
            success = success && (file.write(&unused, 1) == 1);
            success = success && (file.write((uint8_t *)&reserved, 4) == 4);
            success = success && (file.write((uint8_t *)&c.out_path_len, 1) == 1);
            success = success && (file.write((uint8_t *)&c.last_advert_timestamp, 4) == 4);
            success = success && (file.write(c.out_path, 64) == 64);

            if (!success)
                break; // write failed
        }
        file.close();
    }
}

void PunkMesh::setClock(uint32_t timestamp)
{
    uint32_t curr = getRTCClock()->getCurrentTime();
    if (timestamp > curr)
    {
        getRTCClock()->setCurrentTime(timestamp);
        Serial.println("   (OK - clock set!)");
    }
    else
    {
        Serial.println("   (ERR: clock cannot go backwards)");
    }
}

void PunkMesh::importCard(const char *command)
{
    while (*command == ' ')
        command++; // skip leading spaces
    if (memcmp(command, "meshcore://", 11) == 0)
    {
        command += 11;                 // skip the prefix
        char *ep = strchr(command, 0); // find end of string
        while (ep > command)
        {
            ep--;
            if (mesh::Utils::isHexChar(*ep))
                break; // found tail end of card
            *ep = 0;   // remove trailing spaces and other junk
        }
        int len = strlen(command);
        if (len % 2 == 0)
        {
            len >>= 1; // halve, for num bytes
            if (mesh::Utils::fromHex(tmp_buf, len, command))
            {
                importContact(tmp_buf, len);
                return;
            }
        }
    }
    Serial.println("   error: invalid format");
}

float PunkMesh::getAirtimeBudgetFactor() const
{
    return _prefs.airtime_factor;
}

int PunkMesh::calcRxDelay(float score, uint32_t air_time) const
{
    return 0; // disable rxdelay
}

bool PunkMesh::allowPacketForward(const mesh::Packet *packet)
{
    return true;
}

void PunkMesh::onDiscoveredContact(ContactInfo &contact, bool is_new, uint8_t path_len, const uint8_t *path)
{
    // TODO: if not in favs,  prompt to add as fav(?)

    Serial.printf("ADVERT from -> %s\n", contact.name);
    Serial.printf("  type: %s\n", getTypeName(contact.type));
    Serial.print("   public key: ");
    mesh::Utils::printHex(Serial, contact.id.pub_key, PUB_KEY_SIZE);
    Serial.println();

    saveContacts();
}

void PunkMesh::onContactPathUpdated(const ContactInfo &contact)
{
    Serial.printf("PATH to: %s, path_len=%d\n", contact.name, (int32_t)contact.out_path_len);
    saveContacts();
}

bool PunkMesh::processAck(const uint8_t *data)
{
    if (memcmp(data, &expected_ack_crc, 4) == 0)
    { // got an ACK from recipient
        Serial.printf("   Got ACK! (round trip: %d millis)\n", _ms->getMillis() - last_msg_sent);
        // NOTE: the same ACK can be received multiple times!
        expected_ack_crc = 0; // reset our expected hash, now that we have received ACK
        return true;
    }

    // uint32_t crc;
    // memcpy(&crc, data, 4);
    // MESH_DEBUG_PRINTLN("unknown ACK received: %08X (expected: %08X)", crc, expected_ack_crc);
    return false;
}

void PunkMesh::onMessageRecv(const ContactInfo &from, mesh::Packet *pkt, uint32_t sender_timestamp, const char *text)
{
    Serial.printf("(%s) MSG -> from %s\n", pkt->isRouteDirect() ? "DIRECT" : "FLOOD", from.name);
    Serial.printf("   %s\n", text);

    if (strcmp(text, "clock sync") == 0)
    { // special text command
        setClock(sender_timestamp + 1);
    }
}

void PunkMesh::onCommandDataRecv(const ContactInfo &from, mesh::Packet *pkt, uint32_t sender_timestamp, const char *text)
{
}
void PunkMesh::onSignedMessageRecv(const ContactInfo &from, mesh::Packet *pkt, uint32_t sender_timestamp, const uint8_t *sender_prefix, const char *text)
{
}

void PunkMesh::onChannelMessageRecv(const mesh::GroupChannel &channel, mesh::Packet *pkt, uint32_t timestamp, const char *text)
{
    // Send to Lua
    if (lua_runtime) {
        lua_mesh_push_message(lua_runtime, channel, pkt, timestamp, text);
    }

    if (pkt->isRouteDirect())
    {
        Serial.printf("PUBLIC CHANNEL MSG -> (Direct!)\n");
    }
    else
    {
        Serial.printf("PUBLIC CHANNEL MSG -> (Flood) hops %d\n", pkt->path_len);
    }
    // Serial.printf("   %s\n", text);
}

uint8_t PunkMesh::onContactRequest(const ContactInfo &contact, uint32_t sender_timestamp, const uint8_t *data, uint8_t len, uint8_t *reply)
{
    return 0; // unknown
}

void PunkMesh::onContactResponse(const ContactInfo &contact, const uint8_t *data, uint8_t len)
{
    // not supported
}

uint32_t PunkMesh::calcFloodTimeoutMillisFor(uint32_t pkt_airtime_millis) const
{
    return SEND_TIMEOUT_BASE_MILLIS + (FLOOD_SEND_TIMEOUT_FACTOR * pkt_airtime_millis);
}
uint32_t PunkMesh::calcDirectTimeoutMillisFor(uint32_t pkt_airtime_millis, uint8_t path_len) const
{
    return SEND_TIMEOUT_BASE_MILLIS +
           ((pkt_airtime_millis * DIRECT_SEND_PERHOP_FACTOR + DIRECT_SEND_PERHOP_EXTRA_MILLIS) * (path_len + 1));
}

void PunkMesh::onSendTimeout()
{
    Serial.println("   ERROR: timed out, no ACK.");
}

PunkMesh::PunkMesh(mesh::Radio &radio, StdRNG &rng, mesh::RTCClock &rtc, SimpleMeshTables &tables)
    : BaseChatMesh(radio, *new ArduinoMillis(), rng, rtc, *new StaticPoolPacketManager(16), tables)
{
    // defaults
    memset(&_prefs, 0, sizeof(_prefs));
    _prefs.airtime_factor = 2.0; // one third
    strcpy(_prefs.node_name, "NONAME");
    _prefs.freq = LORA_FREQ;
    _prefs.tx_power_dbm = LORA_TX_POWER;

    command[0] = 0;
    curr_recipient = NULL;
}

float PunkMesh::getFreqPref() const { return _prefs.freq; }
uint8_t PunkMesh::getTxPowerPref() const { return _prefs.tx_power_dbm; }

void PunkMesh::begin()
{
    BaseChatMesh::begin();

#if defined(NRF52_PLATFORM)
    IdentityStore store(fs, "");
#elif defined(RP2040_PLATFORM)
    IdentityStore store(fs, "/identity");
    store.begin();
#else
// IdentityStore store(fs, "/identity");
#endif
    // if (!store.load("_main", self_id, _prefs.node_name, sizeof(_prefs.node_name))) {  // legacy: node_name was from identity file
    //  Need way to get some entropy to seed RNG

#if defined(PRV_KEY)
    self_id = mesh::LocalIdentity(PRV_KEY, PUB_KEY);
#else
    Serial.println("Press ENTER to generate key:");
    char c = 0;
    while (c != '\n')
    { // wait for ENTER to be pressed
        if (Serial.available())
            c = Serial.read();
    }
    ((StdRNG *)getRNG())->begin(millis());

    self_id = mesh::LocalIdentity(getRNG()); // create new random identity

    int count = 0;
    while (count < 10 && (self_id.pub_key[0] == 0x00 || self_id.pub_key[0] == 0xFF))
    { // reserved id hashes
        self_id = mesh::LocalIdentity(getRNG());
        count++;
    }
#endif
    // store.save("_main", self_id);
    // }

    // load persisted prefs
    if (LittleFS.exists("/node_prefs"))
    {
#if defined(RP2040_PLATFORM)
        File file = LittleFS.open("/node_prefs", "r");
#else
        File file = LittleFS.open("/node_prefs");
#endif
        if (file)
        {
            file.read((uint8_t *)&_prefs, sizeof(_prefs));
            file.close();
        }
    }

    loadContacts();
    _public = addChannel("Public", PUBLIC_GROUP_PSK); // pre-configure Andy's public channel

    Serial.print("Created public channel");
}

  void PunkMesh::logRx(mesh::Packet* pkt, int len, float score) {
        Serial.print(getLogDateTime());
        Serial.print(": RX, len=");
        Serial.print(len);
        Serial.print(" (type=");
        Serial.print(pkt->getPayloadType());
        Serial.print(", route=");
        Serial.print(pkt->isRouteDirect() ? "D" : "F");
        Serial.print(", payload_len=");
        Serial.print(pkt->payload_len);
        Serial.print(") SNR=");
        Serial.print((int)_radio->getLastSNR());
        Serial.print(" RSSI=");
        Serial.print((int)_radio->getLastRSSI());
        Serial.print(" score=");
        Serial.println((int)(score*1000));

    //     if (pkt->getPayloadType() == PAYLOAD_TYPE_PATH || pkt->getPayloadType() == PAYLOAD_TYPE_REQ
    //       || pkt->getPayloadType() == PAYLOAD_TYPE_RESPONSE || pkt->getPayloadType() == PAYLOAD_TYPE_TXT_MSG) {
    //       Serial.printf(" [%02X -> %02X]\n", (uint32_t)pkt->payload[1], (uint32_t)pkt->payload[0]);
    //     } else {
    //       Serial.printf("\n");
    //     }
    //     f.close();
    //   
  }

void PunkMesh::savePrefs()
{
#if defined(NRF52_PLATFORM)
    LittleFS.remove("/node_prefs");
    File file = LittleFS.open("/node_prefs", FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
    File file = LittleFS.open("/node_prefs", "w");
#else
    File file = LittleFS.open("/node_prefs", "w", true);
#endif
    if (file)
    {
        file.write((const uint8_t *)&_prefs, sizeof(_prefs));
        file.close();
    }
}

void PunkMesh::showWelcome()
{
    Serial.println("===== MeshCore Chat Terminal =====");
    Serial.println();
    Serial.printf("WELCOME  %s\n", _prefs.node_name);

    // Serial.print("Public key: ");
    // mesh::Utils::printHex(Serial, self_id.pub_key, PUB_KEY_SIZE);

    // Serial.print("Private key: ");
    // mesh::Utils::printHex(Serial, self_id.prv_key, PRIV_KEY_SIZE);

    self_id.printTo(Serial);

    Serial.println();
    Serial.println("   (enter 'help' for basic commands)");
    Serial.println();
}

void PunkMesh::sendSelfAdvert(int delay_millis)
{
    auto pkt = createSelfAdvert(_prefs.node_name, _prefs.node_lat, _prefs.node_lon);
    if (pkt)
    {
        sendFlood(pkt, delay_millis);
    }
}

// ContactVisitor
void PunkMesh::onContactVisit(const ContactInfo &contact)
{
    Serial.printf("   %s - ", contact.name);
    char tmp[40];
    int32_t secs = contact.last_advert_timestamp - getRTCClock()->getCurrentTime();
    AdvertTimeHelper::formatRelativeTimeDiff(tmp, secs, false);
    Serial.println(tmp);
}

void PunkMesh::handleCommand(const char *command)
{
    while (*command == ' ')
        command++; // skip leading spaces

    if (memcmp(command, "send ", 5) == 0)
    {
        if (curr_recipient)
        {
            const char *text = &command[5];
            uint32_t est_timeout;

            int result = sendMessage(*curr_recipient, getRTCClock()->getCurrentTime(), 0, text, expected_ack_crc, est_timeout);
            if (result == MSG_SEND_FAILED)
            {
                Serial.println("   ERROR: unable to send.");
            }
            else
            {
                last_msg_sent = _ms->getMillis();
                Serial.printf("   (message sent - %s)\n", result == MSG_SEND_SENT_FLOOD ? "FLOOD" : "DIRECT");
            }
        }
        else
        {
            Serial.println("   ERROR: no recipient selected (use 'to' cmd).");
        }
    }
    else if (memcmp(command, "public ", 7) == 0)
    { // send GroupChannel msg
        uint8_t temp[5 + MAX_TEXT_LEN + 32];
        uint32_t timestamp = getRTCClock()->getCurrentTime();
        memcpy(temp, &timestamp, 4); // mostly an extra blob to help make packet_hash unique
        temp[4] = 0;                 // attempt and flags

        sprintf((char *)&temp[5], "%s: %s", _prefs.node_name, &command[7]); // <sender>: <msg>
        temp[5 + MAX_TEXT_LEN] = 0;                                         // truncate if too long

        Serial.println((char *)temp);

        int len = strlen((char *)&temp[5]);
        auto pkt = createGroupDatagram(PAYLOAD_TYPE_GRP_TXT, _public->channel, temp, 5 + len);
        if (pkt)
        {
            Serial.println("   Sending...");
            sendFlood(pkt);
            Serial.println("   Sent.");
        }
        else
        {
            Serial.println("   ERROR: unable to send");
        }
    }
    else if (memcmp(command, "list", 4) == 0)
    { // show Contact list, by most recent
        int n = 0;
        if (command[4] == ' ')
        { // optional param, last 'N'
            n = atoi(&command[5]);
        }
        scanRecentContacts(n, this);
    }
    else if (strcmp(command, "clock") == 0)
    { // show current time
        uint32_t now = getRTCClock()->getCurrentTime();
        DateTime dt = DateTime(now);
        Serial.printf("%02d:%02d - %d/%d/%d UTC\n", dt.hour(), dt.minute(), dt.day(), dt.month(), dt.year());
    }
    else if (memcmp(command, "time ", 5) == 0)
    { // set time (to epoch seconds)
        uint32_t secs = _atoi(&command[5]);
        setClock(secs);
    }
    else if (memcmp(command, "to ", 3) == 0)
    { // set current recipient
        curr_recipient = searchContactsByPrefix(&command[3]);
        if (curr_recipient)
        {
            Serial.printf("   Recipient %s now selected.\n", curr_recipient->name);
        }
        else
        {
            Serial.println("   Error: Name prefix not found.");
        }
    }
    else if (strcmp(command, "to") == 0)
    { // show current recipient
        if (curr_recipient)
        {
            Serial.printf("   Current: %s\n", curr_recipient->name);
        }
        else
        {
            Serial.println("   Err: no recipient selected");
        }
    }
    else if (strcmp(command, "advert") == 0)
    {
        auto pkt = createSelfAdvert(_prefs.node_name, _prefs.node_lat, _prefs.node_lon);
        if (pkt)
        {
            sendZeroHop(pkt);
            Serial.println("   (advert sent, zero hop).");
        }
        else
        {
            Serial.println("   ERR: unable to send");
        }
    }
    else if (strcmp(command, "reset path") == 0)
    {
        if (curr_recipient)
        {
            resetPathTo(*curr_recipient);
            saveContacts();
            Serial.println("   Done.");
        }
    }
    else if (memcmp(command, "card", 4) == 0)
    {
        Serial.printf("Hello %s\n", _prefs.node_name);
        auto pkt = createSelfAdvert(_prefs.node_name, _prefs.node_lat, _prefs.node_lon);
        if (pkt)
        {
            uint8_t len = pkt->writeTo(tmp_buf);
            releasePacket(pkt); // undo the obtainNewPacket()

            mesh::Utils::toHex(hex_buf, tmp_buf, len);
            Serial.println("Your MeshCore biz card:");
            Serial.print("meshcore://");
            Serial.println(hex_buf);
            Serial.println();
        }
        else
        {
            Serial.println("  Error");
        }
    }
    else if (memcmp(command, "import ", 7) == 0)
    {
        importCard(&command[7]);
    }
    else if (memcmp(command, "set ", 4) == 0)
    {
        const char *config = &command[4];
        if (memcmp(config, "af ", 3) == 0)
        {
            _prefs.airtime_factor = atof(&config[3]);
            savePrefs();
            Serial.println("  OK");
        }
        else if (memcmp(config, "name ", 5) == 0)
        {
            StrHelper::strncpy(_prefs.node_name, &config[5], sizeof(_prefs.node_name));
            savePrefs();
            Serial.println("  OK");
        }
        else if (memcmp(config, "lat ", 4) == 0)
        {
            _prefs.node_lat = atof(&config[4]);
            savePrefs();
            Serial.println("  OK");
        }
        else if (memcmp(config, "lon ", 4) == 0)
        {
            _prefs.node_lon = atof(&config[4]);
            savePrefs();
            Serial.println("  OK");
        }
        else if (memcmp(config, "tx ", 3) == 0)
        {
            _prefs.tx_power_dbm = atoi(&config[3]);
            savePrefs();
            Serial.println("  OK - reboot to apply");
        }
        else if (memcmp(config, "freq ", 5) == 0)
        {
            _prefs.freq = atof(&config[5]);
            savePrefs();
            Serial.println("  OK - reboot to apply");
        }
        else
        {
            Serial.printf("  ERROR: unknown config: %s\n", config);
        }
    }
    else if (memcmp(command, "ver", 3) == 0)
    {
        Serial.println(FIRMWARE_VER_TEXT);
    }
    else if (memcmp(command, "help", 4) == 0)
    {
        Serial.println("Commands:");
        Serial.println("   set {name|lat|lon|freq|tx|af} {value}");
        Serial.println("   card");
        Serial.println("   import {biz card}");
        Serial.println("   clock");
        Serial.println("   time <epoch-seconds>");
        Serial.println("   list {n}");
        Serial.println("   to <recipient name or prefix>");
        Serial.println("   to");
        Serial.println("   send <text>");
        Serial.println("   advert");
        Serial.println("   reset path");
        Serial.println("   public <text>");
    }
    else
    {
        Serial.print("   ERROR: unknown command: ");
        Serial.println(command);
    }
}

void PunkMesh::loop()
{
    BaseChatMesh::loop();

    int len = strlen(command);
    while (Serial.available() && len < sizeof(command) - 1)
    {
        char c = Serial.read();
        if (c != '\n')
        {
            command[len++] = c;
            command[len] = 0;
        }
        Serial.print(c);
    }
    if (len == sizeof(command) - 1)
    { // command buffer full
        command[sizeof(command) - 1] = '\r';
    }

    if (len > 0 && command[len - 1] == '\r')
    {                         // received complete line
        command[len - 1] = 0; // replace newline with C string null terminator

        handleCommand(command);
        command[0] = 0; // reset command buffer
    }
}

