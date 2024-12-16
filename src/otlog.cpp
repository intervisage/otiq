#include "otlog.h"
#include <string>
#include <fstream>
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

/* Returns current timestamp with ms based on GMT*/
std::string getCurrentTimestamp()
{
    // Get current time as a time_point
    auto now = std::chrono::system_clock::now();

    // Convert to time_t to get the calendar time
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // Convert to tm for formatting
    std::tm *now_tm = std::gmtime(&now_c);

    // Format the timestamp
    std::ostringstream oss;

    // Get milliseconds
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    oss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();

    return oss.str();
}

/* Returns current date based on GMT*/
std::string getCurrentDate()
{
    // Get current time as a time_point
    auto now = std::chrono::system_clock::now();

    // Convert to time_t to get the calendar time
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // Convert to tm for formatting
    std::tm *now_tm = std::gmtime(&now_c);

    // Format the timestamp
    std::ostringstream oss;
    oss << std::put_time(now_tm, "%Y-%m-%d");
    return oss.str();
}


void otlog::deleteAll()
{
   system("rm ./log_* > /dev/null"); 
}

void otlog::log(std::string logMsg)
{
    std::string filePath = "./log_" + getCurrentDate() + ".txt";
    std::string now = getCurrentTimestamp();
    std::ofstream ofs(filePath.c_str(), std::ios_base::out | std::ios_base::app);
    ofs << now << '\t' << logMsg << '\n';
    ofs.close();
}
