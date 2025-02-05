#include <iostream>
#include <SystemUtils.h>
#include <PcapLiveDeviceList.h>
#include <unistd.h>

#include "otbw.h"
#include "otlog.h"
#include "otdb.h"
#include "otpp.h"


int main(int argc, char *argv[])
{

	// check if name of interface was passed
	if (argc != 2)
	{
		std::cout << "Please provide namde of a valid monitoring interface." << std::endl;
		return -1;
	}

	std::string monInterfaceName = argv[1];

	int rc; // return values

	// remove previous logs *** TODO - remove this
	otlog::deleteAll();

	std::string databaseFile = "test.db";

	// open database
	if (otdb::open(databaseFile) != 0)
	{
		otlog::log("MAIN: Could not open database file ( " + databaseFile + " )");
		return 1;
	}

	// IPv4 address of the interface we want to sniff
	// std::string interfaceIPAddr = "192.168.56.10";

	// find the interface by IP address
	// pcpp::PcapLiveDevice *dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIPAddr);

	pcpp::PcapLiveDevice *dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(monInterfaceName);
	if (dev == NULL)
	{

		otlog::log("MAIN: Could not find monitoring interface ( " + monInterfaceName + " )");
		std::cout << "Can't find monitoring interface '" << monInterfaceName << "'." << std::endl;
		return 1;
	}



	// open the device before start capturing/sending packets
	if (!dev->open())
	{
		otlog::log("MAIN: Could not open interface for monitorin ( " + monInterfaceName + " )");
		std::cout << "Cannot open device" << std::endl;
		return 1;
	}

	// start bandwidth calculator
	otbw::start();

	// start packet capture and processing
	otpp::start(dev);

	// pause main thread
	sleep(600);

	// stop packet capture and processing
	otpp::stop(dev);

	// Stop calculating bandwidth
	otbw::stop();

	// std::cout << std::endl
	//  		  << std::endl;
	// otbw::printBandwidths();

	// Save and close database
	otdb::close();

	return 0;
}
