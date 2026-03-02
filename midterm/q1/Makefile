// This is all pretty self explanatory
CXX = g++
CXXFLAGS = -Wall -Wextra

// running snip would toggle the below compilation cmds
all: snip

// use g++ as compiler and append flags output binary from snip.cpp
snip: snip.cpp
	$(CXX) $(CXXFLAGS) -o snip snip.cpp

clean:
	rm -f snip log.txt
