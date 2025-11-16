SRCS := $(shell find . -name '*.cpp')
HDRS := $(shell find . -name '*.hpp')
OBJS := $(SRCS:.cpp=.o)

EXE ?= ripdanya
EVALFILE ?= nnue.bin

HDRS := $(find . -name '*.hpp')

CXX := g++
CXXFLAGS := -std=c++20 -m64 -flto=auto -DNNUE_PATH=\"$(EVALFILE)\"

RELEASEFLAGS := -O3 -march=native
DEBUGFLAGS := -g -O0 -march=native

release: CXXFLAGS += $(RELEASEFLAGS)
release: $(EXE)

debug: CXXFLAGS += $(DEBUGFLAGS)
debug: $(EXE)

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(HDRS) Makefile
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(EXE)
