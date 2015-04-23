cmake_minimum_required(VERSION 3.1)
project(pollax)

set(INCLUDE_DIRECTORIES ../src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++14  -frtti -fno-inline -g3 -O0  -gdwarf-4 -fvar-tracking-assignments -femit-class-debug-always -fno-eliminate-unused-debug-types  -Wall -Werror -fvisibility=hidden -fvisibility-inlines-hidden ")

set(HEADER_FILES ../src/clock.hpp ../src/endians.hpp ../src/engine.hpp ../src/logger.hpp ../src/query.hpp ../src/resolver.hpp ../src/resolverimpl.hpp ../src/resouces.hpp ../src/semaphore.hpp ../src/socket.hpp ../src/stats.hpp ../src/syncvec.hpp ../src/tcpconn.hpp ../src/tcplistener.hpp ../src/tcpstream.hpp ../src/timeevent.hpp ../src/timers.hpp ../src/tlsclientwrapper.hpp ../src/tlscredentials.hpp ../src/tlstcpstream.hpp ../src/types.hpp ../src/udpsocket.hpp ../src/utils.hpp)

set(SOURCE_FILES ../src/clock.cpp ../src/enc_ocb.cpp ../src/engine.cpp ../src/logger.cpp ../src/main.cpp ../src/query.cpp ../src/resolver.cpp ../src/resolverimpl.cpp ../src/resouces.cpp ../src/semaphore.cpp ../src/socket.cpp ../src/stats.cpp ../src/tcpconn.cpp ../src/tcplistener.cpp ../src/tcpstream.cpp ../src/timers.cpp ../src/timevent.cpp ../src/tlsclientwrapper.cpp ../src/tlscredentials.cpp ../src/tlstcpstream.cpp ../src/udpsocket.cpp ../src/utils.cpp)

add_executable(pollax ${SOURCE_FILES} ${HEADER_FILES})
