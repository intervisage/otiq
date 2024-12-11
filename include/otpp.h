#ifndef OTPP_H
#define OTPP_H


#include <Packet.h>

#include "otdb.h"




void processPacketThread();

int updateActiveAsset(std::string ipv4Addr, std::string macAddr = "", otdb::MacInfo macInfo = otdb::none);

int updateInactiveAsset(std::string ipv4Addr);

std::unique_ptr<pcpp::RawPacket> pktBufferOps(bool inserting, pcpp::RawPacket *packet);

namespace otpp
{
    //  populates assets table based on passed packet
    void processPacket(pcpp::RawPacket *packet);

    // starts therad loop in blocked state
    void start();

    // cleans up thread by joining it to the calling code.
    void stop();

}

#endif