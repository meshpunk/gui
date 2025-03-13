#include "Packet.h"
#include <string.h>
#include <SHA256.h>

namespace mesh {

Packet::Packet() {
  header = 0;
  path_len = 0;
  payload_len = 0;
}


void Packet::calculatePacketHash(uint8_t* hash) const {
  SHA256 sha;
  uint8_t t = getPayloadType();
  sha.update(&t, 1);
  sha.update(payload, payload_len);
  sha.finalize(hash, MAX_HASH_SIZE);
}

uint8_t Packet::writeTo(uint8_t dest[]) const {
  uint8_t i = 0;
  dest[i++] = header;
  dest[i++] = path_len;
  memcpy(&dest[i], path, path_len); i += path_len;
  memcpy(&dest[i], payload, payload_len); i += payload_len;
  return i;
}

bool Packet::readFrom(const uint8_t src[], uint8_t len) {
  uint8_t i = 0;
  header = src[i++];
  path_len = src[i++];
  if (path_len > sizeof(path)) return false;   // bad encoding
  memcpy(path, &src[i], path_len); i += path_len;
  if (i >= len) return false;   // bad encoding
  payload_len = len - i;
  memcpy(payload, &src[i], payload_len); //i += payload_len;
  return true;   // success
}

}