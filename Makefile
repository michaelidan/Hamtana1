# =============================================
# Email: michael9090124@gmail.com
# Makefile — v4 (Console + GUI via SFML)
# =============================================
CXX := g++
CXXFLAGS := -std=gnu++17 -O2 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS := 

SRC_ALL := $(wildcard src/*.cpp)
SRC_CORE := $(filter-out src/Main.cpp src/GuiMain.cpp,$(SRC_ALL))
OBJ_CORE := $(SRC_CORE:.cpp=.o)

.PHONY: all Main run gui test valgrind clean

all: run

Main: $(OBJ_CORE) src/Main.cpp
	$(CXX) $(CXXFLAGS) -o Main src/Main.cpp $(OBJ_CORE) $(LDFLAGS)

run: Main
	./Main

Gui: $(OBJ_CORE) src/GuiMain.cpp
	$(CXX) $(CXXFLAGS) -o Gui src/GuiMain.cpp $(OBJ_CORE) -lsfml-graphics -lsfml-window -lsfml-system

gui: Gui
	./Gui

tests/test_runner: $(OBJ_CORE) tests/test_core.cpp
	$(CXX) $(CXXFLAGS) -o tests/test_runner tests/test_core.cpp $(OBJ_CORE) $(LDFLAGS)

test: tests/test_runner
	./tests/test_runner

valgrind: Main
	valgrind --leak-check=full ./Main || true

clean:
	rm -f $(OBJ_CORE) Main Gui tests/test_runner
