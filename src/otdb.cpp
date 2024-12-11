
#include "otdb.h"
#include "otlog.h"
#include "sqlite3.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

namespace
{
    // anonymous namespace used to control visibility

    /* in-memory based db instance */
    sqlite3 *db;

    /* file-based db instance */
    sqlite3 *fileDb;

    /* file name for storing file-based eb instance */
    std::string dbFileName;

    /* Column headings for query reponse */
    std::vector<std::string> colHeadings;

    /* query reponse rows returned as matrix of strings  =  rows x rowValueTypes */
    typedef std::vector<std::string> rowValuesType;
    typedef std::vector<rowValuesType> rowsType;
    rowsType rows;

    /* Last error code */
    char *error_message = 0;

    /* Clears query response matrix */
    void clearResponseRows()
    {
        colHeadings.clear();
        for (int i = 0; i < rows.size(); i++)
        {
            rows[i].clear();
        }
        rows.clear();
    }

    /* callback called by sqlite execute function to populate reponse matrix */
    int callback(void *, int argc, char **argv, char **azColName)
    {
        // store headings if not already added
        if (colHeadings.empty())
        {
            for (int i = 0; i < argc; i++)
            {
                colHeadings.push_back(std::string(azColName[i]));
            }
        }

        // store row values
        rowValuesType currentRowValues;

        for (int i = 0; i < argc; i++)
        {
            currentRowValues.push_back(std::string(argv[i]));
        }

        // append array of row values
        rows.push_back(currentRowValues);

        return 0;
    }

    // returns Sqlite version number
    std::string getSqlite3Version()
    {
        std::string queryString = "SELECT sqlite_version();";
        otdb::query(queryString);
        std::string versionNumber = otdb::resultValue(0, 0);
        return versionNumber;
    }

}

namespace otdb
{

    /* Opens an sqlite3 file using the passed filename.
    ,/If file does not exist a new database is created with required tables and save to the file.*/
    int open(std::string fileName)
    {

        dbFileName = fileName;

        std::string sql;
        int dbFileExistsFlag = 0;
        char *error_message = 0;
        int rc; // return values

        // set flag if file exists
        std::ifstream file(dbFileName);
        dbFileExistsFlag = (file.is_open());
        file.close();

        // TODO - check return code

        if (dbFileExistsFlag)
        {

            otlog::log("OTDB: Found exiting database file ( " + dbFileName + " ).");

            // create database instance in memory
            rc = sqlite3_open(":memory:", &db);

            if (rc != SQLITE_OK)
            {
                otlog::log("OTDB: Failed to create database instance in memory. Return value..." + std::to_string(rc) + ", " + sqlite3_errstr(rc));
                return -1;
            }

            // get version of sqlite3
            otlog::log("OTDB: Using sqlite version " + getSqlite3Version());

            //  create handle to disk database
            rc = sqlite3_open(fileName.c_str(), &fileDb);
            if (rc != SQLITE_OK)
            {
                otlog::log("OTDB: Failed to open file-based database( " + fileName + " ). Return value..." + std::to_string(rc) + ", " + sqlite3_errstr(rc));
                return -1;
            }

            sqlite3_backup *onlineBackupHandle = sqlite3_backup_init(db, "main", fileDb, "main");

            // Copy all pages
            rc = sqlite3_backup_step(onlineBackupHandle, -1);
            if (rc != SQLITE_DONE)
            {
                otlog::log("OTDB: Failed to copy file-based database into memory. Return value... " + std::to_string(rc) + ", " + sqlite3_errstr(rc));
                return -1;
            }

            rc = sqlite3_backup_finish(onlineBackupHandle);
            if (rc != SQLITE_OK)
            {
                otlog::log("OTDB: Failed to clean up after copy of file-based database into memory. Return value... " + std::to_string(rc) + ", " + sqlite3_errstr(rc));
                return -1;
            }

            otlog::log("OTDB: Successfully loaded file-based database ( " + dbFileName + " ) into memory.");
            return 0;
        }
        else
        {

            otlog::log("OTDB: Database file ( " + dbFileName + " ) not found.");

            // create database instance in memory
            rc = sqlite3_open(":memory:", &db);
            if (rc != SQLITE_OK)
            {
                otlog::log("OTDB: Failed to open memory-based database. Return value..." + std::to_string(rc) + ", " + sqlite3_errstr(rc));
                return -1;
            }

            // get version of sqlite3
            otlog::log("OTDB: Using sqlite version " + getSqlite3Version());

            // create asset table
            // JSON objects for MAC as follows son('[{addr:"12:34:56:78:9A",info:"ttl",time:"123456722" }]'))
            // Note - default value for JSON is important!!!

            sql = "CREATE TABLE ASSETS("
                  "IP TEXT PRIMARY KEY,"
                  "FIRST_ACTIVITY INT DEFAULT 0,"
                  "LAST_ACTIVITY INT DEFAULT 0,"
                  "MAC JSON DEFAULT '') WITHOUT ROWID;";    

            // create bandwidth table
            sql += "CREATE TABLE BANDWIDTH("
                   "BUCKET_SIZE INT DEFAULT 0,"
                   "MAXIMUM INT DEFAULT 0,"
                   "MINIMUM INT DEFAULT 0,"
                   "BUCKET_VALUES JSON DEFAULT '');";

            rc = sqlite3_exec(db, sql.c_str(), 0, 0, &error_message);

            if (rc == SQLITE_OK)
            {
                otlog::log("OTDB: In-memory database tables created successfully.");
                return 0;
            }
            else
            {
                otlog::log("OTDB: Failed to create in-memory database tables. Return value..." + std::to_string(rc) + ", " + error_message);
                return -1;
            }
        }
    }

