#!/bin/bash

#a simple script for natting you wifi to your ethernet

internet=eth1
shared=eth0

echo 1 > /proc/sys/net/ipv4/ip_forward
/sbin/iptables -t nat -A POSTROUTING -o $internet -j MASQUERADE
/sbin/iptables -A FORWARD -i $internet -o $shared -m state --state RELATED,ESTABLISHED -j ACCEPT
/sbin/iptables -A FORWARD -i $shared -o $internet -j ACCEPT
until 
ping 192.168.0.100 -c 2 > /dev/null
do
echo "ping failed: upping interface"
ifconfig $shared 192.168.0.1/24 up
done

