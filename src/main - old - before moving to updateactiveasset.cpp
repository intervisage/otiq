#include <iostream>
#include <IPv4Layer.h>
#include <EthLayer.h>
#include <SystemUtils.h>
#include <Packet.h>
#include <PcapLiveDeviceList.h>
#include <sstream>
#include <iomanip>

#include "otdb.h"
#include "otbw.h"
#include "otlog.h"

/* Adds asset to database if not alreaded added, otherwise updates asset, including time stamps
ipv4 - IP V4 Address as string value
macAddress  - optional associated MAC address as string value
macInfo - enumerated value indicating how MAC address was determined
 */

void updateActiveAsset(std::string ipv4Addr, std::string macAddr = "", otdb::MacInfo macInfo = otdb::none)
{
	std::string queryString; // used in queries
	int rc;					 // return values from queries

	// get epoch time
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
	std::uint64_t timestamp = ms.count();
	std::string timestampString = std::to_string(timestamp);

	// If asset not in table  - then add and set first and last activity time
	queryString = "SELECT IP, FIRST_ACTIVITY FROM ASSETS WHERE IP ='" + ipv4Addr + "';";
	otdb::query(queryString);

	if (otdb::rowCount() == 0)
	{

		// // asset is not in table so insert
		queryString = "INSERT INTO ASSETS (IP, FIRST_ACTIVITY, LAST_ACTIVITY)"
					  "VALUES (\"" +
					  ipv4Addr + "\", " + timestampString + ", " + timestampString;

		// add MAC data if passed
		if (macInfo != otdb::none)
		{
			queryString += ", MAC = json('{";
			"addr:\"" + macAddr + "\"";
			", info:\"" + otdb::getMacInfoString(macInfo) + "\"";
			", time:\"" + timestampString + "\"";
			"}')";
		}

		// complete query string values.
		queryString += ");";
	}
	else
	{
		std::string firstActivityTime = otdb::resultValue(0, "FIRST_ACTIVITY");

		// if asset exists but no first activity time set - then asset was originaly seen as a destination
		// so we now set the first and last activity time sbased on timestamp

		if (firstActivityTime == "0")
		{
			queryString = "UPDATE ASSETS SET FIRST_ACTIVITY= " + timestampString + ", LAST_ACTIVITY = " + timestampString;
		}
		else
		{

			// if asset is in table bu there is a first activity time, then we have already seen the asset transmitting - so just update the last activity time.
			queryString = "UPDATE ASSETS SET LAST_ACTIVITY = " + timestampString;
		}

		// Add MAC field if mac info supplied
		if (macInfo != otdb::none)
		{
			queryString += ", MAC = json_insert(MAC, '$[#]',json('{";
			"addr:\"" + macAddr + "\"";
			", info:\"" + otdb::getMacInfoString(macInfo) + "\"";
			", time:\"" + timestampString + "\"";
			"}'))";
		}

		// finlaise query string with 'where' clause
		queryString += " WHERE IP = '" + ipv4Addr + "';";
	}
	rc = otdb::query(queryString);
}

/**
 * A callback function for the async capture which is called each time a packet is captured
 */
