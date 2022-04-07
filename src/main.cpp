/***************************************************
 * main.cpp
 * Created on Thu, 16 Jul 2020 10:11:25 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "net/udpserver.h"
#include "semtech/proto.h"
#include "apphandler.h"

void outputHelp(const char *what) {
  printf("Usage: %s [options]\n", what);
  printf("Options:\n");
  printf("--port,-p <port>                UDP port to listen on\n");
  printf("--dir,-d <directory>            additional python path (multiple allowed)\n");
  printf("--config,-c <name>              python module containing config handlers\n");
  printf("--test,-t <devEUI:appEUI>       test config script with given devEUI/appEUI\n");
  printf("\n");
  printf("Built in Python modules:\n");
  printf("\n");
  printf("  lorawan:\n");
  printf("    Types: DeviceInfo, DeviceSession\n");
  printf("    Functions: send(netid : <Int>, devaddr : <Int>, port : <Int>, data : <Bytearray>, confirm : <Boolean>)\n");
  printf("    Constants: DR0 to DR15, CR4_5 to CR4_8, CLASS_A, CLASS_B, CLASS_C\n");
  printf("\n");
  printf("Python handlers:\n");
  printf("\n");
  printf("  ndpd_get_device_info(devEUI : <str>, appEUI : <str>) -> List(<lorawan.DeviceInfo)\n");
  printf("  ndpd_get_device_session(deviceAddr : <Int>, networkAddr : <Int>) -> List(<lorawan.DeviceSession>)\n");
  printf("  ndpd_save_device_session(session : <lorawan.DeviceSession>)\n");
  printf("  ndpd_process_data(session : <lorawan.DeviceSession>, port : <Int>, data : <Bytearray>)\n");
}

int main(int argc, char *const *argv) {
  std::string base("handlers"), config("config");
  std::string dev("333133376938790c");
  std::string app("5345454220312020");
  int port = 8000;
  bool test = false;

  static struct option long_options[] = {
    /* These options don’t set a flag.
       We distinguish them by their indices. */
    {"dir",    1, 0, 'd'}, // Additional python path
    {"config", 1, 0, 'c'}, // Configuration module name
    {"port", 1, 0,   'p'}, // Port
    {"test", 1, 0,   't'}, // Test config script
    {0, 0, 0, 0}
  };  
  int option_index = 0;

  optind = 1;

  while(1) {
    int c = getopt_long(argc, argv, "hd:c:p:t:", long_options, &option_index);

    if(c == -1) 
      break;

    switch(c) {
    case 'd':
      if (base != "") {
        base = base + ":";
      }
      base = base + optarg;
      break;
    case 'c':
      config = optarg;
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 't':
      {
      char *div = strchrnul(optarg, ':');
      dev = std::string(optarg, div - optarg);
      if(*div != 0) {
        ++ div;
        app = std::string(div);
      }
      test = true;
      }
      break;
    case 'h':
      outputHelp(argv[0]);
      return 0;
    case '?':
      outputHelp(argv[0]);
      return -1;
    }
  }

  srand(time(NULL));
  AppHandler apphandler;

  if(test) {
    lora::EUI devEUI, appEUI;
    lora::stringToHex(dev, devEUI, sizeof(devEUI));
    lora::stringToHex(app, appEUI, sizeof(appEUI));
    std::cout << "|======================================================================|" << std::endl;
    std::cout << "| Testing config script with device " << dev << ":" << app << "  |" << std::endl;
    std::cout << "|======================================================================|" << std::endl;

    auto lst = apphandler.findDeviceInfo(devEUI, appEUI);
    for(auto dev : lst) {
      std::string de = lora::hexToString(dev->devEUI, sizeof(dev->devEUI));
      std::string ae = lora::hexToString(dev->appEUI, sizeof(dev->appEUI));
      std::cout << "  |======================================================================|" << std::endl;
      std::cout << "  | Found device (DevEUI=" << de << ", AppEUI=" << ae << ")" << std::endl;
      std::cout << "  |======================================================================|" << std::endl;
      std::cout << "    Device " << dev << " (DevEUI=" << de << ", AppEUI=" << ae << ")" << std::endl;
      std::cout << "    Device session " << dev->session << " (networkId=" << dev->session->networkId << ", deviceAddr=" << dev->session->deviceAddr << ")" << std::endl;
      std::cout << "    Back referred session device: " << dev->session->device << (dev->session->device == dev ? " (correct)" : " (incorrect)") << std::endl;
      std::cout << "  - Saving device session..." << std::endl;
      apphandler.saveDeviceSession(dev->session);
      std::cout << "    Done" << std::endl;
      lora::Bytearray data;
      for(int i = 0; i < 32; ++ i) {
        data.push_back(i);
      }
      std::cout << "  - Calling data handler..." << std::endl;
      apphandler.processAppData(dev->session, 1, data);
      std::cout << "    Done" << std::endl;
    }
    return 0;
  }

  semtech::ProtoHandler handler(&apphandler);
  udp::NetworkAddress addr(port);
  udp::UdpServer server(addr, &handler);

  while(server.process(1000) >= 0) {
    handler.processSendQueue();
  }
}

