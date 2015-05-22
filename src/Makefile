CXX           = g++
CFLAGS        = -pipe -fdiagnostics-color -flto -fuse-linker-plugin -O2 -march=haswell -mcx16 -msahf -mmovbe -maes -mpclmul -mpopcnt -mabm -mno-lwp -mfma -mno-fma4 -mno-xop -mbmi -mbmi2 -mno-tbm -mavx -mavx2 -msse4.2 -msse4.1 -mlzcnt -mrtm -mhle -mrdrnd -mf16c -mfsgsbase -mno-rdseed -mno-prfchw -mno-adx -mfxsr -mxsave -mxsaveopt --param l1-cache-size=32 --param l1-cache-line-size=64 --param l2-cache-size=8192 -mtune=haswell -fomit-frame-pointer -foptimize-register-move -fgcse-after-reload -fpredictive-commoning -ftree-loop-distribution -minline-all-stringops -fprefetch-loop-arrays -fno-tree-pre -fstrength-reduce -fexpensive-optimizations -floop-parallelize-all -floop-parallelize-all -ftree-parallelize-loops=8 -floop-interchange -Wall -Werror -fvisibility=hidden -fvisibility-inlines-hidden -fno-semantic-interposition -fipa-reference -flra-remat -fipa-ra -fipa-reference -fipa-icf 
CXXFLAGS      = $(CFLAGS) -std=c++14 -pthread
INCPATH       = -I../src
LINK          = $(CXX) -s
LFLAGS        = $(CXXFLAGS) -ltcmalloc
LIBS          = 

BUILD_DIR     = ../build

BIN_DIR       = $(BUILD_DIR)/bin
SB_OBJS_DIR = $(BUILD_DIR)/objs

SB_SRC_DIR   = .

SB_SRCS = main.cpp \
clock.cpp    event.cpp         query.cpp         resouces.cpp   tcpconn.cpp      timers.cpp            tlstcpstream.cpp \
enc_ocb.cpp  logger.cpp        resolver.cpp      semaphore.cpp  tcplistener.cpp  tlsclientwrapper.cpp  udpsocket.cpp \
engine.cpp   resolverimpl.cpp  socket.cpp     tcpstream.cpp    tlscredentials.cpp    utils.cpp

SB_HDRS = \
clock.hpp    logger.hpp        resouces.hpp   tcpconn.hpp      tlsclientwrapper.hpp  udpsocket.hpp \
endians.hpp  query.hpp         semaphore.hpp  tcplistener.hpp  tlscredentials.hpp    utils.hpp \
engine.hpp   resolver.hpp      socket.hpp     tcpstream.hpp    tlstcpstream.hpp \
event.hpp    resolverimpl.hpp  syncqueue.hpp  timers.hpp       types.hpp


TEST_EXE    = sblade
SB_OBJS     = ${SB_SRCS:%.cpp=$(SB_OBJS_DIR)/%.o}
TARGET      = $(BIN_DIR)/$(TEST_EXE)

all: $(TARGET) $(COV_OUTS)

$(SB_OBJS_DIR)/%.o: $(SB_SRC_DIR)/%.cpp | $(SB_OBJS_DIR)
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
