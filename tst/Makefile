CXX           = clang++
CFLAGS        = -pipe -Wall -Werror -fvisibility=hidden -O0 -g -fprofile-instr-generate -fcoverage-mapping
CXXFLAGS      = $(CFLAGS) -std=c++14 -fexceptions -pthread
INCPATH       = -I../src `pkg-config --cflags botan`
LINK          = $(CXX)
LFLAGS        = $(CXXFLAGS)
LIBS          = `pkg-config --libs  botan`

BUILD_DIR     = ../build

BIN_DIR       = $(BUILD_DIR)/tbin
TEST_OBJS_DIR = $(BUILD_DIR)/tobjs
RESULTS_DIR   = $(BUILD_DIR)/results

TEST_SRC_DIR  = .
TEST_SRCS     = main.cpp

SB_SRC_DIR   = ../src

SB_SRCS = \
		clock.cpp logger.cpp engine.cpp stats.cpp enc_ocb.cpp tcplistener.cpp utils.cpp tcpstream.cpp \
		tlstcpstream.cpp tlscredentials.cpp tlsclientwrapper.cpp socket.cpp tcpconn.cpp semaphore.cpp \
		timers.cpp timevent.cpp resouces.cpp udpsocket.cpp query.cpp  resolverimpl.cpp  resolver.cpp
SB_HDRS = \
		clock.hpp engine.hpp logger.hpp query.hpp resolver.hpp resolverimpl.hpp resouces.hpp \
		semaphore.hpp socket.hpp stats.hpp syncvec.hpp tcpconn.hpp tcplistener.hpp tcpstream.hpp \
		timeevent.hpp timers.hpp tlsclientwrapper.hpp tlscredentials.hpp tlstcpstream.hpp types.hpp \
		udpsocket.hpp utils.hpp

TEST_EXE    = sbtest
TEST_OBJS     = ${TEST_SRCS:%.cpp=$(TEST_OBJS_DIR)/%.o} 
SB_OBJS       = ${SB_SRCS:%.cpp=$(TEST_OBJS_DIR)/%.o}
COV_OUTS      = ${SB_SRCS:%=$(RESULTS_DIR)/%.cov} ${SB_HDRS:%=$(RESULTS_DIR)/%.cov}
TARGET        = $(BIN_DIR)/$(TEST_EXE)

all: $(TARGET) $(COV_OUTS)

$(TEST_OBJS_DIR)/%.o: $(SB_SRC_DIR)/%.cpp | $(TEST_OBJS_DIR)
	$(CXX) -c $(CXXFLAGS) $(INCPATH) --analyze -o "$@".plist "$<"
	$(CXX) -MMD -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

$(TEST_OBJS_DIR)/%.o: $(TEST_SRC_DIR)/%.cpp | $(TEST_OBJS_DIR)
	$(CXX) -c $(CXXFLAGS) $(INCPATH) --analyze -o "$@".plist "$<"
	$(CXX) -MMD -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

$(TARGET):  $(TEST_OBJS) $(SB_OBJS) | $(BIN_DIR)
	$(LINK) $(LFLAGS) -o $(TARGET) $(TEST_OBJS) $(SB_OBJS) $(LIBS)

$(BUILD_DIR): 
	mkdir -p "$@"

$(RESULTS_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

$(TEST_OBJS_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

$(BIN_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

clean:
	-rm -rf "$(RESULTS_DIR)" "$(COV_OBJS_DIR)" "$(TEST_OBJS_DIR)" "$(BIN_DIR)"

clean-coverage:
	-rm -rf $(RESULTS_DIR)

$(RESULTS_DIR)/%.cov: $(SB_SRC_DIR)/% $(BIN_DIR)/$(TEST_EXE).profdata | $(RESULTS_DIR)
	llvm-cov show $(TARGET) -color -show-line-counts-or-regions -instr-profile="$(TARGET).profdata" "$<" > "$@"
	llvm-cov report $(TARGET) -color -instr-profile="$(TARGET).profdata" "$<" >> "$@"

$(BIN_DIR)/$(TEST_EXE).profdata: $(BIN_DIR)/default.profraw 
	llvm-profdata merge -o "$@" "$<"

$(BIN_DIR)/default.profraw: $(TARGET)
	cd $(BIN_DIR) && ./$(TEST_EXE)

.PHONY : all clean coverage clean-coverage

-include $(TEST_SRCS:%.cpp=$(TEST_OBJS_DIR)/%.d)
