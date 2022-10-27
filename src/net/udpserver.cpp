/***************************************************
 * udpserver.cpp
 * Created on Thu, 09 Jul 2020 08:38:11 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <arpa/inet.h>
#include <sys/select.h>
#include <string.h>
#include "udpserver.h"

namespace udp {

NetworkAddress::NetworkAddress(const std::string &addr, int port)
{
  setAddress(addr);
  setPort(port);
}

NetworkAddress::NetworkAddress(int port)
{
  setAddress("0.0.0.0");
  setPort(port);
}

NetworkAddress::NetworkAddress()
{
  setAddress("0.0.0.0");
  setPort(0);
}

int NetworkAddress::family() const
{
  return sa.sa_family;
}

int NetworkAddress::port() const
{
  switch(sa.sa_family) {
  case AF_INET:
    return ntohs(sin.sin_port);
  case AF_INET6:
    return ntohs(sin6.sin6_port);
  }

  return -1;
}

void NetworkAddress::setPort(int port)
{
  switch(sa.sa_family) {
  case AF_INET:
    sin.sin_port = ntohs(port);
  case AF_INET6:
    sin6.sin6_port = ntohs(port);
  }
}

std::string NetworkAddress::address() const
{
  char buffer[INET6_ADDRSTRLEN + 1] = {'\0'};

  switch(sa.sa_family) {
  case AF_INET:
    inet_ntop(AF_INET, (void*)&sin.sin_addr, buffer, sizeof(buffer));
    break;
  case AF_INET6:
    inet_ntop(AF_INET6, (void*)&sin6.sin6_addr, buffer, sizeof(buffer));
    break;
  }

  return buffer;
}

socklen_t NetworkAddress::addrlen() const
{
  switch(sa.sa_family) {
  case AF_INET:
    return sizeof(sin);
  case AF_INET6:
    return sizeof(sin6);
  default:
    abort(); // Impossible
  }

  return 0; // No warnings
}

void NetworkAddress::setAddress(const std::string &addr)
{
  int family = AF_INET;

  if(strchr(addr.c_str(), ':') != NULL) {
    family = AF_INET6;
  }

  sa.sa_family = family;

  switch(family) {
  case AF_INET:
    inet_pton(AF_INET, addr.c_str(), (void*)&sin.sin_addr);
    break;
  case AF_INET6:
    inet_pton(AF_INET6, addr.c_str(), (void*)&sin6.sin6_addr);
    break;
  }
}

::sockaddr &NetworkAddress::sockaddr()
{
  return sa;
}

const ::sockaddr &NetworkAddress::sockaddr() const
{
  return sa;
}


UdpServer::UdpServer(const NetworkAddress &localAddr, UdpHandler *handler)
  : m_localAddr(localAddr)
  , m_handler(handler)
{
  m_socket = socket(m_localAddr.family(), SOCK_DGRAM, IPPROTO_UDP);
  if(m_socket < 0) {
    abort();
  }
  if(bind(m_socket, &m_localAddr.sockaddr(), m_localAddr.addrlen()) < 0) {
    abort();
  }
}

int UdpServer::process(int timeoutUs)
{
  Bytearray buffer;
  NetworkAddress addr;
  sockaddr &saddr = addr.sockaddr();
  struct timeval ts;
  unsigned long start = 0;
  int bytes = 0;
  fd_set s;

  FD_ZERO(&s);
  FD_SET(m_socket, &s);

  ts.tv_sec = timeoutUs / 1000000;
  ts.tv_usec = timeoutUs % 1000000;

  int res = select(m_socket + 1, &s, NULL, NULL, &ts);

  if(res < 0) {
    return -1;
  }

  if(res == 0) {
    m_handler->selectTimeout(this);
    return 0;
  }

  do {
    socklen_t addrlen = sizeof(saddr);
    buffer.resize(buffer.size() + CHUNK_SIZE);
    bytes = recvfrom(m_socket, &buffer[start], CHUNK_SIZE, 0, &saddr, &addrlen);

    if(bytes < 0) {
      return bytes;
    }

    start += bytes;
  } while(bytes == CHUNK_SIZE);

  buffer.resize(start);

  m_handler->messageReceived(this, buffer, addr);
  return buffer.size();
}

int UdpServer::sendTo(const Bytearray &msg, const NetworkAddress &dst)
{
  return sendto(m_socket, &msg[0], msg.size(), 0, &dst.sockaddr(), dst.addrlen());
}

}

#ifdef TEST
#include <iostream>

class Handler
  : public udp::UdpHandler
{
public:
  virtual bool 
  messageReceived(udp::UdpServer *srv, const udp::Bytearray &msg, const udp::NetworkAddress &src)
  {
    std::cout << "Got packet from " << src.address() << ":" << src.port() << std::endl;
    std::cout << std::string(&msg[0], msg.size()) << std::endl;
  }
};

int main() {
  Handler handler;
  udp::NetworkAddress addr(8001);
  udp::UdpServer server(addr, &handler);

  while(server.process(0) >= 0) { }
}

#endif
