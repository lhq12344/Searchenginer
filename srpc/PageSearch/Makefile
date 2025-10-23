PKG_CFLAGS := $(shell pkg-config --cflags protobuf srpc wfrest 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs protobuf srpc wfrest 2>/dev/null)

CXX ?= g++
CXXFLAGS := -std=c++17 -O2 -g -Wall -I..
LDFLAGS :=
LDLIBS := $(if $(PKG_LIBS),$(PKG_LIBS),-lprotobuf -lsrpc -lwfrest -lworkflow -lssl -lcrypto -lpthread -lz -llz4 -lsnappy -lppconsul -lfasttext -lfaiss -fopenmp -lopenblas -llapack)

SRCS := $(wildcard *.cc)
OBJS := $(SRCS:.cc=.o)

ALL := pagesuggest_server

.PHONY: all clean
all: $(ALL)

pagesuggest_server: pagesearch.pb.o server.pb_skeleton.o ReadPage.o cut_weight.o tinyxml2.o
	$(CXX) $^ -o $@ $(LDFLAGS) $(LDLIBS)


%.o: %.cc
	$(CXX) $(CXXFLAGS) $(PKG_CFLAGS) -c $< -o $@

# Build tinyxml2 (project-local copy) for linking
tinyxml2.o: ../../src/tinyxml2.cpp
	$(CXX) $(CXXFLAGS) -I../../include -c ../../src/tinyxml2.cpp -o tinyxml2.o

clean:
	rm -f $(OBJS) $(ALL) pagesearch.pb.o server.pb_skeleton.o ReadPage.o cut_weight.o tinyxml2.o

# Usage:
# make        # build server
# make clean  # remove artifacts