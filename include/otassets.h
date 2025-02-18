#ifndef OTASSETS_H
#define OTASSETS_H

#include <map>
#include <mutex>
#include <thread>

#include "otassets.h"
#include "otdb.h"

namespace otassets {

  // enumerations for macInfo within Assets table
  enum MacInfo { arp, duparp, ttl, none };

  struct assetDetails {
    std::string ipAddrStr;
    std::string macAddr;
    MacInfo macInfo;
    std::string dupMacAddr; // used for IP with more than one Mac Address - stores
    // only one duplaicetd mac address.
  };

  typedef __uint128_t otIPAddr;
} // namespace otassets

class AssetList {

private:
  std::map<otassets::otIPAddr, otassets::assetDetails> aList;
  std::mutex aListMutex;

public:
  AssetList() {}

  static __uint128_t bytes_to_uint128(const uint8_t bytes[16]) {
    __uint128_t result = 0;
    for (int i = 0; i < 16; i++) {
      result = (result << 8) | bytes[i];
    }
    return result;
  }

  static std::string uint128_to_string(__uint128_t value) {
    if (value == 0)
      return "0";

    std::stringstream ss;
    std::string result;
    while (value > 0) {
      ss << static_cast<char>('0' + (value % 10));
      value /= 10;
    }

    result = ss.str();
    std::reverse(result.begin(), result.end());
    return result;
  }

  // Add or modify an asset
  void addAssetInfo(otassets::otIPAddr ipAddr, otassets::assetDetails& details) {
    std::lock_guard<std::mutex> lock(aListMutex);

    auto foundAsset = aList.find(ipAddr);

    if (foundAsset == aList.end()) {
      // no entry for this ip address exists so add ip address and asset details
      aList.insert({ ipAddr, details });
    }
    else {
      // entry found so update existing assets with details. Note that arp based
      // MAC suplants ttl based MAC address also - if already have a arp for a
      // different MAC, update MAC address and set mac info to duplaicted arp

      switch (details.macInfo) {

      case otassets::none:
        break;

      case otassets::ttl:
        if (foundAsset->second.macInfo == otassets::none) {
          // if passed mac is based on ttl then update mac only if currently no
          // mac address
          foundAsset->second.macAddr = details.macAddr;
          foundAsset->second.macInfo = otassets::ttl;
        }
        break;

      case otassets::arp:

        if (foundAsset->second.macInfo != otassets::arp) {
          // if passed mac is based on arp and existing mac is based on ttl or
          // if has not beeen set ( ie not arp ) then update mac
          foundAsset->second.macAddr = details.macAddr;
          foundAsset->second.macInfo = otassets::arp;

        }
        else {

          if (details.macAddr != foundAsset->second.macAddr) {
            // if passed mac address with arp is not the same at previously
            // stored mac address then mark as duplicated arp
            foundAsset->second.dupMacAddr =
              foundAsset->second.macAddr; // store previous as dup mac
            foundAsset->second.macAddr = details.macAddr;
            foundAsset->second.macInfo = otassets::duparp;
          }
        }
        break;

        return;
      }
    }
  }

  int getSize() {
    std::lock_guard<std::mutex> lock(aListMutex);
    return aList.size();
  }

  int saveToDB(int captureNumber) {
    otlog::log("OTASSETS: Saving assets to database. Total assets =  " +
      aList.size());
    for (const auto& pair : aList) {
      // std::cout << uint128_to_string(pair.first) << " , " <<
      // pair.second.ipAddrStr << std::endl;

      std::string queryString = "INSERT INTO ASSETS";
      if (pair.second.macInfo == otassets::none) {
        queryString += "(IP) VALUES (\"" + pair.second.ipAddrStr + "\");";
      }
      else
      {
        std::string macInfoStr = "";
        switch (pair.second.macInfo) {
        case otassets::arp:
          macInfoStr = "arp";
          break;
        case otassets::duparp:
          macInfoStr = "dup-arp";
          break;
        case otassets::ttl:
          macInfoStr = "ttl";
          break;
        }

        queryString += " (IP, MAC, MAC_INFO, DUP_MAC) VALUES ("
          "\"" + pair.second.ipAddrStr + "\", "
          "\"" + pair.second.macAddr + "\", "
          "\"" + macInfoStr + "\", "
          "\"" + pair.second.dupMacAddr + "\" );";

      }
      otdb::query(queryString);
    }

    return 0;
  }
};

#endif
