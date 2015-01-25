CXX              = g++
LINKER           = $(CXX)
MAKEDEPEND       = makedepend
#TCMALLOC_PROFILE = -lprofiler 
#CFLAGS           = -Wall -Werror -ggdb $(TCMALLOC_CFLAGS) -pipe
TCMALLOC_MINIMAL = _minimal
TCMALLOC_CFLAGS  =  -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
CFLAGS           = -Wall -Werror -O2 -fomit-frame-pointer -foptimize-register-move -fgcse-after-reload -fpredictive-commoning -ftree-loop-distribution -minline-all-stringops -fprefetch-loop-arrays -fno-tree-pre -fstrength-reduce -fexpensive-optimizations $(TCMALLOC_CFLAGS) -pipe
CXXFLAGS         = $(CFLAGS) -std=c++11 -fexceptions
INCPATH          = -I/mnt/data/tmp/stuff/include/botan-1.11
LFLAGS           = $(CFLAGS) -Wl,-rpath,/mnt/data/tmp/stuff/lib/
LIBS             = $(TCMALLOC_PROFILE) -ltcmalloc$(TCMALLOC_MINIMAL) -lpthread -L/mnt/data/tmp/stuff/lib -lbotan-1.11 -lrt

BUILD_DIR        = ../build

BIN_DIR          = $(BUILD_DIR)/bin
OBJS_DIR         = $(BUILD_DIR)/objs
SRC_DIR          = .

SRCS             = enc_ocb.cpp engine.cpp epollable.cpp logger.cpp main.cpp stats.cpp tcpconnector.cpp tcplistener.cpp tcpstream.cpp timer.cpp tlsclientwrapper.cpp tlscredentials.cpp tlstcpstream.cpp udpclient.cpp udpserver.cpp utils.cpp
OBJS             = ${SRCS:%.cpp=$(OBJS_DIR)/%.o}
TARGET           = $(BIN_DIR)/switchblade

$(OBJS_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJS_DIR)
	$(CXX) -MD  -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

$(TARGET):  $(OBJS) | $(BIN_DIR)
	$(LINKER) $(LFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

$(OBJS_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

$(BIN_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

$(BUILD_DIR):
	mkdir -p "$@"

all: $(TARGET)

clean:
	-rm -rf $(OBJS_DIR) $(BIN_DIR)

.PHONY : clean all

-include $(SRCS:%.cpp=$(OBJS_DIR)/%.d)
