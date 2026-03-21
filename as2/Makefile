CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic

TARGETS := networkMonitor interfaceMonitor

.PHONY: all clean

all: $(TARGETS)

networkMonitor: networkMonitor.cpp common.h
	$(CXX) $(CXXFLAGS) -o $@ networkMonitor.cpp

interfaceMonitor: interfaceMonitor.cpp common.h
	$(CXX) $(CXXFLAGS) -o $@ interfaceMonitor.cpp

clean:
	rm -f $(TARGETS)
