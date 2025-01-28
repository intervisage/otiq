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
    std::condition_variable cv;
    std::atomic<bool> clearBufferFlag;

public:
    PacketBuffer()
    {
        maxQueueSize = 0;
        allowedQueueSize = 1200000;
        popCount = 0;
        pushCount = 0;
        dropCountPush = 0;
        dropCountPop = 0;
        clearBufferFlag.store(false);
    }

    // Add an item to the back of the deque
    void addPacket(pcpp::RawPacket *packet)
    {
        std::lock_guard<std::mutex> lock(dequeMutex);

        if (packetDeque.size() > allowedQueueSize)
        {
            dropCountPush++;
        }
        else
        {
            {
                packetDeque.push_back(packet);
                if (packetDeque.size() > maxQueueSize)
                {
                    maxQueueSize = packetDeque.size();
                }
            }
            pushCount++;
            cv.notify_one();
        }
    }

    /* Removes an item from the front of the deque
    clearBufferFlag used to decide if getPacket should wait for notification from 
    addPacket. Note this is important once capture has ended - in order t clear out the buffer
    */
    pcpp::RawPacket *getPacket()
    {
        
        std::unique_lock<std::mutex> lock(dequeMutex);

        // if not clearing buffer after capture is finished then wait for notification from
        // addPackt.




        if (!clearBufferFlag.load())
        {
            cv.wait(lock, [this]
                    { return !packetDeque.empty() || clearBufferFlag.load(); });
        }

        // if no packets, return null pointer - used by calling routine to check if packets
        // are available for proceassing
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

    // used to cklear buffer after capture has completed
    void clearBuffer()
    {
        std::lock_guard<std::mutex> lock(dequeMutex);
        clearBufferFlag.store(true);

        // notify all threads to make sure none are left hanging waiting for addPacket to trigger notification
        cv.notify_all();
        
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

    // Destructor to clean up remaining pointers in the deque - shouldn't be any@@
    ~PacketBuffer()
    {
        while (!packetDeque.empty())
        {
            packetDeque.pop_front();
        }
    }
};

#endif
