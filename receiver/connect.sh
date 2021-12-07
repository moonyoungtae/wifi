#!/usr/bin/sudo /bin/bash

ip link set wlp1s0 down
echo "Your MAC address of wlp1s0 temporarily changed to 12:34:56:78:90:ff."
echo "The original MAC address was "$(cat /sys/class/net/wlp1s0/address)" ."
ip link set wlp1s0 address 12:34:56:78:90:ff
ip link set wlp1s0 up

iw wlp1s0 connect Atheros-CSITool

echo "Trying to connect to Injector ..."
while [ "$(iw wlp1s0 info | grep ssid | awk '{print $2}')" != "Atheros-CSITool" ]; do
	iw wlp1s0 connect Atheros-CSITool > /dev/null 2>&1
done
echo "Connected!"

echo "Receiver Configured."
