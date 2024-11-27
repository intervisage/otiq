cmake_minimum_required(VERSION 3.15)

project(otiq)

message("Creating make file for project " ${PROJECT_NAME})


# Set C++11
set(CMAKE_CXX_STANDARD 11)


# flags for gprof
#message("Compiling and linking flags set for gprof")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
#SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
#SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")

#message("Project Source Directory =" ${PROJECT_SOURCE_DIR} )
string(CONCAT PCPP_LIB_DIR ${PROJECT_SOURCE_DIR} "/libs/pcapplusplus-23.09/lib")
string(CONCAT PCPP_CMAKE_DIR ${PCPP_LIB_DIR} "/cmake/pcapplusplus")
string(CONCAT SQLITE3_LIB_DIR ${PROJECT_SORCE_DIR} "/libs/sqlite3")
#string(CONCAT PCPP_HDR_DIR ${PCPP_LIB_DIR} "/include/pcapplusplus")
#string(CONCAT INCLUDE_DIR ${PROJECT_SOURCE_DIR} "/include")

# message("PcapPlusPlus Library Location =" ${PCPP_LIB_DIR})
# message("PcapPlusPlus CMake Location =" ${PCPP_CMAKE_DIR})
# message("PcapPlusPlus Headers Location =" ${PCPP_HDR_DIR})


set(CMAKE_PREFIX_PATH ${PCPP_CMAKE_DIR})


#message("CMake Prefix Path = " ${CMAKE_PREFIX_PATH})

find_package(PcapPlusPlus REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp src/sqlite3.c src/otlog.cpp src/otdb.cpp src/otbw.cpp src/otpp.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_link_libraries(${PROJECT_NAME} PUBLIC PcapPlusPlus::Pcap++)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)








