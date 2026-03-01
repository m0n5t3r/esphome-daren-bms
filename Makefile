# Makefile
CXX = g++
CXXFLAGS = -Icomponents/daren_bms -isystem../esphome -DTESTING=1 -std=c++20

# Compile daren_bms.cpp separately
daren_bms.o: components/daren_bms/daren_bms.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


# Compile daren_bms.cpp separately
protocol.o: components/daren_bms/protocol.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


# Compile test.cpp with daren_bms.o
test: tests.cpp daren_bms.o protocol.o
	$(CXX) $(CXXFLAGS) $^ -o $@
	./test


# Clean
clean:
	rm -f *.o test
