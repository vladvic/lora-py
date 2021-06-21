/***************************************************
 * udpserver.h
 * Created on Thu, 09 Jul 2020 07:53:24 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>
#include <vector>

#define CHUNK_SIZE  1024

namespace udp {

typedef std::vector<unsigned char> Bytearray;

struct NetworkAddress {
  NetworkAddress(const std::string &addr, int port);
  NetworkAddress(const NetworkAddress &addr) = default;
  NetworkAddress(int port);
  NetworkAddress();
  int family() const;
  int port() const;
  socklen_t addrlen() const;
  void setPort(int port);
  std::string address() const;
  void setAddress(const std::string &addr);
  ::sockaddr &sockaddr();
  const ::sockaddr &sockaddr() const;

private:
  union {
    ::sockaddr      sa;
    sockaddr_in   sin;
    sockaddr_in6  sin6;
  };
};

class UdpServer;

class UdpHandler {
public:
  virtual ~UdpHandler() = default;
  virtual bool messageReceived(UdpServer *srv, const Bytearray &msg, const NetworkAddress &src) = 0;
};

class UdpServer {
public:
  UdpServer(const NetworkAddress &localAddr, UdpHandler *handler);
  int sendTo(const Bytearray &msg, const NetworkAddress &dst);
  int process(int timeoutUs);

private:
  int m_socket;
  NetworkAddress m_localAddr;
  UdpHandler *m_handler;
};

}
