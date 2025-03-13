#pragma once

#include "Mesh.h"
#include <helpers/IdentityStore.h>

struct NodePrefs {  // persisted to file
    float airtime_factor;
    char node_name[32];
    double node_lat, node_lon;
    char password[16];
    float freq;
    uint8_t tx_power_dbm;
    uint8_t disable_fwd;
    uint8_t advert_interval;   // minutes
    uint8_t unused;
    float rx_delay_base;
    float tx_delay_factor;
    char guest_password[16];
    float direct_tx_delay_factor;
    uint32_t guard;
    uint8_t sf;
    uint8_t cr;
    uint8_t reserved1;
    uint8_t reserved2;
    float bw;
};

class CommonCLICallbacks {
public:
  virtual void savePrefs() = 0;
  virtual const char* getFirmwareVer() = 0;
  virtual const char* getBuildDate() = 0;
  virtual bool formatFileSystem() = 0;
  virtual void sendSelfAdvertisement(int delay_millis) = 0;
  virtual void updateAdvertTimer() = 0;
  virtual void setLoggingOn(bool enable) = 0;
  virtual void eraseLogFile() = 0;
  virtual void dumpLogFile() = 0;
  virtual void setTxPower(uint8_t power_dbm) = 0;
};

class CommonCLI {
  mesh::Mesh* _mesh;
  NodePrefs* _prefs;
  CommonCLICallbacks* _callbacks;
  mesh::MainBoard* _board;
  char tmp[80];

  mesh::RTCClock* getRTCClock() { return _mesh->getRTCClock(); }
  void savePrefs() { _callbacks->savePrefs(); }

  void checkAdvertInterval();

public:
  CommonCLI(mesh::MainBoard& board, mesh::Mesh* mesh, NodePrefs* prefs, CommonCLICallbacks* callbacks) 
      : _board(&board), _mesh(mesh), _prefs(prefs), _callbacks(callbacks) { }

  void loadPrefs(FILESYSTEM* _fs);
  void savePrefs(FILESYSTEM* _fs);
  void handleCommand(uint32_t sender_timestamp, const char* command, char* reply);
};
