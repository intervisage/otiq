
#ifndef OTBW_H
#define OTBW_H

#include <cstdint>


namespace otbw
{
    //  creates and detaches thread to calculate bandwidth
    void start();

    // terminates and joins thread
    void stop();

    // Add received packet bytes to total for period
    void addByteCount(int packetBitCount);

    // returns current packet count
    uint64_t getCurrentPacketCount();

    // prints out bandwidths
    void printBandwidths();

}

#endif