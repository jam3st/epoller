CXX           = g++
MAKEDEPEND    = makedepend
CFLAGS        = -Wall -Werror -g -ggdb -fprofile-arcs --coverage
CXXFLAGS      = $(CFLAGS) -std=c++11 -fexceptions
INCPATH       = -I../src
LINK          = $(CXX)
LFLAGS        = $(CFLAGS)
LIBS          = -lpthread -lgcov

BUILD_DIR     = ../build

BIN_DIR       = $(BUILD_DIR)/tbin
COV_OBJS_DIR  = $(BUILD_DIR)/cobjs
TEST_OBJS_DIR = $(BUILD_DIR)/tobjs
RESULTS_DIR   = $(BUILD_DIR)/results

TEST_SRC_DIR  = .
TEST_SRCS     = main.cpp 

COV_SRC_DIR   = ../src
include ../src/sbsrcs.txt
COV_SRCS      = $(SB_SRCS)

TEST_OBJS     = ${TEST_SRCS:%.cpp=$(TEST_OBJS_DIR)/%.o}
COV_OBJS      = ${COV_SRCS:%.cpp=$(COV_OBJS_DIR)/%.o}
TARGET        = $(BIN_DIR)/sbtest

$(COV_OBJS_DIR)/%.o: $(COV_SRC_DIR)/%.cpp | $(COV_OBJS_DIR)
	-rm -f "$(COV_OBJS_DIR)/$<"
	cp "$<" "$(COV_OBJS_DIR)"
	$(CXX) -MD  -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

$(TEST_OBJS_DIR)/%.o: $(TEST_SRC_DIR)/%.cpp | $(TEST_OBJS_DIR)
	$(CXX) -MD  -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

$(TARGET):  $(TEST_OBJS) $(COV_OBJS) | $(BIN_DIR)
	$(LINK) $(LFLAGS) -o $(TARGET) $(TEST_OBJS) $(COV_OBJS) $(LIBS)

$(RESULTS_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

$(TEST_OBJS_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

$(COV_OBJS_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

$(BIN_DIR): | $(BUILD_DIR)
	mkdir -p "$@"

all: $(TARGET) coverage

clean:
	-rm -rf "$(RESULTS_DIR)" "$(COV_OBJS_DIR)" "$(TEST_OBJS_DIR)" "$(BIN_DIR)"

clean-coverage:
	-rm -rf $(RESULTS_DIR)

coverage: $(RESULTS_DIR)/index.html

$(RESULTS_DIR)/index.html: $(TARGET) | $(RESULTS_DIR)
	-rm -rf "$(RESULTS_DIR)/*"
	lcov --directory "$(COV_OBJS_DIR)" --zerocounters --rc lcov_branch_coverage=1 --derive-func-data
	-valgrind --leak-check=full --show-leak-kinds=all --xml=yes --xml-file=$(RESULTS_DIR)/valgrind.xml -v "$(TARGET)"
	lcov --capture --derive-func-data --rc lcov_branch_coverage=1 --directory "$(COV_OBJS_DIR)" --output-file=$(RESULTS_DIR)/info.lcov
	./filter-lcov.py $(RESULTS_DIR)/info.lcov.filtered $(RESULTS_DIR)/info.lcov
	genhtml --rc lcov_branch_coverage=1 --output-directory=$(RESULTS_DIR) $(RESULTS_DIR)/info.lcov.filtered
	xdg-open $(RESULTS_DIR)/index.html
	xdg-open $(RESULTS_DIR)/valgrind.xml

.PHONY : all clean coverage clean-coverage

-include $(TEST_SRCS:%.cpp=$(TEST_OBJS_DIR)/%.d)
-include $(COV_SRCS:%.cpp=$(COV_OBJS_DIR)/%.d)
