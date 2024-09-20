# Makefile for Single Server-Client Application

# Compiler
CXX = g++
CXXFLAGS = -pthread

# Source files
SERVER_SRC = server.cpp
CLIENT_SRC = client.cpp

# Binary names
SERVER_BIN = server
CLIENT_BIN = client

# Default target
.PHONY: all clean

all: $(SERVER_BIN) $(CLIENT_BIN)

# Compile server
$(SERVER_BIN): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Compile client
$(CLIENT_BIN): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Clean up
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)

