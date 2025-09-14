# Emails: ronavraham1999@gmail.com_michael9090124@gmail.com
# Makefile — Coup v5 (tests always run with vendored doctest)

CXX      ?= g++
CXXFLAGS ?= -std=gnu++17 -O2 -Wall -Wextra -Wpedantic -Iinclude -Itests

OBJS := src/Game.o src/Player.o

.PHONY: all run gui Gui test valgrind clean

# ברירת מחדל: לבנות את הדמו הקונסולי
all: Main
	@echo "Built Main."
	@echo "Run demo: make run"
	@echo "GUI (optional): make gui"
	@echo "Tests: make test"

# ליבת המשחק
src/Game.o: src/Game.cpp include/Game.hpp include/Player.hpp include/Role.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

src/Player.o: src/Player.cpp include/Player.hpp include/Role.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# דמו קונסולי
Main: src/Main.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

run: Main
	./Main

# --- טסטים (מחייבים doctest.h המקומי) ---
tests/test_runner: tests/test_core.cpp $(OBJS) tests/doctest/doctest.h
	$(CXX) $(CXXFLAGS) -o $@ tests/test_core.cpp $(OBJS)

test: tests/test_runner
	./tests/test_runner

# GUI — ננסה לקשר מול SFML; אם אין ספריות/כותרות, נדלג בלי להפיל את כלל הבנייה
gui: Gui

# גילוי נאות - זה GPT כתב.
Gui: src/GuiMain.cpp $(OBJS)
	@echo "Attempting to build GUI..."
	@set -e; \
	$(CXX) $(CXXFLAGS) -o $@ $^ -lsfml-graphics -lsfml-window -lsfml-system 2>/tmp/gui_err.$$ || true; \
	if [ -x ./Gui ]; then \
	  echo "GUI built: ./Gui"; \
	else \
	  echo "GUI skipped (SFML not available). To enable: sudo apt install libsfml-dev"; \
	  rm -f ./Gui; \
	fi; \
	rm -f /tmp/gui_err.$$
# גילוי נאות - עד כאן GPT כתב.

# Valgrind (אופציונלי)
valgrind: Main
	valgrind --leak-check=full --error-exitcode=1 ./Main

# ניקוי
clean:
	rm -f src/Game.o src/Player.o Main Gui tests/test_runner
	rm -rf logs

