#ifndef OTASSETS_H
#define OTASSETS_H

#include <map>
#include <thread>
#include <mutex>

#include "otassets.h"

namespace otassets
{
    struct assetDetails
    {
        uint64_t lastActivity;
        uint64_t macAddress;
        bool macFromArp;
    };

    typedef uint64_t ipv4AddressInt;
}

class AssetList
{

private:
    std::map<otassets::ipv4AddressInt, otassets::assetDetails *> aList;
    std::mutex aListMutex;

public:
    AssetList()
    {
    }

    // Add an item to the back of the deque
    void addAsset(otassets::ipv4AddressInt ipv4Addr, otassets::assetDetails details)
    {
        std::lock_guard<std::mutex> lock(aListMutex);

        if (aList.find(ipv4Addr) == aList.end())
        {
            // no entry for this ip address exists so add ip address and asset details
            aList.insert({ipv4Addr, nullptr});
        }
    }

    // // Remove an item from the front of the deque
    // pcpp::RawPacket *getPacket()
    // {
    //     // check if deque is empty
    //     std::unique_lock<std::mutex> lock(dequeMutex);

    //     if (packetDeque.empty())
    //     {
    //         return nullptr;
    //     }
    //     else
    //     {
    //         pcpp::RawPacket *packet = packetDeque.front();
    //         packetDeque.pop_front();
    //         popCount++;

    //         return packet;
    //     }
    // }

    // // Check if the deque is empty
    // bool hasPackets()
    // {
    //     std::lock_guard<std::mutex> lock(dequeMutex);
    //     return !packetDeque.empty();
    // }

    int getSize()
    {
        std::lock_guard<std::mutex> lock(aListMutex);
        return aList.size();
    }

    // uint64_t getMaxQueueSize()
    // {
    //     return maxQueueSize;
    // }

    // uint64_t getPushCount()
    // {
    //     return pushCount;
    // }

    // uint64_t getPopCount()
    // {
    //     return popCount;
    // }

    // uint64_t getDropCountPush()
    // {
    //     return dropCountPush;
    // }

    // uint64_t getDropCountPop()
    // {
    //     return dropCountPop;
    // }

    // // Destructor to clean up remaining pointers in the deque
    // ~PacketBuffer()
    // {
    //     while (!packetDeque.empty())
    //     {
    //         packetDeque.pop_front();
    //     }
    // }
};

#endif
