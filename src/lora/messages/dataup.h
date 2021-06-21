/***************************************************
 * joinrequest.h
 * Created on Wed, 15 Jul 2020 08:34:08 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include "../message.h"

namespace lora {

enum CtrlBits {
  ADR = 0x80,
  ADRACKReq = 0x40,
  ACK = 0x20,
  FPending = 0x10
};

enum Direction {
  UPLINK = 0,
  DOWNLINK = 1
};

class DataUp
  : public Message
{
public:
  DataUp();

  uint32_t devAddress() const;
  uint8_t  ctrl() const;
  uint16_t cnt() const;
  uint8_t  port() const;
  const Bytearray &opts() const;
  const Bytearray &data() const;

  void setDevAddress(uint32_t value);
  void setCtrl(uint8_t ctrl);
  void setCnt(uint16_t cnt);
  void setOpts(const Bytearray &opts);
  void setPort(uint8_t port);
  void setData(const Bytearray &data);

private:
  uint32_t calcMic();
  void composeMsg(Bytearray &msg, const Device *dev);
  virtual
  void compose(const Device *dev);
  virtual
  void parse(const Device *dev);
  virtual
  Direction getDirection() { return UPLINK; }

  uint32_t m_devAddr;
  uint8_t  m_ctrl;
  uint8_t  m_port;
  uint16_t m_cnt;
  Bytearray m_optsEncrypted;
  Bytearray m_dataEncrypted;
  Bytearray m_opts;
  Bytearray m_data;
};

class UnconfirmedDataUp 
  : public DataUp
{
public:
  MessageType type() { return UNCONFIRMED_DATA_UP; }

private:
  typedef Message::Registrar<UNCONFIRMED_DATA_UP, UnconfirmedDataUp> Registrar;
  static Registrar s_registrar;
};

class ConfirmedDataUp 
  : public DataUp
{
public:
  MessageType type() { return CONFIRMED_DATA_UP; }

private:
  typedef Message::Registrar<CONFIRMED_DATA_UP, ConfirmedDataUp> Registrar;
  static Registrar s_registrar;
};

class UnconfirmedDataDown
  : public DataUp
{
public:
  MessageType type() { return UNCONFIRMED_DATA_DOWN; }
  Direction getDirection() { return DOWNLINK; }

private:
  typedef Message::Registrar<UNCONFIRMED_DATA_DOWN, UnconfirmedDataDown> Registrar;
  static Registrar s_registrar;
};

class ConfirmedDataDown
  : public DataUp
{
public:
  MessageType type() { return CONFIRMED_DATA_DOWN; }
  Direction getDirection() { return DOWNLINK; }

private:
  typedef Message::Registrar<CONFIRMED_DATA_DOWN, ConfirmedDataDown> Registrar;
  static Registrar s_registrar;
};

}
