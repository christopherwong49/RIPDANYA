SRCS := $(shell find . -name '*.cpp')
OBJS := $(SRCS:.cpp=.o)

EVALFILE ?= nnue.bin

HDRS := $(find . -name '*.hpp')

CXX := g++
CXXFLAGS := -std=c++20 -m64 -DNNUE_PATH=\"$(EVALFILE)\"

RELEASEFLAGS := -O3 -march=native
DEBUGFLAGS := -g -O0 -march=x86-64-v3

release: CXXFLAGS += $(RELEASEFLAGS)
release: ripdanya

debug: CXXFLAGS += $(DEBUGFLAGS)
debug: ripdanya

ripdanya: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) ripdanya
