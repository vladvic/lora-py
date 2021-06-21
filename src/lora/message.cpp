/***************************************************
 * message.cpp
 * Created on Tue, 14 Jul 2020 05:23:39 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <iostream>
#include "message.h"
#include "util/concat.h"

namespace lora {

std::map<int, Message::Factory> Message::s_messageFactories;
JoinRequest::Registrar JoinRequest::s_registrar;
JoinAccept::Registrar  JoinAccept::s_registrar;
UnconfirmedDataUp::Registrar UnconfirmedDataUp::s_registrar;
ConfirmedDataUp::Registrar   ConfirmedDataUp::s_registrar;
UnconfirmedDataDown::Registrar UnconfirmedDataDown::s_registrar;
ConfirmedDataDown::Registrar   ConfirmedDataDown::s_registrar;

Packet::Packet()
{
  m_mic = 0;
}

void Packet::setDevice(Device *device)
{
  m_device = device;
  if(m_message) {
    m_message->setDevice(m_device);
  }
}

Packet::Packet(MessageType t)
{
  m_message.reset(Message::create((uint8_t)t));
  if(!m_message) {
    throw std::runtime_error("Message type not supported!");
  }
}

Packet::Packet(const Bytearray &ba)
{
  uint8_t type = ba[0] >> 5;
  m_message.reset(Message::create(type));
  if(!m_message) {
    throw std::runtime_error("Message type not supported!");
  }
  m_message->setPayload(ba);
}

MessageType Packet::type()
{
  if(!m_message) {
    return NONE;
  }

  return m_message->type();
}

bool Packet::verify()
{
  if(!m_message) {
    return false;
  }
  uint32_t mic = m_message->mic();

  if(m_mic == 0) {
    m_mic = mic;
  }

  return m_mic == mic;
}

const Bytearray &
Message::getPayload()
{
  if(m_payload.empty()) {
    compose(&m_device);
  }

  return m_payload;
}

void Message::setDevice(Device *device)
{
  m_device = *device;
  if(!m_payload.empty()) {
    parse(&m_device);
    if(!verify()) {
      throw std::invalid_argument("Wrong MIC for given device");
    }
  }
}

void Message::setPayload(const Bytearray &payload)
{
  m_payload = payload;
  parse(&m_device);
}

void Message::clearPayload()
{
  m_payload.clear();
}

uint32_t Message::mic()
{
  const Bytearray &bytes = getPayload();

  if(bytes.size() < 4) {
    return 0;
  }

  return extract_int(&bytes[bytes.size() - 4]);
}

bool Message::verify() const
{
  if(!m_payload.size()) {
    return true;
  }

  Message *self = const_cast<Message*>(this);
  Bytearray bytes;
  Bytearray &payload = const_cast<Bytearray&>(m_payload);
  uint32_t oldMic = self->mic();

  payload.swap(bytes);
  uint32_t newMic = self->mic();

  payload.swap(bytes);

  return newMic == oldMic;
}

MessageType Message::type()
{
  const Bytearray &bytes = getPayload();

  if(bytes.empty()) {
    return NONE;
  }

  return (MessageType)(bytes[0] >> 5);
}

Message *Message::create(uint8_t type)
{
  if(s_messageFactories.find(type) != s_messageFactories.end()) {
    Message::Factory &factory = s_messageFactories[type];
    return factory();
  }

  return NULL;
}

void Message::registerFactory(uint8_t type, const Factory &factory)
{
  s_messageFactories[type] = factory;
}

}
