
#include <chrono>
#include <Packet.h>
#include <IPv4Layer.h>
#include <EthLayer.h>
#include <ArpLayer.h>
#include <SystemUtils.h>
#include <sstream>
#include <thread>
#include <mutex>
#include <pthread.h>
#include <condition_variable>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <atomic>
#include <deque>
#include <map>
#include <Packet.h>
#include <PcapLiveDeviceList.h>
#include <sstream>

#include "otpp.h"
#include "otbw.h"
#include "otlog.h"
#include "otdb.h"
#include "otpbuff.h"
#include "otassets.h"

void processPacket();

int updateActiveAsset(std::string ipv4Addr, std::string macAddr = "", otdb::MacInfo macInfo = otdb::none);

int updateInactiveAsset(std::string ipv4Addr);

PacketBuffer pBuffer;



std::thread processPacketThreadPtr_1;
std::thread processPacketThreadPtr_2;
std::thread processPacketThreadPtr_3;
std::thread processPacketThreadPtr_4;

pcpp::RawPacket *receivedRawPacket;
std::mutex parserMutex;
bool terminateThread;
AssetList assetList;

namespace otpp
{

	void start(pcpp::PcapLiveDevice *dev)
	{

		terminateThread = false;
		//receivedRawPacket = nullptr;

		// start packet capture thread
		otlog::log("OTPP: Starting async traffic capture.");
		dev->startCapture(otpp::onPacketArrives, &pBuffer);

		// start packet consumer thread 1
		processPacketThreadPtr_1 = std::thread(processPacket);
		std::string threadName1 = "otiq-otpp-1";
		pthread_setname_np(processPacketThreadPtr_1.native_handle(), threadName1.c_str());

		// // start packet consumer thread 2
		processPacketThreadPtr_2 = std::thread(processPacket);
		std::string threadName2 = "otiq-otpp-2";
		pthread_setname_np(processPacketThreadPtr_2.native_handle(), threadName2.c_str());

		// start packet consumer thread 3
		processPacketThreadPtr_3 = std::thread(processPacket);
		std::string threadName3 = "otiq-otpp-3";
		pthread_setname_np(processPacketThreadPtr_3.native_handle(), threadName3.c_str());

		// start packet consumer thread 3
		processPacketThreadPtr_4 = std::thread(processPacket);
		std::string threadName4 = "otiq-otpp-4";
		pthread_setname_np(processPacketThreadPtr_4.native_handle(), threadName4.c_str());
	}

	void stop(pcpp::PcapLiveDevice *dev)
	{

		// stop packet capture thread
		dev->stopCapture();
		otlog::log("OTPP: Async traffic capture stopped.");

		// signal processing thread to stop
		terminateThread = true;

		// clear buffer of remaining packets
		pBuffer.clearBuffer();

		// wait for thread to return
		processPacketThreadPtr_1.join();
		processPacketThreadPtr_2.join();
		processPacketThreadPtr_3.join();
		processPacketThreadPtr_4.join();

		otlog::log("OTPP: Process Loops terminated.");

		// // clear buffer - this will delete packet instances through use of unique_ptrs
		if (pBuffer.getSize() != 0)
		{
			otlog::log("OTPP: WARNING - Packets left in buffer!!!  " + std::to_string(pBuffer.getSize()));
		}
		else
		{
			otlog::log("OTPP: No remaining packets in buffer.");
		}

		// output stats
		float avgDrop = static_cast<float>(pBuffer.getDropCountPush()) * 100 / (pBuffer.getDropCountPush() + pBuffer.getPushCount());
		float buffPercent = static_cast<float>(pBuffer.getMaxQueueSize()) * 100 /pBuffer.getPushCount();
		std::cout << "Maximum buffer size = " << std::to_string(pBuffer.getMaxQueueSize()) << std::endl;
		std::cout << "Maximum buffer size percentage of Push = " << std::to_string(buffPercent) << std::endl;
		std::cout << "Push Count = " << std::to_string(pBuffer.getPushCount()) << std::endl;
		std::cout << "Pop Count = " << std::to_string(pBuffer.getPopCount()) << std::endl;
		std::cout << "Drop Packet Count Push = " << std::to_string(pBuffer.getDropCountPush()) << std::endl;
		std::cout << "Drop Packet Percentage = " << std::to_string(avgDrop) << std::endl;
		std::cout << "Asset list size = " << std::to_string(assetList.getSize()) << std::endl;
	}

	// Note that onPacketArrives is called from a separate PCapPlusPlus thread - so need to pass pointer to buffer
	void onPacketArrives(pcpp::RawPacket *rawPacket, pcpp::PcapLiveDevice *dev, void *userData)
	{

		// pass raw packet length to otbw to calculate bandwidth
		otbw::addByteCount(rawPacket->getRawDataLen());

		// create new raw packet from passed raw packet and push associated pointer into buffer
		pcpp::RawPacket* newRawPacket = new pcpp::RawPacket(*rawPacket);
		PacketBuffer* pBuffer = reinterpret_cast<PacketBuffer *>(userData);
		pBuffer->addPacket(newRawPacket);
	}
}

