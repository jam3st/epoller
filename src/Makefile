CXX           = g++
#CXX           = clang++
CXXFLAGS      = -pipe -std=c++14 -pthread -fdiagnostics-color -flto -fuse-linker-plugin -Wall -Werror -O2 -march=haswell -mcx16 -msahf -mmovbe -maes -mpclmul -mpopcnt -mabm -mno-lwp -mfma -mno-fma4 -mno-xop -mbmi -mbmi2 -mno-tbm -mavx -mavx2 -msse4.2 -msse4.1 -mlzcnt -mrtm -mhle -mrdrnd -mf16c -mfsgsbase -mno-rdseed -mno-prfchw -mno-adx -mfxsr -mxsave -mxsaveopt --param l1-cache-size=32 --param l1-cache-line-size=64 --param l2-cache-size=8192 -mtune=haswell -fomit-frame-pointer -foptimize-register-move -fgcse-after-reload -fpredictive-commoning -ftree-loop-distribution -minline-all-stringops -fprefetch-loop-arrays -fno-tree-pre -fstrength-reduce -fexpensive-optimizations -floop-parallelize-all -ftree-parallelize-loops=8 -floop-interchange -Wall -Werror -fvisibility=hidden -fvisibility-inlines-hidden -fipa-reference -flra-remat -fipa-ra -fipa-reference -fipa-icf
#CXXFLAGS      = -pipe -std=c++14 -pthread -fdiagnostics-color -flto -Wall -Werror -O2 -march=haswell -mcx16 -maes -mpclmul -mpopcnt -mfma -mno-fma4 -mno-xop -mbmi -mbmi2 -mno-tbm -mavx -mavx2 -msse4.2 -msse4.1 -mlzcnt -mrtm -mrdrnd -mf16c -mfsgsbase -mno-rdseed -mno-prfchw -mno-adx -mtune=haswell -fomit-frame-pointer -minline-all-stringops -fvisibility=hidden -fvisibility-inlines-hidden 
COV_CXXFLAGS  = -pipe -std=c++14 -pthread -fdiagnostics-color -flto -Wall -Werror -O0 -g3 -gdwarf-4 -fno-eliminate-unused-debug-types -fprofile-arcs --coverage
INCPATH       = -I../src
LINK          = $(CXX)
LFLAGS        = $(CXXFLAGS) -ltcmalloc -s
COV_LFLAGS    = $(COV_CXXFLAGS) -ltcmalloc -lgcov

BUILD_DIR     = ../build

BIN_DIR       = $(BUILD_DIR)/bin
SB_OBJS_DIR   = $(BUILD_DIR)/objs
COV_OBJS_DIR  = $(BUILD_DIR)/cobj
RESULTS_DIR   = $(BUILD_DIR)/results

SB_SRC_DIR    = .
SB_SRCS = \
clock.cpp counters.cpp enc_ocb.cpp engine.cpp event.cpp logger.cpp main.cpp query.cpp resolver.cpp resolverimpl.cpp resouces.cpp semaphore.cpp socket.cpp tcpconn.cpp tcplistener.cpp tcpstream.cpp timers.cpp tlsclientwrapper.cpp tlscredentials.cpp tlstcpstream.cpp udpsocket.cpp utils.cpp
SB_HDRS = \
clock.hpp counters.hpp endians.hpp engine.hpp event.hpp logger.hpp query.hpp resolver.hpp resolverimpl.hpp resouces.hpp semaphore.hpp socket.hpp syncqueue.hpp tcpconn.hpp tcplistener.hpp tcpstream.hpp timers.hpp tlsclientwrapper.hpp tlscredentials.hpp tlstcpstream.hpp types.hpp udpsocket.hpp utils.hpp

TEST_EXE    = sblade
SB_OBJS     = ${SB_SRCS:%.cpp=$(SB_OBJS_DIR)/%.o}
COV_OBJS    = ${SB_SRCS:%.cpp=$(COV_OBJS_DIR)/%.o}
TARGET      = $(BIN_DIR)/$(TEST_EXE)
COV_TARGET  = $(BIN_DIR)/$(TEST_EXE).coverage

all: $(TARGET)

coverage: $(RESULTS_DIR)/index.html

covreage_run: $(COV_TARGET)
	-$(COV_TARGET)

$(RESULTS_DIR)/index.html: covreage_run | $(RESULTS_DIR)
	-rm -rf "$(RESULTS_DIR)/*"
	lcov --no-external -b . --list-full-path   --capture --derive-func-data --rc lcov_branch_coverage=1 --directory "$(COV_OBJS_DIR)" --output-file=$(RESULTS_DIR)/info.lcov
	genhtml --rc lcov_branch_coverage=1 --output-directory=$(RESULTS_DIR) $(RESULTS_DIR)/info.lcov
	echo Coverage in: $(RESULTS_DIR)/index.html

##!/usr/bin/python
#import re
#def remove_user(dst_file_name, src_file_name):
#    inf = open(src_file_name, 'r')
#    out = open(dst_file_name, 'w')
#    output = True
#    inline = inf.readline()
#    while inline:
#        if re.match('SF:/usr/lib/gcc/', inline):
#            output = False;
#        if output:
#            out.write(inline)
#        if re.match('end_of_record', inline):
#            output = True;
#        inline = inf.readline()
#    inf.close()
#    out.close()
#if __name__ == '__main__':
#    import sys
#    remove_user(sys.argv[1], sys.argv[2])


$(SB_OBJS_DIR)/%.o: $(SB_SRC_DIR)/%.cpp | $(SB_OBJS_DIR)
	$(CXX) -MMD $(INCPATH) -o "$@"  -c "$<" $(CXXFLAGS)

$(COV_OBJS_DIR)/%.o: $(SB_SRC_DIR)/%.cpp | $(COV_OBJS_DIR)
	$(CXX) -MD  -c $(COV_CXXFLAGS) $(INCPATH) -o "$@" -c "$<"
#	-rm -f "$(COV_OBJS_DIR)/$<"
#	cp "$<" "$(COV_OBJS_DIR)"
#	$(CXX) -MD  -c $(COV_CXXFLAGS) $(INCPATH) -o "$@" -c "$(COV_OBJS_DIR)/$<"


$(TARGET): $(SB_OBJS) | $(BIN_DIR)
	$(LINK) $(LFLAGS) -o "$@" $(SB_OBJS)

$(COV_TARGET): $(COV_OBJS) | $(BIN_DIR)
	$(LINK) $(COV_LFLAGS) -o "$@" $(COV_OBJS)

$(BUILD_DIR): 
	mkdir -p "$@"

$(SB_OBJS_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

$(COV_OBJS_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

$(RESULTS_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

$(BIN_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

clean:
	-rm -rf "$(RESULTS_DIR)" "$(COV_OBJS_DIR)" "$(SB_OBJS_DIR)" "$(BIN_DIR)" "$(BUILD_DIR)"

.PHONY : all clean

-include $(SB_SRCS:%.cpp=$(SB_OBJS_DIR)/%.d)
