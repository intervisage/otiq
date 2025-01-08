#ifndef OTFIFO_H
#define OTFIFO_H



#include <iostream>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>

//TO DO - Get rid of these headers
#include <sstream>
#include "otlog.h"


// Thread-safe FIFO queue using std::deque for pointers
template <typename T>
class ThreadSafeQueue
{
private:
    std::deque<T*> deque_;            // Deque to hold pointers
    std::mutex mutex_;                 // Mutex for thread safety
    std::condition_variable cond_var_; // Condition variable for synchronization

public:
    // Add an item to the back of the deque
    void enqueue(T item)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            deque_.push_back(item);
        }
        cond_var_.notify_one(); // Notify one waiting thread


    }

    // Remove an item from the front of the deque
    T* dequeue()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_var_.wait(lock, [this]
                       { return !deque_.empty(); }); // Wait until the deque is not empty
        T *item = deque_.front();
        deque_.pop_front();
        return item;
    }

    // Check if the deque is empty
    bool isEmpty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return deque_.empty();
    }

    int size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return deque_.size();
    }


void clear(pcpp::RawPacket *packet){
    {
        std::lock_guard<std::mutex> lock(mutex_);
       delete packet;

    }
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
