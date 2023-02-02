# lora-py

lora-py is a C++ LoraWAN server with Python interface for all operations, including fetching device and session information,
receiving device data, sending data back to device.
lora-py server performs most operations internally, leaving only app logic to Python handlers.

# TODO

Timeout gateways if it's not refreshed by periodical updates

Add ability to refresh devices from the database (i.e. if a device is already cached, but have changed since then)

# Lora-py Python handlers

https://github.com/vladvic/lora-py-handlers
