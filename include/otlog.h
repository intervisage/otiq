#ifndef OTLOG_H
#define OTLOG_H


#include <string>

std::string getCurrentDateTime(std::string s);

namespace otlog
{
    void log(std::string logMsg);

    //removes all logs
    void deleteAll();

}

#endif