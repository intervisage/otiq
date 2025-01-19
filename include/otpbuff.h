#ifndef OTPBUFF_H
#define OTPBUFF_H

#include <iostream>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <Packet.h>

#include <sstream>
#include "otlog.h"

// Thread-safe FIFO queue using std::deque for pointers

class PacketBuffer
{
private:
    std::deque<pcpp::RawPacket *> packetDeque; // Deque to hold pointers
    std::mutex dequeMutex;                     // Mutex for thread safety
    uint64_t maxQueueSize;
    uint64_t allowedQueueSize;
    uint64_t dropCountPush;
    uint64_t dropCountPop;
    uint64_t popCount;
    uint64_t pushCount;

public:
    PacketBuffer()
    {
        maxQueueSize = 0;
        allowedQueueSize = 1500000;
        popCount = 0;
        pushCount = 0;
        dropCountPush = 0;
        dropCountPop = 0;
    }

    // Add an item to the back of the deque
    void addPacket(pcpp::RawPacket *packet)
    {
        std::lock_guard<std::mutex> lock(dequeMutex);

        if (packetDeque.size() < allowedQueueSize)
        {
            {
                packetDeque.push_back(packet);
                if (packetDeque.size() > maxQueueSize)
                {
                    maxQueueSize = packetDeque.size();
                }
            }
            pushCount++;
        }
        else
        {
            dropCountPush++;
        }
    }

    // Remove an item from the front of the deque
    pcpp::RawPacket *getPacket()
    {
        // check if deque is empty
        std::unique_lock<std::mutex> lock(dequeMutex);

        if (packetDeque.empty())
        {
            return nullptr;
        }
        else
        {
            pcpp::RawPacket *packet = packetDeque.front();
            packetDeque.pop_front();
            popCount++;

            return packet;
        }
    }

    // Check if the deque is empty
    bool hasPackets()
    {
        std::lock_guard<std::mutex> lock(dequeMutex);
        return !packetDeque.empty();
    }

    int getSize()
    {
        std::lock_guard<std::mutex> lock(dequeMutex);
        return packetDeque.size();
    }

    uint64_t getMaxQueueSize()
    {
        return maxQueueSize;
    }

    uint64_t getPushCount()
    {
        return pushCount;
    }

    uint64_t getPopCount()
    {
        return popCount;
    }

    uint64_t getDropCountPush()
    {
        return dropCountPush;
    }

    uint64_t getDropCountPop()
    {
        return dropCountPop;
    }

    // Destructor to clean up remaining pointers in the deque
    ~PacketBuffer()
    {
        while (!packetDeque.empty())
        {
            packetDeque.pop_front();
        }
    }
};

#endif
