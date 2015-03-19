TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += link_pkgconfig
#PKGCONFIG += botan


INCLUDEPATH += ../src
LIBS += -pthread
QMAKE_CXXFLAGS_DEBUG = -frtti -fstandalone-debug  -fno-inline -gdwarf-4  -g3 -O0
QMAKE_CXXFLAGS_RELEASE = -flto -O3
QMAKE_CXXFLAGS = -pipe --std=c++14 -Wall -Werror -fvisibility=hidden -fvisibility-inlines-hidden -fintegrated-as

QMAKE_LFLAGS_DEBUG += -ggdb -g3 -O0
QMAKE_LFLAGS_RELEASE += -flto -O3 -s


SOURCES += \
	../src/main.cpp \
	../src/logger.cpp \
	../src/engine.cpp \
	../src/stats.cpp \
	../src/enc_ocb.cpp \
	../src/tcplistener.cpp \
	../src/utils.cpp \
	../src/tcpstream.cpp \
	../src/tlstcpstream.cpp \
	../src/tlscredentials.cpp \
	../src/tlsclientwrapper.cpp \
	../src/socket.cpp \
	../src/tcpconn.cpp \
	../src/semaphore.cpp \
	../src/timers.cpp \
	../src/timevent.cpp \
	../src/resouces.cpp \
	../src/udpsocket.cpp \
	../src/query.cpp \
	../src/clock.cpp \
	../src/resolverimpl.cpp \
	../src/resolver.cpp

HEADERS += \
	../src/types.hpp \
	../src/tlstcpstream.hpp \
	../src/tlsclientwrapper.hpp \
	../src/logger.hpp \
	../src/socket.hpp \
	../src/tcpstream.hpp \
	../src/tcplistener.hpp \
	../src/utils.hpp \
	../src/tlscredentials.hpp \
	../src/engine.hpp \
	../src/stats.hpp \
	../src/syncvec.hpp \
	../src/tcpconn.hpp \
	../src/semaphore.hpp \
	../src/engine.hpp \
	../src/timeevent.hpp \
	../src/timers.hpp \
	../src/resouces.hpp \
	../src/udpsocket.hpp \
	../src/query.hpp \
	../src/clock.hpp \
	../src/resolver.hpp \
	../src/resolverimpl.hpp
