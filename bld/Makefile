GCC_CXX                = g++
CLANG_CXX              = clang++
GCC_CXXFLAGS           = -pipe -std=c++14 -pthread -fdiagnostics-color -flto -fuse-linker-plugin -Wall -Werror -O2 -march=haswell -mcx16 -msahf -mmovbe -maes -mpclmul -mpopcnt -mabm -mno-lwp -mfma -mno-fma4 -mno-xop -mbmi -mbmi2 -mno-tbm -mavx -mavx2 -msse4.2 -msse4.1 -mlzcnt -mrtm -mhle -mrdrnd -mf16c -mfsgsbase -mno-rdseed -mno-prfchw -mno-adx -mfxsr -mxsave -mxsaveopt --param l1-cache-size=32 --param l1-cache-line-size=64 --param l2-cache-size=8192 -mtune=haswell -fomit-frame-pointer -foptimize-register-move -fgcse-after-reload -fpredictive-commoning -ftree-loop-distribution -minline-all-stringops -fprefetch-loop-arrays -fno-tree-pre -fstrength-reduce -fexpensive-optimizations -floop-parallelize-all -ftree-parallelize-loops=8 -floop-interchange -Wall -Werror -fvisibility=hidden -fvisibility-inlines-hidden -fipa-reference -flra-remat -fipa-ra -fipa-reference -fipa-icf
CLANG_CXXFLAGS         = -pipe -std=c++14 -pthread -fdiagnostics-color -flto -Wall -Werror -O3 -march=haswell -mcx16 -maes -mpclmul -mpopcnt -mfma -mno-fma4 -mno-xop -mbmi -mbmi2 -mno-tbm -mavx -mavx2 -msse4.2 -msse4.1 -mlzcnt -mrtm -mrdrnd -mf16c -mfsgsbase -mno-rdseed -mno-prfchw -mno-adx -mtune=haswell -fomit-frame-pointer -minline-all-stringops -fvisibility=hidden -fvisibility-inlines-hidden 
GCC_COV_CXXFLAGS       = -pipe -std=c++14 -pthread -fdiagnostics-color -flto -Wall -Werror -O0 -g3 -gdwarf-4 -fno-eliminate-unused-debug-types -fprofile-arcs --coverage
GCC_LFLAGS             = $(GCC_CXXFLAGS) -ltcmalloc -s
CLANG_LFLAGS           = $(CLANG_CXXFLAGS) -ltcmalloc -s
GCC_COV_LFLAGS         = $(GCC_COV_CXXFLAGS) -ltcmalloc -lgcov

BUILD_DIR              = ../build

BIN_DIR                = $(BUILD_DIR)/bin
GCC_OBJS_DIR           = $(BUILD_DIR)/gccobjs
CLANG_OBJS_DIR         = $(BUILD_DIR)/clangobjs
COV_OBJS_DIR           = $(BUILD_DIR)/covobj
RESULTS_DIR            = $(BUILD_DIR)/results

INCPATH                = -I../src
SB_SRC_DIR             = ../src
SB_SRCS = \
clock.cpp counters.cpp enc_ocb.cpp engine.cpp event.cpp logger.cpp main.cpp query.cpp resolver.cpp resolverimpl.cpp resouces.cpp semaphore.cpp socket.cpp tcpconn.cpp tcplistener.cpp tcpstream.cpp timers.cpp tlsclientwrapper.cpp tlscredentials.cpp tlstcpstream.cpp udpsocket.cpp utils.cpp

PRODUCT                = sblade
GCC_OBJS               = ${SB_SRCS:%.cpp=$(GCC_OBJS_DIR)/%.o}
CLANG_OBJS             = ${SB_SRCS:%.cpp=$(CLANG_OBJS_DIR)/%.o}
COV_OBJS               = ${SB_SRCS:%.cpp=$(COV_OBJS_DIR)/%.o}
TARGET_GCC             = $(BIN_DIR)/$(PRODUCT).gcc
TARGET_CLANG           = $(BIN_DIR)/$(PRODUCT).clang
TARGET_COV             = $(BIN_DIR)/$(PRODUCT).coverage

