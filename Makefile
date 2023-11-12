CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -pedantic -O3 -g
CC = clang
CCFLAGS = -std=gnu11 -Wall -Wextra -pedantic -g
INCLUDES = -I src -I lib/cxxopts

PROFILE_LINK = -L /usr/lib/x86_64-linux-gnu -lunwind -lunwind-x86_64 -lunwind-ptrace
TEST_LINK = -lunwind -lunwind-x86_64 -lunwind-ptrace

bandwidth: src/bandwidth.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) src/bandwidth.cpp -o build/bandwidth

profile: src/profile.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(PROFILE_LINK) src/profile.cpp -o build/profile

test: src/test.c
	$(CC) $(CCFLAGS) $(INCLUDES) $(TEST_LINK) src/test.c -o build/test

clean:
	rm -rf build/*