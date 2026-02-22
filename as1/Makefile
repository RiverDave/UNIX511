CXX = g++
CXXFLAGS = -Wall -Wextra

TARGET = as1

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp

clean:
	rm -f $(TARGET) worker1.bin worker2.bin worker3.bin syslog.log