static void onPacketArrives(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *cookie)
{
	std::string queryString = "";
	char *error_message = 0;
	int rc = 0;

	// update bandwidth calculator with packet length
	otbw::addByteCount(packet->getRawDataLen());

	// parse raw packet
	pcpp::Packet parsedPacket(packet);

	// check if arp message
	// auto *arpLayer = parsedPacket.getLayerOfType<pcpp::ArpLayer>();
	// if (arpLayer != nullptr)
	// {

	// 	// update asset with MAC address if IP address is already in asset table
	// 	otlog::log("MAIN: ARP message detected");

	// 	std::string senderIPAddr = arpLayer->getSenderIpAddr().toString();
	// 	std::string macAddr = arpLayer->getSenderMacAddress().toString();

	// 	updateActiveAsset(senderIPAddr, macAddr, otdb::arp);

	// 	// no further processing of ARP message
	// 	return;
	// }

	// check if IPv4
	pcpp::IPv4Layer *ipv4Layer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();
	if (parsedPacket.isPacketOfType(pcpp::IPv4))
	{
		pcpp::IPv4Layer *ipv4Layer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();

		// get source address and check if already in database
		std::string srcAddr = ipv4Layer->getSrcIPv4Address().toString();
		std::string dstAddr = ipv4Layer->getDstIPv4Address().toString();


		updateActiveAsset(srcAddr);


		// // get epoch time
		// auto now = std::chrono::system_clock::now();
		// auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
		// std::uint64_t timestamp = ms.count();
		// std::string timestampString = std::to_string(timestamp);

		
		// queryString = "SELECT IP, FIRST_ACTIVITY FROM ASSETS WHERE IP =\"" + srcAddr + "\"";
		// otdb::query(queryString);

		// if (otdb::rowCount() == 0)
		// {

		// 	// If not in table - then add and set first and last activity time
		// 	queryString = "INSERT INTO ASSETS (IP,FIRST_ACTIVITY, LAST_ACTIVITY) VALUES (\"" + srcAddr + "\",";
		// 	queryString += timestampString + ",";
		// 	queryString += timestampString + ")";
		// 	rc = otdb::query(queryString);
		// }
		// else
		// {
		// 	std::string firstActivityTime = otdb::resultValue(0, "FIRST_ACTIVITY");

		// 	// if asset exists but no first activity time set - then asset was originaly seen as a destination
		// 	// so we now set the first and last activity time sbased on timestamp

		// 	if (firstActivityTime == "0")
		// 	{
		// 		queryString = "UPDATE ASSETS SET FIRST_ACTIVITY=" + timestampString + ", LAST_ACTIVITY =" + timestampString;
		// 		queryString += " WHERE IP = \"" + srcAddr + "\";";
		// 	}
		// 	else
		// 	{

		// 		// if asset is in table bu there is a first activity time, then we have already seen the asset transmitting - so just update the last activity time.
		// 		queryString = "UPDATE ASSETS SET LAST_ACTIVITY =" + timestampString;
		// 		queryString += " WHERE IP = \"" + srcAddr + "\";";
		// 	}
		// 	otdb::query(queryString);
		// }

		// // add destination address with no time stamp if not already in table
		queryString = "INSERT OR IGNORE INTO ASSETS (IP) VALUES (\"" + dstAddr + "\");";
		rc = otdb::query(queryString);
	}
}

int main(int argc, char *argv[])
{

	otbw::start();

	int rc; // return values

	// std::string sql;
	// char *error_message = 0;

	// open database
	otdb::open("test.db");

	// IPv4 address of the interface we want to sniff
	std::string interfaceIPAddr = "192.168.56.10";

	// find the interface by IP address
	pcpp::PcapLiveDevice *dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIPAddr);
	if (dev == NULL)
	{
		std::cerr << "Cannot find interface with IPv4 address of '" << interfaceIPAddr << "'" << std::endl;
		return 1;
	}

	// std::cout
	// 	<< "Interface info:" << std::endl
	// 	<< "   Interface name:        " << dev->getName() << std::endl			 // get interface name
	// 	<< "   Interface description: " << dev->getDesc() << std::endl			 // get interface description
	// 	<< "   MAC address:           " << dev->getMacAddress() << std::endl	 // get interface MAC address
	// 	<< "   Default gateway:       " << dev->getDefaultGateway() << std::endl // get default gateway
	// 	<< "   Interface MTU:         " << dev->getMtu() << std::endl;			 // get interface MTU

	// if (dev->getDnsServers().size() > 0)
	// 	std::cout << "   DNS server:            " << dev->getDnsServers().at(0) << std::endl;

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
	pcpp::multiPlatformSleep(30);

	// stop capturing packets
	dev->stopCapture();

	otlog::log("MAIN: Async traffic capture stopped.");

	// Stop calculating bandwidth
	otbw::stop();

	otbw::printBandwidths();

	// Save and close database
	otdb::close();

	return 0;
}
