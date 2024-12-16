#include "otbw.h"

#include <map>
#include <thread>
#include <iostream>
#include <chrono>
#include <cstdint>
#include <iomanip>


#include "otlog.h"
#include "otdb.h"

namespace
{

    /* each bucket stores the number of times the bit count has been seen in one second
    eg (10000000,2) means max of 10000000 bps has been seen twice, (1000,5) means 1000bps has
    been seen 5 times.
    */

    std::uint64_t perSecondByteCount = 0; // running total of bits captured in current time period
    std::uint64_t bucketValue[10];
    std::uint64_t bucketSize;
    std::uint64_t minBandwidth, maxBandwidth;
    std::uint64_t packetCount;
    std::thread calcBandwidthThread;
    bool runStatus = false;
    std::uint64_t totalByteCount = 0;
    std::uint64_t totalDropPacketCount = 0;

    /* Gets bucket size  required to fit latest bps,  assuming 10 buckets in total*/
    uint64_t getBucketSize(uint64_t num)
    {
        // check number is in sesnible range and return 0 if not
        // assumes 999Gbps (999,999,999,999) will be highest bandwidth
        if (num < 0 or num > 1000000000000)
            return 0;

        uint64_t bSize = 10;
        num /= 100;

        while (num != 0)
        {
            num /= 10;
            bSize *= 10;
        }
        return bSize;
    }

    void calcBandwidth()
    {

        while (runStatus)
        {
            /* Calculates bandwidth and stores in maxBandwidth
             */

            // convert bytes to bits
            std::uint64_t perSecondBitCount = perSecondByteCount * 8;

            // update min and max bandwidths
            if (perSecondBitCount > maxBandwidth)
            {
                maxBandwidth = perSecondBitCount;
            }

            if (perSecondBitCount < minBandwidth)
            {
                minBandwidth = perSecondBitCount;
            }

            uint64_t requiredBucketSize = getBucketSize(perSecondBitCount);
            // check to see if new bucket size is required
            if (requiredBucketSize > bucketSize)
            {

                // std::string logEntry = " *** changing bucket size from " + std::to_string(bucketSize) + " from " + std::to_string(requiredBucketSize);
                // logEntry = logEntry + " bucketValue[0] was " + std::to_string(bucketValue[0]);

                // change bucket size to rnew required size
                bucketSize = requiredBucketSize;
                // move current individual bucket counts into first bucket and zeo individual buckets.
                for (int i = 1; i < 9; i++)
                {
                    bucketValue[0] += bucketValue[i];
                }

                // logEntry = logEntry + " is now " + std::to_string(bucketValue[0]);
                // otlog::log(logEntry);
            }

            // increment count of associated bucket
            int bucketToIncrement = perSecondBitCount / bucketSize;
            bucketValue[bucketToIncrement]++;

            // std::string logEntry = " perSecondCount = " + std::to_string(perSecondBitCount) + ", bucketToIncrement = " + std::to_string(bucketToIncrement) + ", new bucket count = " + std::to_string(bucketValue[bucketToIncrement]);
            // otlog::log(logEntry);

            // resets bit counter
            perSecondByteCount = 0;

            // sleep for 1 second - Do not change the sample period
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

}

namespace otbw
{
    /*  creates and detached thread to calculate bandwidth
     */
    void start()
    {
    

        // initialis smallest bucket
        // lowest number of bps per bucket. So with 10 buckets, max mandwidth = 999bps eg 1000bps - 1.
        bucketSize = 10;
        minBandwidth = 10000000000;
        maxBandwidth = 0;
        packetCount = 0;

        // initialise bucket
        for (int i = 0; i < 10; i++)
        {
            bucketValue[i] = 0;
        }

    
        // zero total byte count
        totalByteCount = 0;

        // start thread execution
        runStatus = true;
        calcBandwidthThread = std::thread(calcBandwidth);

        // provide custom name for thread
        std::string threadName = "otiq-otbw";
        pthread_setname_np(calcBandwidthThread.native_handle(), threadName.c_str());
        // calcBandwidthThread.detach(); NOTE _ DETACH NOT REQUIRED _ NEED THREAD TO BE JOINED BY STOP
    }

    /* terminates and joins thread
     */
    void stop()
    {
        // stop process and join to main thread
        runStatus = false;

        calcBandwidthThread.join();

        // convert bucket values into percentages
        int numberOfValues = 0;
        for (int i = 0; i < 10; i++)
        {
            numberOfValues += bucketValue[i];
        }
        for (int i = 0; i < 10; i++)
        {
            bucketValue[i] = bucketValue[i] * 100 / numberOfValues;
        }

        // copy values to database - assumes database is still open.
        std::string queryString = "INSERT INTO BANDWIDTH (BUCKET_SIZE, MAXIMUM, MINIMUM, BUCKET_VALUES)"
                                  " VALUES (" +
                                  std::to_string(bucketSize) + "," + std::to_string(maxBandwidth) + "," + std::to_string(minBandwidth) + ",json_array(";

        for (int i = 0; i < 9; i++)
        {
            queryString += std::to_string(bucketValue[i]) + ",";
        }
        queryString += std::to_string(bucketValue[9]) + "));";
        int rc = otdb::query(queryString);
    }

    // Used to add the number of received bytes to the total
    void addByteCount(int packetByteCount)
    {
        packetCount++;
        perSecondByteCount += packetByteCount;
        totalByteCount += packetByteCount;
    }

    uint64_t getCurrentPacketCount()
    {
        return packetCount;
    }

    // prints out bandwidths
    void printBandwidths()
    {
        std::cout << "Packet count = " << std::to_string(packetCount) << std::endl;
        std::cout << "Dropped Packets  = " << std::to_string(totalDropPacketCount) << std::endl;
        float droppedpercent = (float)totalDropPacketCount * 100 / packetCount;
        std::cout << "Percentage dropped packets  = " << std::fixed << std::setprecision(2) << std::to_string(droppedpercent) << std::endl;


        std::cout << "Total byte count = " << totalByteCount << " , Bandwidths as follows... " << std::endl;

        std::cout << "Min Bandwidth = " << std::to_string(minBandwidth) << std::endl;
        std::cout << "Max Bandwidth  = " << std::to_string(maxBandwidth) << std::endl;
        std::cout << "Final bucket size  = " << std::to_string(bucketSize) << std::endl;

        

        for (int i = 0; i < 10; i++)
        {
            std::cout << "bucket [ " << std::to_string(i) + " ] = " << std::to_string(bucketValue[i]) << std::endl;
        }
    }

    void incDropPacketCount()
    {
        totalDropPacketCount++;
    }

    uint64_t getDropPacketCount()
    {
        return totalDropPacketCount;
    }

}
