#! /bin/bash

echo "Running test.sh..."
echo "deleting log files..."
rm /home/paul/otiq/build/log*
echo "Running otiq as root..."
echo visage09 | sudo -S /home/paul/otiq/build/otiq enp0s3
echo "wait 2 seconds then run tcpreplay in background"
sleep 2
echo visage09 | tcpreplay -M 20 -i enp0s3 /home/paul/otiq/pcaps/base.pcap &


