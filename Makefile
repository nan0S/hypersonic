TARGET = hypersonic

ALL_TARGETS := \
	hypersonic \
	arena

OBJECTS := \
	Common.o \
	Manager.o \
	GameState.o \
	Action.o \
	Agent.o \
	State.o \
	BeamSearch.o \
	MCTS.o \
	RHEA.o \
	Dummy.o \
	Arena.o

SRC_DIR := src
BUILD_DIR := build

CXX := g++ -std=c++17
OFLAGS := -Ofast -march=native -flto -fomit-frame-pointer -s -DNDEBUG
WFLAGS := -Wall -Wextra
DFLAGS := -ggdb -fsanitize=address
CXXFLAGS := $(IFLAGS) $(OFLAGS) $(WFLAGS)

OBJECTS := $(addprefix $(BUILD_DIR)/, $(OBJECTS))
DEPENDS := $(patsubst %.o, %.d, $(OBJECTS))
DEPENDS += $(BUILD_DIR)/$(TARGET).d
DEPENDS += $(BUILD_DIR)/arena.d

.PHONY: clean distclean

all: $(TARGET)

fight: arena
	./arena

debug: CXXFLAGS += $(DFLAGS)
debug: $(TARGET)

-include $(DEPENDS)

$(TARGET): $(OBJECTS) $(BUILD_DIR)/$(TARGET).o
	$(CXX) $(CXXFLAGS) -MMD -MP $^ -o $@

arena: $(OBJECTS) $(BUILD_DIR)/arena.o
	$(CXX) $(CXXFLAGS) -MMD -MP $^ -o $@

benchmark: $(OBJECTS) $(BUILD_DIR)/benchmark.o
	$(CXX) $(CXXFLAGS) -MMD -MP $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
distclean: clean
	rm -f $(TARGET) arena
