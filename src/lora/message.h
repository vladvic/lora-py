/***************************************************
 * message.h
 * Created on Mon, 13 Jul 2020 05:03:49 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <memory>
#include <functional>
#include <map>
#include "handler.h"
#include "types.h"

namespace lora {

class AppHandler;

enum MessageType {
  NONE = -1,
  JOIN_REQUEST = 0,
  JOIN_ACCEPT  = 1,
  UNCONFIRMED_DATA_UP   = 2,
  UNCONFIRMED_DATA_DOWN = 3,
  CONFIRMED_DATA_UP   = 4,
  CONFIRMED_DATA_DOWN = 5,
  REJOIN_REQUEST = 6,
};

class Message;

class Message {
public:
  Message()
    : m_device(NULL, NULL)
  { }
  virtual ~Message() = default;
  const Bytearray &getPayload();
  void setPayload(const Bytearray &);
  void clearPayload();

  void setDevice(Device *device);
  const Device *getDevice() const { return &m_device; }

  virtual
  uint32_t mic();
  virtual
  MessageType type();
  bool verify() const;

  typedef std::function<Message *()> Factory;

  static
  Message *create(uint8_t type);

  template<int N, typename T>
  class Registrar {
  public:
    Registrar() {
      Message::registerFactory(N, []() { return new T; });
    }
  };

private:
  virtual
  void compose(const Device *) = 0;
  virtual
  void parse(const Device *) = 0;

  static
  void registerFactory(uint8_t type, const Factory &factory);
  static std::map<int, Factory> s_messageFactories;

  Device m_device;

protected:
  template<int N, typename T>
  friend class Registrar;
  Bytearray m_payload;
};

class Packet {
public:
  Packet();
  Packet(const Bytearray &ba);
  Packet(MessageType t);
  void setDevice(Device *device);
  MessageType type();
  template<typename Type>
  Type &message() {
    if(!m_message) {
      std::unique_ptr<Type> t(new Type());
      if(dynamic_cast<Message*>(t.get())) {
        m_message.reset(dynamic_cast<Message*>(t.release()));
        m_message->setDevice(m_device);
      }
    }

    Type *result = dynamic_cast<Type*>(m_message.get());

    if(!result) {
      throw std::invalid_argument("Invalid message type!");
    }

    return dynamic_cast<Type&>(*m_message.get());
  }
  bool verify();

private:
  uint8_t m_mic;
  std::unique_ptr<Message> m_message;
  Device *m_device;
};

}

#include "messages/joinrequest.h"
#include "messages/joinaccept.h"
#include "messages/data.h"

