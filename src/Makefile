CXX           = clang++
CFLAGS        = -pipe -flto -O3 -Wall -Werror -fvisibility=hidden -fvisibility-inlines-hidden
CXXFLAGS      = $(CFLAGS) -std=c++14 -fexceptions -pthread
INCPATH       = -I../src `pkg-config --cflags botan`
LINK          = $(CXX) -s
LFLAGS        = $(CXXFLAGS)
LIBS          = `pkg-config --libs  botan`

BUILD_DIR     = ../build

BIN_DIR       = $(BUILD_DIR)/bin
SB_OBJS_DIR = $(BUILD_DIR)/objs

SB_SRC_DIR   = .

SB_SRCS = main.cpp \
		clock.cpp logger.cpp engine.cpp stats.cpp enc_ocb.cpp tcplistener.cpp utils.cpp tcpstream.cpp \
		tlstcpstream.cpp tlscredentials.cpp tlsclientwrapper.cpp socket.cpp tcpconn.cpp semaphore.cpp \
		timers.cpp timevent.cpp resouces.cpp udpsocket.cpp query.cpp  resolverimpl.cpp  resolver.cpp
SB_HDRS = \
		clock.hpp engine.hpp logger.hpp query.hpp resolver.hpp resolverimpl.hpp resouces.hpp \
		semaphore.hpp socket.hpp stats.hpp syncvec.hpp tcpconn.hpp tcplistener.hpp tcpstream.hpp \
		timeevent.hpp timers.hpp tlsclientwrapper.hpp tlscredentials.hpp tlstcpstream.hpp types.hpp \
		udpsocket.hpp utils.hpp

TEST_EXE    = switchblade
SB_OBJS     = ${SB_SRCS:%.cpp=$(SB_OBJS_DIR)/%.o}
TARGET      = $(BIN_DIR)/$(TEST_EXE)

all: $(TARGET) $(COV_OUTS)

$(SB_OBJS_DIR)/%.o: $(SB_SRC_DIR)/%.cpp | $(SB_OBJS_DIR)
	$(CXX) -c $(CXXFLAGS) $(INCPATH) --analyze -o "$@".plist "$<"
	$(CXX) -MMD -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

$(TARGET):  $(SB_OBJS) $(SB_OBJS) | $(BIN_DIR)
	$(LINK) $(LFLAGS) -o $(TARGET) $(SB_OBJS) $(LIBS)

$(BUILD_DIR): 
	mkdir -p "$@"

$(SB_OBJS_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

$(BIN_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

clean:
	-rm -rf "$(RESULTS_DIR)" "$(COV_OBJS_DIR)" "$(SB_OBJS_DIR)" "$(BIN_DIR)"

.PHONY : all clean

-include $(SB_SRCS:%.cpp=$(SB_OBJS_DIR)/%.d)
