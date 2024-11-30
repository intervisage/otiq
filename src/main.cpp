#include <iostream>
#include <SystemUtils.h>
#include <Packet.h>
#include <PcapLiveDeviceList.h>
#include <unistd.h>

#include "otbw.h"
#include "otlog.h"
#include "otpp.h"



// uint64_t min = 1000000;
// uint64_t max = 0;

/**
 * A callback function for the async capture which is called each time a packet is captured
 * TEST FOR GIT
 */
static void onPacketArrives(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *cookie)
{
	otbw::addByteCount(packet->getRawDataLen());

	otpp::processPacket(packet);

	
	
}

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

	// open database
	otdb::open("test.db");

	// IPv4 address of the interface we want to sniff
	// std::string interfaceIPAddr = "192.168.56.10";

	// find the interface by IP address
	// pcpp::PcapLiveDevice *dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIPAddr);
	pcpp::PcapLiveDevice *dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(monInterfaceName);
	if (dev == NULL)
	{
		std::cerr << "Can't find monitoring interface '" << monInterfaceName << "'." << std::endl;
		return 1;
	}

	// start bandwidth calculator
	otbw::start();

	// std::cout
	// 	<< "Interface info:" << std::endl
	// 	<< "   Interface name:        " << dev->getName() << std::endl			 // get interface name
	// 	<< "   Interface description: " << dev->getDesc() << std::endl			 // get interface description
	// 	<< "   MAC address:           " << dev->getMacAddress() << std::endl	 // get interface MAC address
	// 	<< "   Default gateway:       " << dev->getDefaultGateway() << std::endl // get default gateway
	// 	<< "   Interface MTU:         " << dev->getMtu() << std::endl;			 // get interface MTU

	// if (dev->getDnsServers().size() > 0)
	// 	std::cout << "   DNS server:            " << dev->getDnsServers().at(0) << std::endl;

	// start process packet loop thread
	otpp::start();

	// open the device before start capturing/sending packets
	if (!dev->open())
	{
		std::cerr << "Cannot open device" << std::endl;
		return 1;
	}

	otlog::log("MAIN: Starting async traffic capture.");

	// start capture in async mode. Give a callback function to call to whenever a packet is captured and the stats object as the cookie
	// make sure database is open and in memory before this starts
	dev->startCapture(onPacketArrives, NULL);

	// pause main thread
	// pcpp::multiPlatformSleep(30);
	sleep(20);

	// stop capturing packets
	dev->stopCapture();

	otlog::log("MAIN: Async traffic capture stopped.");

	// Stop calculating bandwidth
	otbw::stop();

	//*** otlog::log("MAIN: Packet processing loop stopped.");

	// tidy up process packet thread
	otpp::stop();

	otbw::printBandwidths();

	// Save and close database
	otdb::close();

	return 0;
}
