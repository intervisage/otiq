#!/bin/sh
exec tcpreplay -M 1 -i ens37 ./simple_modbus.pcap