    int close()
    {
        /*
        Copies in-memory database to file-based database using Vaccum to tidy up structures,
        then closed in-memory database.

        */

        std::string sql;
        char *error_message = 0;
        int rc; // return values

        // remove any existing database file to allow Vacuum into it from memory based db
        remove(dbFileName.c_str());

        // save in memory database to file
        sql = "VACUUM INTO \'" + dbFileName + "\';";
        rc = sqlite3_exec(db, sql.c_str(), 0, 0, &error_message);

        if (rc == SQLITE_OK)
        {
            otlog::log("OTDB: Successfully copied in-memory database to file ( " + dbFileName + " ).");
            return 0;
        }
        else
        {
            otlog::log("OTDB: Failed to copy in-memory database to file ( " + dbFileName + " ). Return value..." + std::to_string(rc) + ", " + error_message);
            return -1;
        }

        // Close Database

        sqlite3_close(db);
        return 0;
    }

    /* Returns count of rows returned by last query */
    int rowCount()
    {
        return rows.size();
    }

    /* Runs query after first clearing the reponse matrix  */
    int query(std::string queryString)
    {
      
        clearResponseRows();
     
        int rc = sqlite3_exec(db, queryString.c_str(), callback, 0, &error_message);

        if (rc != SQLITE_OK)
        {
            otlog::log("OTDB: Failed to run query [ " + queryString + " ]. Return value... " + std::to_string(rc) + ", " + sqlite3_errstr(rc));
            return -1;
        }

        return 0;
    }

    /* returns value of nth column on specified row */
    std::string resultValue(int row, int colIndex)
    {

        // check bounds
        if (row<0 | row> rows.size() - 1)
        {
            return "";
        }

        if (colIndex<0 | colIndex> rows[row].size())
        {
            return "";
        }

        return rows[row][colIndex];
    }

    /* return value of named column on named row */
    std::string resultValue(int row, std::string columnName)
    {

        // check bounds
        if (row<0 | row> rows.size() - 1)
        {
            return "";
        }

        // check column name exists
        auto colIterator = std::find(colHeadings.begin(), colHeadings.end(), columnName);
        if (colIterator == colHeadings.end())
        {
            return "";
        }

        // concvert iterator to int
        int colIndex = std::distance(colHeadings.begin(), colIterator);
        return rows[row][colIndex];
    }

    // return string associated with enumeration e.g. "arp" for arp
    std::string getMacInfoString(MacInfo macInfo)
    {
        switch (macInfo)
        {
        case otdb::arp:
            return "arp";
            break;
        case otdb::ttl:
            return "ttl";
            break;
        default:
            return "";
            break;
        }
    }

}