void processPacket()
{
	while (!terminateThread || pBuffer.hasPackets())
	{

		pcpp::RawPacket *rawPacket = pBuffer.getPacket();

		if (rawPacket != nullptr)
		{

			pcpp::Packet *packet = new pcpp::Packet(rawPacket);

			// check if valid eternet II packet and stop processing if not
			// TODO - Add Support for IEEE 802.3 ?????????????????????????????????
			// pcpp::EthLayer* ethernetLayer = packet->getLayerOfType<pcpp::EthLayer>();
			// if (ethernetLayer == nullptr)
			// {
			// 	// no futher processing required
			// 	delete packet;
			// 	delete rawPacket;
			// 	break;
			// }

			// 			// // check if arp message
			// auto *arpLayer = packet->getLayerOfType<pcpp::ArpLayer>();
			// if (arpLayer != nullptr)
			// {
			// 	// note that sender is used by host requesting MAC address  and host responding with Mac address
			// 	// updateActiveAsset(arpLayer->getSenderIpAddr().toString(), arpLayer->getSenderMacAddress().toString(), otdb::arp);

			// 	// no further processing of ARP message

			// 	delete packet;
			// 	delete rawPacket;
			// 	break;
			// }

			// process ip layer
			if (packet->isPacketOfType(pcpp::IPv4))
			{
				pcpp::IPv4Layer *ipv4Layer = packet->getLayerOfType<pcpp::IPv4Layer>();
				assetList.addAsset(ipv4Layer->getSrcIPv4Address().toInt(), otassets::assetDetails());
			}

			/* Clean up memory*/
			delete packet;
			delete rawPacket;
		}
	}
}

// Called from main to process packets as they arrive

// 	while (!terminateThread)
// 	{
// 		pcpp::RawPacket *recivedRawPacket = nullptr;

// 		while (!pBuffer.isEmpty())
// 		{
// 			// receivedRawPacket = pBuffer.dequeue();

// 			/* show which thread has popped */
// 			std::ostringstream threadID;
// 			threadID << std::this_thread::get_id();
// 			std::cout << receivedRawPacket << " receivedRawPacket Pointer, Thread = " << threadID.str() << std::endl;

// 			// std::cout << " terminateThread = " << std::to_string(terminateThread) << ", buffer size = " << std::to_string(pBuffer.size()) << std::endl;

// 			popCount++;

// 			auto start = std::chrono::high_resolution_clock::now();

// 			// create parsed packet with all layers. Note - freeRawPacket is set to true to raw packet is deleted when parsedPacket is deleted on out of scope
// 			pcpp::Packet parsedPacket(receivedRawPacket);

// 			// process thread
// 			std::string queryString = "";
// 			char *error_message = 0;
// 			int rc = 0;

// 			// parse raw packet

// 			pcpp::EthLayer *ethernetLayer = parsedPacket.getLayerOfType<pcpp::EthLayer>();

// 			// check if valid eternet II packet and stop processing if not
// 			// TODO - Add Support for IEEE 802.3 ?????????????????????????????????

// 			// if (ethernetLayer == nullptr)
// 			// {
// 			// 	// no futher processing required - so free up raw packet and continue
// 			// 	receivedRawPacket->clear();
// 			// 	std::cout << "HERE!!!!" << std::endl;
// 			// 	break;
// 			// }

// 			// don't process communications between MacBook and Virtual Machine
// 			// pcpp::MacAddress ubuntuMacAddress = pcpp::MacAddress("08:00:27:54:a5:55");
// 			// pcpp::MacAddress macBookMacAddress = pcpp::MacAddress("3a:f9:d3:16:91:64");
// 			// pcpp::MacAddress srcMacAddr = ethernetLayer->getSourceMac();
// 			// pcpp::MacAddress dstMacAddr = ethernetLayer->getDestMac();

// 			// if (srcMacAddr == macBookMacAddress || dstMacAddr == macBookMacAddress || srcMacAddr == ubuntuMacAddress || dstMacAddr == ubuntuMacAddress)
// 			// {
// 			// 	runThread = false;
// 			// 	continue;
// 			// }

// 			// // check if arp message
// 			// auto *arpLayer = parsedPacket.getLayerOfType<pcpp::ArpLayer>();
// 			// if (arpLayer != nullptr)
// 			// {
// 			// 	// note that sender is used by host requesting MAC address  and host responding with Mac address
// 			// 	//updateActiveAsset(arpLayer->getSenderIpAddr().toString(), arpLayer->getSenderMacAddress().toString(), otdb::arp);

// 			// 	// no further processing of ARP message
// 			// 	continue;
// 			// }

// 			// check if IPv4
// 			pcpp::IPv4Layer *ipv4Layer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();

// 			if (parsedPacket.isPacketOfType(pcpp::IPv4))
// 			{
// 				pcpp::IPv4Layer *ipv4Layer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();
// 				uint64_t ipv4AddrInt = ipv4Layer->getSrcIPv4Address().toInt();
// 				if (AssetList.find(ipv4AddrInt) == AssetList.end())
// 				{
// 					// no entry for this ip address exists so add ip address and empty
// 					AssetList.insert({ipv4AddrInt, new assetStruct()});
// 				}

