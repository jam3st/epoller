TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += link_pkgconfig
PKGCONFIG += botan


INCLUDEPATH += ../src
LIBS += -pthread
QMAKE_CXXFLAGS += -g3 --std=c++14 -Wall -Werror



SOURCES += \
	../src/main.cpp \
	../src/logger.cpp \
	../src/engine.cpp \
	../src/stats.cpp \
	../src/enc_ocb.cpp \
	../src/tcplistener.cpp \
	../src/utils.cpp \
	../src/tcpstream.cpp \
	../src/udpserver.cpp \
	../src/udpclient.cpp \
	../src/tlstcpstream.cpp \
	../src/tlscredentials.cpp \
	../src/tlsclientwrapper.cpp \
	../src/socket.cpp \
	../src/tcpconn.cpp \
    ../src/epoll.cpp \
    ../src/semaphore.cpp

HEADERS += \
	../src/types.hpp \
	../src/tlstcpstream.hpp \
	../src/tlsclientwrapper.hpp \
	../src/logger.hpp \
	../src/socket.hpp \
	../src/tcpstream.hpp \
	../src/tcplistener.hpp \
	../src/utils.hpp \
	../src/udpserver.hpp \
	../src/udpclient.hpp \
	../src/tlscredentials.hpp \
	../src/engine.hpp \
	../src/stats.hpp \
	../src/syncvec.hpp \
	../src/tcpconn.hpp \
    ../src/epoll.hpp \
    ../src/semaphore.hpp \
    ../src/engine.hpp
