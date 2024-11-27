#!/bin/sh
exec tcpreplay -l 0 -M 45  -i enp0s3 ./base.pcap

