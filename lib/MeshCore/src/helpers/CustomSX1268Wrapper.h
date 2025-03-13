#pragma once

#include "CustomSX1268.h"
#include "RadioLibWrappers.h"

class CustomSX1268Wrapper : public RadioLibWrapper {
public:
  CustomSX1268Wrapper(CustomSX1268& radio, mesh::MainBoard& board) : RadioLibWrapper(radio, board) { }
  bool isReceiving() override { 
    if (((CustomSX1268 *)_radio)->isReceiving()) return true;

    idle();  // put sx126x into standby
    // do some basic CAD (blocks for ~12780 micros (on SF 10)!)
    bool activity = (((CustomSX1268 *)_radio)->scanChannel() == RADIOLIB_LORA_DETECTED);
    if (activity) {
      startRecv();
    } else {
      idle();
    }
    return activity;
  }
  float getLastRSSI() const override { return ((CustomSX1268 *)_radio)->getRSSI(); }
  float getLastSNR() const override { return ((CustomSX1268 *)_radio)->getSNR(); }

  float packetScore(float snr, int packet_len) override {
    int sf = ((CustomSX1268 *)_radio)->spreadingFactor;
    return packetScoreInt(snr, sf, packet_len);
  }
};
