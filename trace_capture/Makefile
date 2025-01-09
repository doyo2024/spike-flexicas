
MODE ?=
NCORE ?= `nproc`

MAKE = make
CXX = g++

# c++17 + concept by default, c++20 would work as well, spike enforce 20
CXXSTD = --std=c++2a

CXXFLAGS  = $(CXXSTD) -O2 -I. -fPIC

all: libtraceGen.a

.PONY: all

HEADERS  = $(wildcard *.hpp)
TRACE_OBJS = traceGen.o

$(TRACE_OBJS) : %o:%cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

libtraceGen.a: $(TRACE_OBJS) 
	ar rvs $@ $(TRACE_OBJS)

clean:
	rm $(TRACE_OBJS)
	rm libtraceGen.a

.PHONY: clean
