#ifndef OTDB_H
#define OTDB_H

#include <string>

namespace otdb
{

    // enumerations for macInfo within Assets table
    enum MacInfo
    {
        arp,
        ttl,
        none
    };

    // return string associated with enumeration e.g. "arp" for arp
    std::string getMacInfoString(MacInfo macInfo);

    // opens existing file based db and loads it into memory.
    // if file does not exist then creates database in memory.
    // returns non zero if fails. errorMessage provides last error.
    int open(std::string fileName);

    // runs a query
    int query(std::string queryString);

    // saves memoy based db to file and cleans up
    int close();

    // return number of rows returned by query()
    int rowCount();

    // returns value of nth column on specified row
    std::string resultValue(int row, int column);

    // return value of named column on specified row
    std::string resultValue(int row, std::string columnHeader);

    // returns current packet count
    int getCurrentPacketCount();

}

#endif