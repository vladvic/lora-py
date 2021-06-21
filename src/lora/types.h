/***************************************************
 * lora.h
 * Created on Thu, 09 Jul 2020 18:56:06 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <memory>
#include <vector>
#include <list>
#include <string>
#include <stdint.h>

namespace lora {

enum MacCID {
  LinkCheckReq = 0x02,
  LinkCheckAns = 0x02,
  LinkADRReq = 0x03,
  LinkADRAns = 0x03,
  DutyCycleReq = 0x04,
  DutyCycleAns = 0x04,
  RXParamSetupReq = 0x05,
  RXParamSetupAns = 0x05,
  DevStatusReq = 0x06,
  DevStatusAns = 0x06,
  NewChannelReq = 0x07,
  NewChannelAns = 0x07,
  RXTimingSetupReq = 0x08,
  RXTimingSetupAns = 0x08,
};

enum Modulation {
  LORA, FSK,
};

enum DataRate {
  DR0, DR1, DR2, DR3, 
  DR4, DR5, DR6, DR7,
  DR8, DR9, DR10, DR11,
  DR12, DR13, DR14, DR15,
};

enum CodingRate {
  CR4_5, CR4_6, CR4_7, CR4_8
};

enum Polarity {
  NORMAL, INVERSE
};

enum DeviceClass {
  CLASS_A = 0,
  CLASS_B,
  CLASS_C
};

struct MediaParameters {
  uint32_t rxTimestampUs; // Receive timestamp in milliseconds
  int frequency; // Frequency, Hz
  int interface; // Concentrator "IF" channel used for RX (unsigned integer)
  int rfChain; // Concentrator "RF chain" used for RX (unsigned integer)
  bool crc; // CRC status: 1 = OK, -1 = fail, 0 = no CRC
  Modulation modulation; // Modulation identifier "LORA" or "FSK"
  union {
    DataRate dataRate; // LoRa data rate identifier (eg. SF12BW500)
    int fskDataRate; // FSK data rate
  };
  CodingRate codingRate; // LoRa ECC coding rate identifier
  int rssi; // RSSI in dBm (signed integer, 1 dB precision)
  double lsnr; // Lora SNR ratio in dB (signed float, 0.1 dB precision)

  MediaParameters();
};

struct MediaSettings {
  uint32_t txTimestampUs; // Schedule transmission to timestamp in milliseconds
  int frequency; // TX central frequency in MHz (unsigned float, Hz precision)
  int rfChain; // Concentrator "RF chain" used for TX (unsigned integer)
  int power; // TX output power in dBm (unsigned integer, dBm precision)
  Modulation modulation; // Modulation identifier "LORA" or "FSK"
  union {
    DataRate dataRate; // LoRa data rate identifier (eg. SF12BW500)
    int fskDataRate; // FSK data rate
  };
  CodingRate codingRate; // LoRa ECC coding rate identifier
  int fDeviation; // FSK frequency deviation (unsigned integer, in Hz) 
  Polarity polarity;
  int preambleSize;
  bool disableCrc;

  MediaSettings();
};

typedef std::vector<uint8_t> Bytearray;
typedef uint8_t EUI[8];
typedef uint8_t Key[16];

class GatewayId {
public:
  virtual ~GatewayId() = default;
  virtual std::string toString() const = 0;
  virtual GatewayId *clone() const = 0;
};

typedef std::vector<uint32_t> ChannelList;

struct UInt24_T_Struct {
  uint32_t value:24;

  template<typename IntType>
  UInt24_T_Struct(IntType v) { value = v; }
  template<typename IntType>
  operator IntType() { return value; }
} __attribute__((__packed__));

typedef struct UInt24_T_Struct uint24_t;

struct DeviceSession;

struct DeviceInfo {
  EUI devEUI;
  EUI appEUI;
  Key appKey;
  ChannelList cflist;
  DeviceClass devClass;
  DeviceSession *session;

  DeviceInfo() 
    : devClass(CLASS_A)
    , session(NULL)
  { }
  DeviceInfo(const std::string &devEui,
             const std::string &appEui,
             const std::string &appKey);
};

struct DeviceSession {
  DeviceInfo *device;
  uint32_t networkId;
  uint32_t deviceAddr;
  uint32_t nOnce;
  uint32_t devNOnce;
  Key fNwkSIntKey;
  Key sNwkSIntKey;
  Key nwkSEncKey;
  Key appSKey;
  time_t lastAccessTime;
  uint32_t fCntUp;
  uint32_t fCntDown;
  uint8_t rxDelay1;
  uint8_t joinAckDelay1;
  uint32_t rx2Channel;
  DataRate rx2Datarate;
  void *userInfo;

  DeviceSession() 
    : device(NULL)
    , nOnce(0)
    , devNOnce(0)
    , fCntUp(0)
    , fCntDown(0)
    , rxDelay1(5)
    , joinAckDelay1(5)
  { }
  void generateSessionKeys();

  time_t lastReceivedUs;
  MediaParameters lastReceivedMedia;
};

struct Device {
  const DeviceInfo    *info;
  const DeviceSession *session;
  Device(const DeviceInfo *info, const DeviceSession *session)
    : info(info)
    , session(session)
  { }
};

}


#include "util/concat.h"

namespace lora {

struct MacCommand {
  MacCID command;
  DeviceSession *session;
  std::unique_ptr<GatewayId> gatewayId;
  Bytearray params;

  template<typename ... Ts>
  MacCommand(MacCID cmd, DeviceSession *s, const GatewayId &gw, const Ts &... xs)
    : command(cmd)
    , session(s)
    , gatewayId(gw.clone())
  {
    concat_recursive(this->params, xs...);
  }

  MacCommand(MacCID cmd, DeviceSession *s, const GatewayId &gw)
    : command(cmd)
    , session(s)
    , gatewayId(gw.clone())
  { }
};

std::string dumpSession(const DeviceSession *s);
std::string hexToString(const unsigned char *hex, int hexLen);
std::string hexToString(const Bytearray &bytes);
int stringToHex(const std::string &str, unsigned char *hex, int hexLen);

}
