CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pthread -finput-charset=utf-8 -fexec-charset=utf-8
LDFLAGS  ?= -pthread

SRCS = log_crypto.cpp metrics.cpp log_writer.cpp logger.cpp \
       test_business.cpp benchmark.cpp main.cpp
OBJS = $(addprefix $(BIN_DIR)/, $(SRCS:.cpp=.o))
DEPS = $(wildcard *.h)
BIN_DIR = bin
TARGET  = $(BIN_DIR)/logsys

all: $(BIN_DIR) $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

$(BIN_DIR)/%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) run

test: $(TARGET)
	./$(TARGET) test

bench: $(TARGET)
	./$(TARGET) bench

clean:
	rm -rf $(BIN_DIR) *.o *.exe
	rm -rf logs bench_logs* test_logs_*

.PHONY: all run test bench clean
