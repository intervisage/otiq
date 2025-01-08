#ifndef OTFIFO_H
#define OTFIFO_H

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

class ThreadSafeQueue
{
private:
    std::deque<pcpp::Packet *> deque_; // Deque to hold pointers
    std::mutex mutex_;                 // Mutex for thread safety
    std::condition_variable cond_var_; // Condition variable for synchronization
    std::atomic_bool ignore_wait;      // used to stop deque witing for notification from enqueue thread
    uint64_t maxQueueSize;
    uint64_t allowedQueueSize;
    uint64_t dropCount;
    uint64_t popCount;
    uint64_t pushCount;

public:
    ThreadSafeQueue()
    {
        ignore_wait = false;
        maxQueueSize = 0;
        allowedQueueSize = 50000000;
        dropCount = 0;
        pushCount = 0;
        popCount = 0;
    }

    // Add an item to the back of the deque
    void enqueue(pcpp::Packet *item)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (deque_.size() < allowedQueueSize)
        {
            {
                {
                    // std::lock_guard<std::mutex> lock(mutex_);
                    deque_.push_back(item);
                }
                if (deque_.size() > maxQueueSize)
                {
                    maxQueueSize = deque_.size();
                }
            }
            pushCount++;
        }
        else
        {
            otlog::log("Buffer max exceeded - currently = " + std::to_string(deque_.size()));
            dropCount++;
        }
        // cond_var_.notify_one(); // Notify one waiting thread
    }

    // Remove an item from the front of the deque
    pcpp::Packet *dequeue()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        //     cond_var_.wait(lock, [this]
        //                    { return !deque_.empty(); }); // Wait until the deque is not empty
        if (deque_.size() != 0)
        {
            pcpp::Packet *item = deque_.front();
            deque_.pop_front();
            popCount++;
            return item;
        }
        else
        {
            return nullptr;
        }
    }

    // Check if the deque is empty
    bool isEmpty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return deque_.empty();
    }

    int getSize()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return deque_.size();
    }

    void stop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ignore_wait = true;
    }

    uint64_t getMaxQueueSize()
    {
        return maxQueueSize;
    }

    uint64_t getDropCount()
    {
        return dropCount;
    }

    uint64_t getPushCount()
    {
        return pushCount;
    }

    uint64_t getPopCount()
    {
        return popCount;
    }

    // Destructor to clean up remaining pointers in the deque
    ~ThreadSafeQueue()
    {
        while (!deque_.empty())
        {
            delete deque_.front();
            deque_.pop_front();
        }
    }
};

#endif
