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
        std::string ipv4Address; 
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


    int getSize()
    {
        std::lock_guard<std::mutex> lock(aListMutex);
        return aList.size();
    }
 
};

#endif
