
#include <chrono>
#include <Packet.h>
#include <IPv4Layer.h>
#include <EthLayer.h>
#include <ArpLayer.h>
#include <SystemUtils.h>
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

#include "otpp.h"
#include "otbw.h"
#include "otlog.h"

enum bufferOpTypes
{
	PUSH,
	POP
};

pcpp::RawPacket *pktBufferOps(bufferOpTypes bufferOp, pcpp::RawPacket *packet);

void processPacketThread();

int updateActiveAsset(std::string ipv4Addr, std::string macAddr = "", otdb::MacInfo macInfo = otdb::none);

int updateInactiveAsset(std::string ipv4Addr);

std::deque<pcpp::RawPacket *> pktBuffer;

std::map<uint64_t,uint64_t> ipList;


std::thread processPacketThreadPtr_1;
std::thread processPacketThreadPtr_2;
pcpp::RawPacket *receivedPacket;
std::mutex pktBufferOpMutex;

std::condition_variable pp_cv;

uint64_t minExTime;
uint64_t maxExTime;
uint64_t totalExecutionTime;
uint64_t pushCount;
uint64_t popCount;
uint64_t dropCount;
uint64_t maxpktsQueued;
uint64_t pktsQueued;

static std::atomic<bool> runThread;
bool terminateThread;

namespace otpp
{

	void start()
	{

		// initialise shared running status with locked

		// std::unique_lock<std::mutex> startThreadLock(mtx);
		minExTime = 100000000;
		maxExTime = 0;
		totalExecutionTime = 0;
		maxpktsQueued = 0;
		pktsQueued = 0;

		runThread = false;
		pushCount = 0;
		popCount = 0;
		dropCount = 0;

		terminateThread = false;

		processPacketThreadPtr_1 = std::thread(processPacketThread);
		std::string threadName1 = "otiq-otpp-1";
		pthread_setname_np(processPacketThreadPtr_1.native_handle(), threadName1.c_str());

		// processPacketThreadPtr_2 = std::thread(processPacketThread);
		// std::string threadName2 = "otiq-otpp-2";
		// pthread_setname_np(processPacketThreadPtr_2.native_handle(), threadName2.c_str());

		otlog::log("OTPP: Packet processing loop initiated ( not running ).");
		otlog::log("Maximum packet buffer size supported = " + std::to_string(pktBuffer.max_size()));
		// mtx.unlock();
	}

	void stop()
	{

		// signal thread to stop
		terminateThread = true;
		otlog::log("OTPP: Terminating packet processing loop.");

		// Monitor time taken to process packets in buffer after terminate signal passed
		auto clearoutStart = std::chrono::high_resolution_clock::now();
		uint64_t clearoutPackets = pktsQueued;

		// wait for thread to return
		processPacketThreadPtr_1.join();
		// processPacketThreadPtr_2.join();

		auto clearoutStop = std::chrono::high_resolution_clock::now();

		uint64_t clearoutTime = std::chrono::duration_cast<std::chrono::seconds>(clearoutStop - clearoutStart).count();

		otlog::log("OTPP: Buffer clearout packets = " + std::to_string(clearoutPackets));
		otlog::log("OTPP: Buffer clearout time (seconds) = " + std::to_string(clearoutTime));

		// clear buffer - this will delete packet instances through use of unique_ptrs
		if (pktBuffer.size() != 0)
		{
			otlog::log("OTPP: WARNING - Packets left in buffer!!!");
		}
		else
		{
			otlog::log("OTPP: No remaining packets in buffer.");
		}
		pktBuffer.clear();

		/* EXECUTION TIMING */

		uint64_t avgExeTime = pushCount == 0 ? 0 : totalExecutionTime / pushCount;
		std::cout << "Max Execution time (nano seconds) = " << std::to_string(maxExTime) << std::endl;
		std::cout << "Min Execution time (nano seconds) = " << std::to_string(minExTime) << std::endl;
		std::cout << "Avg Execution time (nano seconds) = " << std::to_string(avgExeTime) << std::endl;
		std::cout << "Maximum buffer size = " << std::to_string(maxpktsQueued) << std::endl;
		std::cout << "Push Count = " << std::to_string(pushCount) << std::endl;
		std::cout << "Pop Count = " << std::to_string(popCount) << std::endl;
		std::cout << "Drop Packet Count = " << std::to_string(dropCount) << std::endl;
	}

	void processPacket(pcpp::RawPacket *packet)
	{

		// add packet to buffer unless tthread is being terminated.
		if (!terminateThread)
		{
			pktBufferOps(PUSH, packet);
		}
	}

}

