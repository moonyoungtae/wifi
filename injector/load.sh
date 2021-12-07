#!/usr/bin/sudo /bin/bash

print_help() {
	echo "Usage   : ./load.sh CHANNEL_INDEX [MCS_INDEX] [+|-]"
	echo "Example : ./load.sh 1 0 -"
	echo "Example : ./load.sh 1 31"
	echo "Example : ./load.sh 1 +"
	echo "Example : ./load.sh 6"
	echo
	echo "CHANNEL_INDEX"
	echo "  802.11 Channel Index."
	echo
	echo "MCS_INDEX"
	echo "  HT MCS Index. Auto mode if unset."
	echo
	echo "HT_MODE"
	echo "  Use HT20 or HT40. HT20 will used if unset."
	exit
}

if [ "$1" == "--help" ]; then
	print_help
elif [ "$1" == "-h" ]; then
	print_help
elif [ "$1" == "help" ]; then
	print_help
elif [ "$1" == "" ]; then
	echo "Set channel, please!"
	print_help
else
	cp ./hostapd.default.conf ./hostapd.temp.conf
	echo "channel="$1 >> ./hostapd.temp.conf
fi

if [ "$2" == "" ]; then
	echo "ht_capab=[SHORT-GI-20][HT20]" >> ./hostapd.temp.conf
elif [ "$2" == "+" ]; then
	echo "ht_capab=[SHORT-GI-40][HT40+]" >> ./hostapd.temp.conf
elif [ "$2" == "-" ]; then
	echo "ht_capab=[SHORT-GI-40][HT40-]" >> ./hostapd.temp.conf
else
	MCS_INDEX=$(echo $2 | tr -dc "0-9")
	if [ "$MCS_INDEX" == "" ]; then
		echo "Incorrect HT mode character!"
		rm ./hostapd.temp.conf
		print_help
	elif [ "$MCS_INDEX" -gt "32" ]; then
		echo "MCS Index is too large."
		rm ./hostapd.temp.conf
		print_help
	elif [ "$MCS_INDEX" -lt "0" ]; then
		echo "MCS Index is too small."
		rm ./hostapd.temp.conf
		print_help
	else
		echo "beacon_rate=ht:$MCS_INDEX" >> ./hostapd.temp.conf
		if [ "$3" == "" ]; then
			echo "ht_capab=[SHORT-GI-20][HT20]" >> ./hostapd.temp.conf
		elif [ "$3" == "+" ]; then
			echo "ht_capab=[SHORT-GI-40][HT40+]" >> ./hostapd.temp.conf
		elif [ "$3" == "-" ]; then
			echo "ht_capab=[SHORT-GI-40][HT40-]" >> ./hostapd.temp.conf
		else
			echo "Incorrect HT mode character!"
			rm ./hostapd.temp.conf
			print_help
		fi
	fi
fi

ip addr add 10.5.19.1/24 dev wlp1s0 > /dev/null 2>&1
systemctl stop isc-dhcp-server > /dev/null 2>&1
dhcpd -cf ./dhcpd.conf
./hostap/hostapd/hostapd -d ./hostapd.temp.conf -B

#rm ./hostapd.temp.conf

echo "Injector Configured."