// 				// get sources address as string
// 				// updateActiveAsset(ipv4Layer->getSrcIPv4Address().toString());

// 				// updateInactiveAsset(ipv4Layer->getDstIPv4Address().toString());
// 			}

// 			/* EXECUTION TIMING */

// 			auto stop = std::chrono::high_resolution_clock::now();

// 			uint64_t timeTaken = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

// 			totalExecutionTime += timeTaken;

// 			// update process time metrics
// 			if (timeTaken < minExTime)
// 			{
// 				minExTime = timeTaken;
// 			}

// 			if (timeTaken > maxExTime)
// 			{
// 				maxExTime = timeTaken;
// 			}

// 			// get rid of instance
// 			pBuffer.clear(receivedRawPacket);
// 		}
// 	}
// }

/* Adds asset to database if not alreaded added, otherwise updates asset, including time stamps
ipv4 - IP V4 Address as string value
macAddress  - optional associated MAC address as string value
macInfo - enumerated value indicating how MAC address was determined
 */
int updateActiveAsset(std::string ipv4Addr, std::string macAddr, otdb::MacInfo macInfo)
{

	std::string queryString; // used in queries
	int rc;					 // return values from queries

	// get epoch time
	// auto now = std::chrono::system_clock::now();
	// auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
	// std::uint64_t timestamp = ms.count();
	// std::string timestampString = std::to_string(timestamp);

	// queryString = "INSERT OR IGNORE INTO ASSETS (IP) ";
	// queryString += " VALUES ('" + ipv4Addr + "');";
	// return otdb::query(queryString);

	/* check if asset is already in database - Concentrate on IP columns  - as this query needs to
	super efficinet  - will be called on every packet!!!!*/

	queryString = "SELECT EXISTS(SELECT 1 FROM ASSETS WHERE IP = '" + ipv4Addr + "' );";
	otdb::query(queryString);

	if (otdb::resultValue(0, 0) == "0")
	{

		// 	// // asset is not in table so insert
		queryString = "INSERT INTO ASSETS (IP)";
		// queryString += " VALUES (\"" + ipv4Addr + "\", " + timestampString + ", " + timestampString;
		queryString += " VALUES (\"" + ipv4Addr + "\");";
	}
	// 	// add MAC data if passed
	// 	// if (macInfo != otdb::none)
	// 	// {
	// 	// 	queryString += ", json('[{"
	// 	// 				   "addr:\"" +
	// 	// 				   macAddr + "\""
	// 	// 							 ", info:\"" +
	// 	// 				   otdb::getMacInfoString(macInfo) + "\""
	// 	// 													 ", time:\"" +
	// 	// 				   timestampString + "\""
	// 	// 									 "}]')";
	//	// }
	// 	// 	else
	// 	// 	{

	// 	// 		// otherwise just add empty json array
	// 	// 		queryString += ", json('[]')";
	// 	// }

	// 	// 	// complete query string values.
	// 	// 	queryString += ");";
	// }
	// else
	// {
	// 	std::string firstActivityTime = otdb::resultValue(0, "FIRST_ACTIVITY");

	// 	// if asset exists but no first activity time set - then asset was originaly seen as a destination
	// 	// so we now set the first and last activity time sbased on timestamp

	// 	if (firstActivityTime == "0")
	// 	{
	// 		queryString = "UPDATE ASSETS SET FIRST_ACTIVITY= " + timestampString + ", LAST_ACTIVITY = " + timestampString;
	// 	}
	// 	else
	// 	{

	// 		// if asset is in table but there is a first activity time, then we have already seen the asset transmitting - update the last activity time but not first activity time
	// 		queryString = "UPDATE ASSETS SET LAST_ACTIVITY = " + timestampString;
	// 	}

	// 	// // Add MAC field if mac info supplied
	// 	if (macInfo != otdb::none)
	// 	{
	// 		queryString += ", MAC = json_insert(MAC, '$[#]',json('{"
	// 					   "addr:\"" +
	// 					   macAddr + "\""
	// 								 ", info:\"" +
	// 					   otdb::getMacInfoString(macInfo) + "\""
	// 														 ", time:\"" +
	// 					   timestampString + "\""
	// 										 "}'))";
	// 	}
	// 	else
	// 	{
	// 		// otherwise create empty json array unless there is already MAC info.
	// 		std::string macData = otdb::resultValue(0, "MAC");
	// 		if (macData == "")
	// 		{
	// 			queryString += ", MAC = json('[]')";
	// 		}
	// 	}

	// 	// // finalise query string with 'where' clause
	// 	queryString += " WHERE IP = '" + ipv4Addr + "';";
	// }

	return otdb::query(queryString);
}

/* Adds inactive asset (typically asset receiving message ie destination address) to database if not alreaded added.
ipv4 - IP V4 Address as string value
 */
int updateInactiveAsset(std::string ipv4Addr)
{

	return 0;
	// add destination address with no time stamp if not already in table
	std::string queryString = "INSERT OR IGNORE INTO ASSETS (IP) VALUES (\"" + ipv4Addr + "\");";
	return otdb::query(queryString);
}
