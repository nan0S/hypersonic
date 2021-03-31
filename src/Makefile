TARGET = hypersonic

OBJS = Common.o \
	Manager.o \
	GameState.o \
	Action.o \
	Agent.o \
	State.o \
	BeamSearch.o \
	MCTS.o \
	RHEA.o \
	Dummy.o

CXX = g++
CXXFLAGS = -std=c++17 -DLOCAL -Wall -Wextra -Wreorder -Ofast -O3 -flto -march=native -s
DFLAGS = -ggdb -fsanitize=address -fsanitize=undefined
RFLAGS = -DNDEBUG

all: $(TARGET)

release: CXXFLAGS += $(RFLAGS)
release: $(TARGET)

debug: CXXFLAGS += $(DFLAGS)
debug: $(TARGET)

$(TARGET): $(OBJS) main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

arena: $(OBJS) Arena.o arena.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

fight: arena
	./arena

%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f *.o
distclean: clean
	rm -f $(TARGET) fight
