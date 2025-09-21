#ifndef PUNKMESH_H
#define PUNKMESH_H

#include <Arduino.h>
#include <Mesh.h>
#include <helpers/BaseChatMesh.h>
#include <helpers/SimpleMeshTables.h>
#include <helpers/StaticPoolPacketManager.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/IdentityStore.h>
#include <RTClib.h>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <luavgl.h>
}

// Forward declarations
// class PunkMesh;

// NodePrefs is shared between Lua + C++ config
struct NodePrefs
{
  float airtime_factor;
  char node_name[32];
  double node_lat, node_lon;
  float freq;
  uint8_t tx_power_dbm;
  uint8_t unused[3];
};

struct MeshMessage {
    char from[32];     // short pubkey or addr
    char text[128];    // payload
    uint32_t timestamp;
    uint8_t hops;
    bool direct;
};

#define MAX_MESSAGES 64   // tweak as needed

static MeshMessage message_history[MAX_MESSAGES];
static int msg_head = 0;
static int msg_count = 0;

// Class declaration
class PunkMesh : public BaseChatMesh, ContactVisitor
{
public:
  NodePrefs _prefs;
  uint32_t expected_ack_crc;
  ChannelDetails *_public;
  unsigned long last_msg_sent;
  ContactInfo *curr_recipient;
  char command[512 + 10];
  uint8_t tmp_buf[256];
  char hex_buf[512];
  lua_State *lua_runtime = NULL;

  PunkMesh(mesh::Radio &radio, StdRNG &rng, mesh::RTCClock &rtc, SimpleMeshTables &tables);

  void begin();
  void loop();
  void handleCommand(const char *command);
  void sendSelfAdvert(int delay_millis);
  void showWelcome();
  void savePrefs();
  const char *getTypeName(uint8_t type) const;
  void loadContacts();
  void saveContacts();
  void setClock(uint32_t timestamp);
  void importCard(const char *command);

  float getFreqPref() const;
  uint8_t getTxPowerPref() const;

  void broadcastMessage(const char* text);

protected:
  void logRx(mesh::Packet *pkt, int len, float score) override;
  float getAirtimeBudgetFactor() const override;
  int calcRxDelay(float score, uint32_t air_time) const override;
  bool allowPacketForward(const mesh::Packet *packet) override;
  void onDiscoveredContact(ContactInfo &contact, bool is_new, uint8_t path_len, const uint8_t *path) override;
  void onContactPathUpdated(const ContactInfo &contact) override;
  bool processAck(const uint8_t *data) override;
  void onMessageRecv(const ContactInfo &from, mesh::Packet *pkt, uint32_t sender_timestamp, const char *text) override;
  void onCommandDataRecv(const ContactInfo &from, mesh::Packet *pkt, uint32_t sender_timestamp, const char *text) override;
  void onSignedMessageRecv(const ContactInfo &from, mesh::Packet *pkt, uint32_t sender_timestamp, const uint8_t *sender_prefix, const char *text) override;
  void onChannelMessageRecv(const mesh::GroupChannel &channel, mesh::Packet *pkt, uint32_t timestamp, const char *text) override;
  uint8_t onContactRequest(const ContactInfo &contact, uint32_t sender_timestamp, const uint8_t *data, uint8_t len, uint8_t *reply) override;
  void onContactResponse(const ContactInfo &contact, const uint8_t *data, uint8_t len) override;
  void onSendTimeout() override;
  uint32_t calcFloodTimeoutMillisFor(uint32_t pkt_airtime_millis) const override;
  uint32_t calcDirectTimeoutMillisFor(uint32_t pkt_airtime_millis, uint8_t path_len) const override;

  void onContactVisit(const ContactInfo &contact) override;

  // Punk stuff
  void store_message(const char* from, const char* text, uint32_t timestamp, uint8_t hops, bool direct);

};

// Optional: declare global instance if using one
// extern MyMesh the_mesh;

#endif