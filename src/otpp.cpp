
#include <chrono>
#include <Packet.h>
#include <IPv4Layer.h>
#include <IPv6Layer.h>
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

PacketBuffer pBuffer;

std::thread processPacketThreadPtr_1;
std::thread processPacketThreadPtr_2;
std::thread processPacketThreadPtr_3;
std::thread processPacketThreadPtr_4;

pcpp::RawPacket* receivedRawPacket;
std::mutex parserMutex;
bool terminateThread;
AssetList assetList;

int arpCount = 0; //////////////////////////////////////

namespace otpp
{

	void start(pcpp::PcapLiveDevice* dev)
	{

		terminateThread = false;
		// receivedRawPacket = nullptr;

		// start packet capture thread
		otlog::log("OTPP: Starting async traffic capture.");
		dev->startCapture(otpp::onPacketArrives, &pBuffer);

		// start packet consumer thread 1
		processPacketThreadPtr_1 = std::thread(processPacket);
		std::string threadName1 = "otiq-otpp-1";
		pthread_setname_np(processPacketThreadPtr_1.native_handle(), threadName1.c_str());

		// start packet consumer thread 2
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

	void stop(pcpp::PcapLiveDevice* dev)
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

		assetList.saveToDB(1); // ************

		// output stats
		float avgDrop = static_cast<float>(pBuffer.getDropCountPush()) * 100 / (pBuffer.getDropCountPush() + pBuffer.getPushCount());
		float buffPercent = static_cast<float>(pBuffer.getMaxQueueSize()) * 100 / pBuffer.getPushCount();
		std::cout << "Maximum buffer size = " << std::to_string(pBuffer.getMaxQueueSize()) << std::endl;
		std::cout << "Maximum buffer size percentage of Push = " << std::to_string(buffPercent) << std::endl;
		std::cout << "Push Count = " << std::to_string(pBuffer.getPushCount()) << std::endl;
		std::cout << "Pop Count = " << std::to_string(pBuffer.getPopCount()) << std::endl;
		std::cout << "Drop Packet Count Push = " << std::to_string(pBuffer.getDropCountPush()) << std::endl;
		std::cout << "Drop Packet Percentage = " << std::to_string(avgDrop) << std::endl;
		std::cout << "Asset list size = " << std::to_string(assetList.getSize()) << std::endl;
	}

	// Note that onPacketArrives is called from a separate PCapPlusPlus thread - so need to pass pointer to buffer
	void onPacketArrives(pcpp::RawPacket* rawPacket, pcpp::PcapLiveDevice* dev, void* userData)
	{

		// pass raw packet length to otbw to calculate bandwidth
		otbw::addByteCount(rawPacket->getRawDataLen());

		// create new raw packet from passed raw packet and push associated pointer into buffer
		pcpp::RawPacket* newRawPacket = new pcpp::RawPacket(*rawPacket);
		PacketBuffer* pBuffer = reinterpret_cast<PacketBuffer*>(userData);
		pBuffer->addPacket(newRawPacket);
	}
}

void processPacket()
{

	while (!terminateThread || pBuffer.hasPackets())
	{

		pcpp::RawPacket* rawPacket = pBuffer.getPacket();

		if (rawPacket != nullptr)
		{
			// parse raw packet
			pcpp::Packet* packet = new pcpp::Packet(rawPacket);

			if (packet->isPacketOfType(pcpp::Ethernet))
			{

				// check if valid eternet II packet and stop processing if not
				// TODO - Add Support for IEEE 802.3

				// check for Arp message
				pcpp::ArpLayer* arpLayer = packet->getLayerOfType<pcpp::ArpLayer>();
				if (arpLayer != nullptr)
				{
					// can always get mac and IP adress of requesting device
					otassets::otIPAddr ipAddr = arpLayer->getSenderIpAddr().toInt();
					otassets::assetDetails details = { arpLayer->getSenderIpAddr().toString(),arpLayer->getSenderMacAddress().toString(),otassets::arp };
					assetList.addAssetInfo(static_cast<otassets::otIPAddr>(ipAddr), details);
					
					if (arpLayer->isReply()) {
						// if reply - we can also get info on responding device
						otassets::otIPAddr ipAddr = arpLayer->getTargetIpAddr().toInt();
						otassets::assetDetails details = { arpLayer->getTargetIpAddr().toString(),arpLayer->getTargetMacAddress().toString(),otassets::arp };
						assetList.addAssetInfo(static_cast<otassets::otIPAddr>(ipAddr), details);
					}
				}

				else if (packet->isPacketOfType(pcpp::IPv6))
				{
					pcpp::IPv6Layer* ipv6Layer = packet->getLayerOfType<pcpp::IPv6Layer>();
					otassets::otIPAddr ipAddr = AssetList::bytes_to_uint128(ipv6Layer->getSrcIPv6Address().toBytes());
					otassets::assetDetails details = { ipv6Layer->getSrcIPv6Address().toString(), "", otassets::none };
					assetList.addAssetInfo(static_cast<otassets::otIPAddr>(ipAddr), details);
				}

				// check for ipV4 - NOTE THIS MUST BE DONE AFTER IPV6 SO ENCAPSUATED IPV4 DOES NOT SHOW Up
				else if (packet->isPacketOfType(pcpp::IPv4))
				{
					pcpp::IPv4Layer* ipv4Layer = packet->getLayerOfType<pcpp::IPv4Layer>();
					otassets::otIPAddr ipAddr = ipv4Layer->getSrcIPv4Address().toInt();
					otassets::assetDetails details = { ipv4Layer->getSrcIPv4Address().toString(), "", otassets::none };
					assetList.addAssetInfo(ipAddr, details);
				}
			}

			/* Clean up memory*/

			delete packet;
			delete rawPacket;
		}
	}
}