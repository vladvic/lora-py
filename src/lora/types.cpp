/***************************************************
 * lora.cpp
 * Created on Fri, 10 Jul 2020 04:14:09 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <sstream>
#include <ctype.h>
#include <string.h>
#include "util/concat.h"
#include "aes.h"
#include "types.h"

namespace lora {

DeviceInfo::DeviceInfo(const std::string &devEui,
                       const std::string &appEui,
                       const std::string &appKey)
  : devClass(CLASS_A)
  , session(NULL)
{
  stringToHex(devEui, this->devEUI, sizeof(this->devEUI));
  stringToHex(appEui, this->appEUI, sizeof(this->appEUI));
  stringToHex(appKey, this->appKey, sizeof(this->appKey));
}

MediaParameters::MediaParameters()
{
  frequency = 0;
  interface = -1;
  rfChain = -1;
  crc = 0;
  modulation = LORA;
  dataRate = DR0;
  codingRate = CR4_5;
  rssi = 0;
  lsnr = 0;
}

MediaSettings::MediaSettings()
{
  frequency = 0;
  rfChain = -1;
  power = 0;
  modulation = LORA;
  dataRate = DR0;
  codingRate = CR4_5;
  fDeviation = -1;
  polarity = NORMAL;
  preambleSize = -1;
  disableCrc = false;
}

std::string hexToString(const Bytearray &bytes) {
  return hexToString(bytes.data(), bytes.size());
}

std::string hexToString(const unsigned char *hex, int hexLen) {
  char digit[] = "0123456789abcdef";
  std::string result;

  result.reserve(hexLen * 2);

  for(int i = 0; i < hexLen; i ++) {
    char byte = hex[i];
    result += digit[(byte >> 4) & 0xf];
    result += digit[(byte >> 0) & 0xf];
  }

  return result;
}

int stringToHex(const std::string &str, unsigned char *hex, int hexLen) {
  int pos = 0;

  for(auto i = str.begin(); i != str.end(); ++ i) {
    char digit = *i;
    char value = 0;

    if(pos / 2 >= hexLen) {
      return pos / 2;
    }

    if(digit >= '0' && digit <= '9') {
      value = digit - '0';
    }

    if(tolower(digit) >= 'a' && tolower(digit) <= 'f') {
      value = 10 + tolower(digit) - 'a';
    }

    if(pos % 2) {
      hex[pos / 2] |= value << 0;
    }
    else {
      hex[pos / 2] = value << 4;
    }

    pos ++;
  }

  return pos / 2;
}

void DeviceSession::generateSessionKeys()
{
  if(!this->device) {
    return;
  }

  Bytearray block = concat((uint8_t)0x00, (uint24_t)nOnce, 
                                          (uint24_t)networkId, (uint16_t)devNOnce, 
                                          (uint32_t)0, (uint16_t)0, (uint8_t)0);

  block[0] = 0x01;
  Bytearray fNwkSIntKey = aes128_encrypt(this->device->appKey, block);
  block[0] = 0x02;
  Bytearray appSKey = aes128_encrypt(this->device->appKey, block);
  Bytearray sNwkSIntKey = fNwkSIntKey;
  Bytearray nwkSEncKey = fNwkSIntKey;

  memcpy(this->fNwkSIntKey, fNwkSIntKey.data(), sizeof(this->fNwkSIntKey));
  memcpy(this->appSKey,     appSKey.data(),     sizeof(this->appSKey));
  memcpy(this->sNwkSIntKey, sNwkSIntKey.data(), sizeof(this->sNwkSIntKey));
  memcpy(this->nwkSEncKey,  nwkSEncKey.data(),  sizeof(this->nwkSEncKey));
};

std::string 
dumpSession(const DeviceSession *s) {
  std::stringstream ss;

  ss << "DeviceSession<" << s << ">: ";
  ss << s->networkId << ":" << s->deviceAddr << " <" << s->nOnce << ">:<" << s->devNOnce << ">\n";
  ss << "Keys:\n";
  ss << "\t" << hexToString(s->fNwkSIntKey, sizeof(Key)) << "\n";
  ss << "\t" << hexToString(s->sNwkSIntKey, sizeof(Key)) << "\n";
  ss << "\t" << hexToString(s->nwkSEncKey, sizeof(Key)) << "\n";
  ss << "\t" << hexToString(s->appSKey, sizeof(Key)) << "\n";

  return ss.str();
};

}

#ifdef TEST
#include <iostream>
#include <stdio.h>

int main() {
  std::string str = "373130386b396703";
  lora::Bytearray hex;
  hex.resize(8);

  int syms = lora::stringToHex(str, hex.data(), hex.size());
  str = lora::hexToString(hex);

  for(int i = 0; i < hex.size(); i ++) {
    printf("%0.2x", (unsigned char)hex[i]);
  }
  printf("\n");
  std::cout << str << std::endl;
}
#endif
