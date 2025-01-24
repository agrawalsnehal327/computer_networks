# Compiler and flags
CXX = g++
CXXFLAGS = --std=c++20 -Wall -Wextra

# Targets
TARGETS = compareclient client server server_compare

# Source files
SRCS = client_compare_tcp_udp.cpp client.cpp server.cpp server_compare_tcp_udp.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default rule
all: $(TARGETS)

# Rules for each target
compareclient: client_compare_tcp_udp.o
	$(CXX) $(CXXFLAGS) -o $@ $^

client: client.o
	$(CXX) $(CXXFLAGS) -o $@ $^

server: server.o
	$(CXX) $(CXXFLAGS) -o $@ $^

server_compare: server_compare_tcp_udp.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to clean build files
clean:
	rm -f $(TARGETS) $(OBJS)

# Rule for object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

# Phony targets
.PHONY: all clean

