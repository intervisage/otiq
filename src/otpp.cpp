
#include <chrono>
#include <Packet.h>
#include <IPv4Layer.h>
#include <EthLayer.h>
#include <SystemUtils.h>
#include <thread>
#include <mutex>
#include <pthread.h>
#include <condition_variable>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <atomic>

#include "otpp.h"
#include "otbw.h"
#include "otlog.h"

std::thread processPacketThreadPtr;
pcpp::RawPacket *receivedPacket;
std::mutex mtx;
std::condition_variable pp_cv;

uint64_t minExTime = 1000000;
uint64_t maxExTime = 0;
uint64_t threadRunCount;

static std::atomic<bool> runThread;
bool terminateThread;

namespace otpp
{

	void start()
	{
		// initialise shared running status with locked

		//std::unique_lock<std::mutex> startThreadLock(mtx);
		runThread = false;
		threadRunCount =0;
		terminateThread = false;
		processPacketThreadPtr = std::thread(processPacketThread);
		std::string threadName = "otiq-otpp";
		pthread_setname_np(processPacketThreadPtr.native_handle(), threadName.c_str());
		otlog::log("OTPP: Packet processing loop initiated ( not running ).");
		//mtx.unlock();
	}

	void stop()
	{

		// set terminate flag (and runThread) and notify thread.
		//std::unique_lock<std::mutex> stopThreadLock(mtx);
		runThread = false;
		terminateThread = true;
		//mtx.unlock();
		//pp_cv.notify_one();

		// wait for 1 second to allow current loop time to complete.
		//sleep(1);

		processPacketThreadPtr.join();

		// if (processPacketThreadPtr.joinable())
		// {
		// 	processPacketThreadPtr.join();
		// }

		/* EXECUTION TIMING */
		std::cout << "Max Execution time (nano seconds) = " << std::to_string(maxExTime) << std::endl;
		std::cout << "Min Execution time (nano seconds) = " << std::to_string(minExTime) << std::endl;
		std::cout << "Thread Run Count = " << std::to_string(threadRunCount) << std::endl;
	}

	int processPacket(pcpp::RawPacket *packet)
	{

		if (!runThread)
		{
			receivedPacket = packet;
			// std::unique_lock<std::mutex> processPacketLock(mtx);
			runThread = true;
			// pp_cv.notify_one();
			return 0;
		}
		else
		{
			otbw::incDropPacketCount();
			return -1;
		}
	}

}

// Called from main to process packets as they arrive
void processPacketThread()
{

	while (!terminateThread)
	{

		if (runThread)
		{

			auto start = std::chrono::high_resolution_clock::now();

			runThread = false;

			threadRunCount ++;
			// process thread

			std::string queryString = "";
			char *error_message = 0;
			int rc = 0;

			// thread loop  - only designed to process one packet at a time
			// while (true)
			// {

			// aquire lock
			// std::unique_lock<std::mutex> processPacketThreadLock(mtx);

			// // block executon until notified via pp_cv and newPacketReceived status
			// pp_cv.wait(processPacketThreadLock, []()
			// 		   { return runThread || terminateThread; });

			// // time execution
			// // auto start = std::chrono::high_resolution_clock::now();

			// // if terminating thread then remove lock and return from thread
			// if (terminateThread)
			// {
			// 	mtx.unlock();
			// 	return;
			// }

			// parse raw packet
			// pcpp::Packet parsedPacket(receivedPacket);
			// pcpp::EthLayer *ethernetLayer = parsedPacket.getLayerOfType<pcpp::EthLayer>();

			// check if valid eternet II packet and stop processing if not
			// TODO - Add Support for IEEE 802.3 ?????????????????????????????????

			// if (ethernetLayer == nullptr)
			// {
			// 	otlog::log("MAIN: Invalid Ethernet packet at packet.. " + std::to_string(otbw::getCurrentPacketCount()));

			// 	// no futher processing required - so continue to get to wait at top of loop. Note still protected by lock.
			// 	runThread = false;
			// 	continue;
			// }

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

			// update bandwidth calculator with packet length
			// otbw::addByteCount(packet->getRawDataLen());

			// // check if arp message
			// auto *arpLayer = parsedPacket.getLayerOfType<pcpp::ArpLayer>();
			// if (arpLayer != nullptr)
			// {

			// 	otlog::log("MAIN: ARP message detected");

			// 	// note that sender is used by host requesting MAC address  and host responding with Mac address
			// 	updateActiveAsset(arpLayer->getSenderIpAddr().toString(), arpLayer->getSenderMacAddress().toString(), otdb::arp);

			// 	// no further processing of ARP message
			// 	return;
			// }

			// check if IPv4
			// pcpp::IPv4Layer *ipv4Layer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();
			// if (parsedPacket.isPacketOfType(pcpp::IPv4))
			// {
			// 	pcpp::IPv4Layer *ipv4Layer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();
			// 	updateActiveAsset(ipv4Layer->getSrcIPv4Address().toString());
			// 	updateInactiveAsset(ipv4Layer->getDstIPv4Address().toString());
			// }

			// once packet processed - change status of runThread
			// runThread = false;

			/* EXECUTION TIMING */

			auto stop = std::chrono::high_resolution_clock::now();

			uint64_t timeTaken = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

			otlog::log("MAIN: processPacket Execution time (nano seconds) = " + std::to_string(timeTaken));

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

	return 0;

	std::string queryString; // used in queries
	int rc;					 // return values from queries

	// get epoch time
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
	std::uint64_t timestamp = ms.count();
	std::string timestampString = std::to_string(timestamp);

	queryString = "INSERT OR IGNORE INTO ASSETS (IP) ";
	queryString += " VALUES ('" + ipv4Addr + "');";
	return otdb::query(queryString);

	// If asset not in table  - then add and set first and last activity time
	queryString = "SELECT IP, FIRST_ACTIVITY, MAC FROM ASSETS WHERE IP ='" + ipv4Addr + "';";
	otdb::query(queryString);

	if (otdb::rowCount() == 0)
	{

		// // asset is not in table so insert
		queryString = "INSERT INTO ASSETS (IP, FIRST_ACTIVITY, LAST_ACTIVITY, MAC)";
		queryString += " VALUES (\"" +
					   ipv4Addr + "\", " + timestampString + ", " + timestampString;

		// add MAC data if passed
		if (macInfo != otdb::none)
		{
			queryString += ", json('[{"
						   "addr:\"" +
						   macAddr + "\""
									 ", info:\"" +
						   otdb::getMacInfoString(macInfo) + "\""
															 ", time:\"" +
						   timestampString + "\""
											 "}]')";
		}
		else
		{

			// otherwise just add empty json array
			queryString += ", json('[]')";
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

			// if asset is in table but there is a first activity time, then we have already seen the asset transmitting - update the last activity time but not first activity time
			queryString = "UPDATE ASSETS SET LAST_ACTIVITY = " + timestampString;
		}

		// // Add MAC field if mac info supplied
		if (macInfo != otdb::none)
		{
			queryString += ", MAC = json_insert(MAC, '$[#]',json('{"
						   "addr:\"" +
						   macAddr + "\""
									 ", info:\"" +
						   otdb::getMacInfoString(macInfo) + "\""
															 ", time:\"" +
						   timestampString + "\""
											 "}'))";
		}
		else
		{
			// otherwise create empty json array unless there is already MAC info.
			std::string macData = otdb::resultValue(0, "MAC");
			if (macData == "")
			{
				queryString += ", MAC = json('[]')";
			}
		}

		// // finalise query string with 'where' clause
		queryString += " WHERE IP = '" + ipv4Addr + "';";
	}
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
