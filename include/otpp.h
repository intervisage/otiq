#ifndef OTPP_H
#define OTPP_H

#include <Packet.h>

#include "otdb.h"

namespace
{

  

}

namespace otpp
{
    //  populates assets table based on passed packet
    void processPacket(pcpp::RawPacket *packet);

    // starts therad loop in blocked state
    void start();

    // cleans up thread by joining it to the calling code.
    void stop();
    // types of packet operations for paketBuffer

}

#endif