---
title: LoRa-Py
filename: LICENSE.md
tags:
- LoRaWAN
- Python
- Radio
- RF
- C++
- C
---
# Lora-py

![LoRaPy-Logo](./LoRaPy_Logo.svg "LoRaPy Logo")

Lora-py is a C++ LoraWAN server with Python interface for all operations, including fetching device and session information,
receiving device data, sending data back to device.
lora-py server performs most operations internally, leaving only app logic to Python handlers.

## What it can do

Lora-py allows you to add a device by DevUI+AppUI. Be careful to make device features and class settings match those
on real device, otherwise your device may refuse to connect.

Lora-py allows you to process data sent from device and send your data back to device. For the format of the received 
and sent data please consult the device documentation.

You can write your own handlers in Python and plug it in for your own arbitrary device stack.

## Code

LoRa-Py repository is located at the following URL: [https://github.com/vladvic/lora-py](https://github.com/vladvic/lora-py)

## Building

Building lora-py requires git, Cmake >= 3.5, gcc, python 3+.

```shell
$ git clone https://github.com/vladvic/lora-py.git && cd lora-py
$ mkdir build && cd build
$ cmake .. && make
```

## Running

Just start ./lora-py executable. You should have your python handlers module accessible at python path, or add
the directory in the command line.

The default handlers module name is config (config.py), but you can specify an alternative name on the command
line.


```
Usage: ./lora-server [options]
Options:
  --help,-h                       output this help
  --port,-p <port>                UDP port to listen on; default: 8000
  --dir,-d <directory>            additional python path (allowed multiple separated by :); default: handlers
  --config,-c <name>              python module containing config handlers; default: config
  --test,-t <devEUI:appEUI>       test config script with given devEUI/appEUI; default: 333133376938790c:5345454220312020

Built in Python modules:

  lorawan:
    Types: DeviceInfo, DeviceSession
    Functions:
               send(netid : <Int>, devaddr : <Int>, port : <Int>, data : <Bytearray>, confirm : <Boolean>)
         invalidate(deveui : <String>, appeui : <String>)

    Constants:
               DR0 to DR15, CR4_5 to CR4_8, CLASS_A, CLASS_B, CLASS_C

Python handlers (callbacks):

  ndpd_get_device_info(devEUI : <str>, appEUI : <str>) -> List(<lorawan.DeviceInfo)
  ndpd_load_device_session(deviceAddr : <Int>, networkAddr : <Int>) -> List(<lorawan.DeviceSession>)
  ndpd_save_device_session(session : <lorawan.DeviceSession>)
  ndpd_process_data(session : <lorawan.DeviceSession>, port : <Int>, data : <Bytearray>)

```

## Lora-py Python sample handlers

For sample python handlers please address the following git repository:

[https://github.com/vladvic/lora-py-handlers](https://github.com/vladvic/lora-py-handlers)

## License

LoRa-Py is released under MIT license. You can see the full text [here](./LICENSE.html)
