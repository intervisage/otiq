#ifndef OTPP_H
#define OTPP_H



namespace otpp
{
    // call back for arriving packets
    void onPacketArrives(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *cookie);

    // starts therad loop in blocked state
    void start(pcpp::PcapLiveDevice *dev);

    // cleans up thread by joining it to the calling code.
    void stop(pcpp::PcapLiveDevice *dev);
    // types of packet operations for paketBuffer

}

#endif