default: $(TARGET_GCC) 

all: $(TARGET_GCC) $(TARGET_CLANG) coverage

coverage: $(RESULTS_DIR)/index.html

$(RESULTS_DIR)/index.html: $(TARGET_COV) | $(RESULTS_DIR)
	-rm -rf "$(RESULTS_DIR)/*"
	-"$(TARGET_COV)"
	lcov --external --base-directory .. --list-full-path --capture --derive-func-data --rc lcov_branch_coverage=1 --directory="$(COV_OBJS_DIR)" --output-file="$(RESULTS_DIR)/info.lcov"
	lcov --list "$(RESULTS_DIR)/info.lcov"
	lcov --remove "$(RESULTS_DIR)/info.lcov" "*g++*" -o "$(RESULTS_DIR)/info.lcov.filtered"
	genhtml --rc lcov_branch_coverage=1 --output-directory="$(RESULTS_DIR)" "$(RESULTS_DIR)/info.lcov.filtered"
	echo Coverage in: "$(RESULTS_DIR)/index.html"

$(GCC_OBJS_DIR)/%.o: $(SB_SRC_DIR)/%.cpp | $(GCC_OBJS_DIR)
	$(CLANG_CXX) -MMD --analyze -Xanalyzer -analyzer-output=text $(CLANG_CXXFLAGS) $(INCPATH) -o "$(@)".anal -c "$(<)"
	$(GCC_CXX) $(GCC_CXXFLAGS) $(INCPATH) -o "$(@)"  -c "$(<)"

$(CLANG_OBJS_DIR)/%.o: $(SB_SRC_DIR)/%.cpp | $(CLANG_OBJS_DIR)
	$(CLANG_CXX) -MMD $(CLANG_CXXFLAGS) $(INCPATH) -o "$(@)" -c "$(<)"

$(COV_OBJS_DIR)/%.o: $(SB_SRC_DIR)/%.cpp | $(COV_OBJS_DIR)
	$(GCC_CXX) -MMD  $(GCC_COV_CXXFLAGS) $(INCPATH) -o "$(@)" -c "$(<)"

$(TARGET_GCC): $(GCC_OBJS) | $(BIN_DIR)
	$(GCC_CXX) $(GCC_LFLAGS) -o "$(@)" $(GCC_OBJS)

$(TARGET_CLANG): $(CLANG_OBJS) | $(BIN_DIR)
	$(CLANG_CXX) $(CLANG_LFLAGS) -o "$(@)" $(CLANG_OBJS)

$(TARGET_COV): $(COV_OBJS) | $(BIN_DIR)
	$(GCC_CXX) $(GCC_COV_LFLAGS) -o "$(@)" $(COV_OBJS)

$(BUILD_DIR): 
	mkdir -p "$(@)"

$(GCC_OBJS_DIR): | $(BUILD_DIR)
	mkdir -p "$(@)"

$(CLANG_OBJS_DIR): | $(BUILD_DIR)
	mkdir -p "$(@)"

$(COV_OBJS_DIR): | $(BUILD_DIR)
	mkdir -p "$(@)"

$(RESULTS_DIR): | $(BUILD_DIR)
	mkdir -p "$(@)"

$(BIN_DIR): | $(BUILD_DIR)
	mkdir -p "$(@)"

clean:
	-rm -rf "$(RESULTS_DIR)" "$(COV_OBJS_DIR)" "$(GCC_OBJS_DIR)" "$(CLANG_OBJS_DIR)" "$(BIN_DIR)" "$(BUILD_DIR)"

.PHONY : all clean conv_run

-include $(SB_SRCS:%.cpp=$(SB_OBJS_DIR)/%.d)
