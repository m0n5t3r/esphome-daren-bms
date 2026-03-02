# Makefile
CXX = g++
CXXFLAGS = -Icomponents/daren_bms -isystem../esphome -DTESTING=1 -std=c++20

protocol.o: components/daren_bms/protocol.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


test: tests.cpp protocol.o
	$(CXX) $(CXXFLAGS) $^ -o $@
	./test


# Clean
clean:
	rm -f *.o test
