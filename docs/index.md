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

## Building

Building lora-py requires Cmake >= 3.5, gcc, python 3+.

```shell
\$ mkdir build && cd build
\$ cmake .. && make
```

## Lora-py Python sample handlers

For sample python handlers please address the following git repository:

https://github.com/vladvic/lora-py-handlers

