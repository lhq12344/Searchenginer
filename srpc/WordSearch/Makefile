# Simple Makefile to build server.pb_skeleton executable
CXX = g++
# Build for debugging: disable optimizations and keep frame pointers for useful backtraces
CXXFLAGS = -std=c++17 -g -O0 -fno-omit-frame-pointer -I../../include -I.
# Optional: add custom lib dir, e.g. make LIBDIR=/usr/local/lib
LIBDIR ?=
# Libraries to link (override via make LIBS=... if needed)
LIBS ?= -lpthread -lprotobuf -lworkflow -lwfrest -lsrpc -lppconsul -lssl -lcrypto -llz4 -lsnappy -lz

# If LIBDIR is provided, add -L and rpath for that dir; otherwise just use LIBS.
ifneq ($(LIBDIR),)
LDFLAGS = -L$(LIBDIR) -Wl,-rpath,$(LIBDIR) $(LIBS)
else
LDFLAGS = $(LIBS)
endif

SRCS = server.pb_skeleton.cc ReadIndist.cpp wordsearch.pb.cc
# Convert .cc/.cpp sources to object names reliably
OBJS = $(patsubst %.cc,%.o,$(SRCS))
OBJS := $(patsubst %.cpp,%.o,$(OBJS))
TARGET = server_pb_skeleton

all: $(TARGET)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
