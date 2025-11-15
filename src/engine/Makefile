SRCS := $(shell find . -name '*.cpp')
OBJS := $(SRCS:.cpp=.o)

EXE ?= ripdanya
EVALFILE ?= nnue.bin

HDRS := $(find . -name '*.hpp')

CXX := g++
CXXFLAGS := -std=c++20 -m64 -DNNUE_PATH=\"$(EVALFILE)\"

RELEASEFLAGS := -O3 -march=native
DEBUGFLAGS := -g -O0 -march=x86-64-v3

release: CXXFLAGS += $(RELEASEFLAGS)
release: $(EXE)

debug: CXXFLAGS += $(DEBUGFLAGS)
debug: $(EXE)

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(EXE)