/* ensures all buffer operations are protected by lock.
bufferOp: type of operation. packet:
RawPacket to be inserted during push operation.
return null pointer for all push opperations.
returns RawPacket for pop operations or null ptr if no packetes in buffer */

pcpp::RawPacket *pktBufferOps(bufferOpTypes bufferOp, pcpp::RawPacket *packet)
{

	// std::cout << "Packet in buffer = " << pktsQueued << std::endl;

	// lock this code - note lock will be released on return ( lock goes out of scope)
	std::unique_lock<std::mutex> pkBufferOpLock(pktBufferOpMutex);

	if (bufferOp == PUSH)
	{

		pushCount++;

		// enforce limit on buffer size //TODO - need to consider how this limit is set.
		if (pktsQueued < 100000000)
		{
			// instatiate new unique packet pointer with passed rawpacket pointer.
			pktBuffer.push_back(new pcpp::RawPacket(*packet));

			// increment buffer size
			pktsQueued++;

			// update max buffer size
			if (pktsQueued > maxpktsQueued)
			{
				maxpktsQueued = pktsQueued;
			}
		}
		else
		{
			// otbw::incDropPacketCount();
			dropCount++;
		}

		// always return null ptr when inserting
		return nullptr;
	}

	if (bufferOp == POP)
	{

		// check there are packets left to pop
		if (pktBuffer.size() == 0)
		{
			return nullptr;
		}
		else
		{

			// get first item
			pcpp::RawPacket *returnPacket = pktBuffer.front();

			// remove first item
			pktBuffer.pop_front();

			// decrement count of packets to be processed
			pktsQueued--;

			// return what was the first packet in the buffer
			return returnPacket;
		}
	}
	// catch all return null pointer.
	return nullptr;
}

// Called from main to process packets as they arrive
void processPacketThread()
{

	while (!terminateThread)
	{
		pcpp::RawPacket *receivedPacket;

		while ((receivedPacket = pktBufferOps(POP, nullptr)) != nullptr)
		{

			popCount++;

			auto start = std::chrono::high_resolution_clock::now();

			pcpp::Packet parsedPacket(receivedPacket);

			// process thread
			std::string queryString = "";
			char *error_message = 0;
			int rc = 0;

			// parse raw packet

			pcpp::EthLayer *ethernetLayer = parsedPacket.getLayerOfType<pcpp::EthLayer>();

			// check if valid eternet II packet and stop processing if not
			// TODO - Add Support for IEEE 802.3 ?????????????????????????????????

			if (ethernetLayer == nullptr)
			{
				// no futher processing required - so continue
				continue;
			}

			// don't process communications between MacBook and Virtual Machine
			// pcpp::MacAddress ubuntuMacAddress = pcpp::MacAddress("08:00:27:54:a5:55");
			// pcpp::MacAddress macBookMacAddress = pcpp::MacAddress("3a:f9:d3:16:91:64");
			// pcpp::MacAddress srcMacAddr = ethernetLayer->getSourceMac();
			// pcpp::MacAddress dstMacAddr = ethernetLayer->getDestMac();

			// if (srcMacAddr == macBookMacAddress || dstMacAddr == macBookMacAddress || srcMacAddr == ubuntuMacAddress || dstMacAddr == ubuntuMacAddress)
			// {
			// 	runThread = false;
			// 	continue;
			// }

			// // check if arp message
			auto *arpLayer = parsedPacket.getLayerOfType<pcpp::ArpLayer>();
			if (arpLayer != nullptr)
			{
				// note that sender is used by host requesting MAC address  and host responding with Mac address
				updateActiveAsset(arpLayer->getSenderIpAddr().toString(), arpLayer->getSenderMacAddress().toString(), otdb::arp);

				// no further processing of ARP message
				continue;
			}

			// check if IPv4
			pcpp::IPv4Layer *ipv4Layer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();

			if (parsedPacket.isPacketOfType(pcpp::IPv4))
			{
				pcpp::IPv4Layer *ipv4Layer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();

				// get sources address as string
				updateActiveAsset(ipv4Layer->getSrcIPv4Address().toString());

				// updateInactiveAsset(ipv4Layer->getDstIPv4Address().toString());
			}

			/* EXECUTION TIMING */

			auto stop = std::chrono::high_resolution_clock::now();

			uint64_t timeTaken = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

			totalExecutionTime += timeTaken;

			// update process time metrics
			if (timeTaken < minExTime)
			{
				minExTime = timeTaken;
			}

			if (timeTaken > maxExTime)
			{
				maxExTime = timeTaken;
			}
		}
	}
}

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
