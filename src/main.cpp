#include "HintSystem.hpp"
#include "RubiksCube.hpp"
#include <iostream>
#include <unordered_map>

#ifdef _WIN32
#define PDC_NCMOUSE
#include <pdcurses.h>
#include <windows.h>

struct WinMouseDrag {
  int prev_x = -1, prev_y = -1;
  bool active = false;

  void poll(RubiksCube &cube) {
    bool down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    POINT pt;
    GetCursorPos(&pt);

    if (down && !active) {
      active = true;
      prev_x = pt.x;
      prev_y = pt.y;
    } else if (!down) {
      active = false;
    } else if (active) {
      int dx = pt.x - prev_x;
      int dy = pt.y - prev_y;
      if (dx != 0 || dy != 0) {
        cube.rotateByMouseDelta(static_cast<float>(dx) * 0.4f,
                                static_cast<float>(dy) * 0.4f);
        prev_x = pt.x;
        prev_y = pt.y;
      }
    }
  }
};

#else
#include <curses.h>
#endif

void printInstructions() {
  std::cout << "======================================" << std::endl;
  std::cout << "      3x3 Rubik's Cube Simulator      " << std::endl;
  std::cout << "======================================" << std::endl;
  std::cout << std::endl;
  std::cout << "Controls:" << std::endl;
  std::cout << "  Arrow Keys - Rotate cube" << std::endl;
  std::cout << "  Mouse Drag - Rotate cube" << std::endl;
  std::cout << "  Scroll     - Zoom in/out" << std::endl;
  std::cout << "  +/-        - Zoom in/out" << std::endl;
  std::cout << "  C          - Reset cube" << std::endl;
  std::cout << "  X          - Scramble cube" << std::endl;
  std::cout << "  ESC        - Exit" << std::endl;
  std::cout << std::endl;
  std::cout << "Rotate faces (based on current view):" << std::endl;
  std::cout << "  f/F - Front clockwise/counter" << std::endl;
  std::cout << "  b/B - Back clockwise/counter" << std::endl;
  std::cout << "  l/L - Left clockwise/counter" << std::endl;
  std::cout << "  r/R - Right clockwise/counter" << std::endl;
  std::cout << "  u/U - Up clockwise/counter" << std::endl;
  std::cout << "  d/D - Down clockwise/counter" << std::endl;
  std::cout << std::endl;
  std::cout << "======================================" << std::endl;
  std::cout << "   Please use full-screen terminal    " << std::endl;
  std::cout << "======================================" << std::endl;
  std::cout << std::endl;
  std::cout << "Press Enter to start..." << std::endl;

  std::cin.ignore();
}

int main() {
  printInstructions();

#ifdef _WIN32
  {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
      dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
      SetConsoleMode(hOut, dwMode);
    }
  }
  timeBeginPeriod(1);
#endif

  initscr();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  timeout(16);

  if (!has_colors()) {
    endwin();
    std::cout << "Terminal does not support colors!" << std::endl;
    return 1;
  }

  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
#ifndef _WIN32
  printf("\033[?1003h\n");
#endif
  mouseinterval(0);

  start_color();
#ifndef _WIN32
  use_default_colors();
#endif

  RubiksCube cube;
  HintSystem hint;
  std::unordered_map<int, int> colorCache;
  colorCache.reserve(64);
#ifdef _WIN32
  WinMouseDrag mouseDrag;
#endif

  try {
    while (true) {
      int ch = getch();

#ifdef _WIN32
      mouseDrag.poll(cube);
      if (ch == KEY_MOUSE) {
        MEVENT event;
        if (getmouse(&event) == OK) {
          if (event.bstate & BUTTON4_PRESSED) {
            cube.zoom(3);
          } else if (event.bstate & BUTTON5_PRESSED) {
            cube.zoom(-3);
          }
        }
        int w, h;
        getmaxyx(stdscr, h, w);
        if (w >= 80 && h >= 40) {
          cube.draw(stdscr, w, h, colorCache);
          hint.draw(stdscr, w, h, cube);
          refresh();
        }
        continue;
      }
#else
      if (ch == KEY_MOUSE) {
        static int prev_x = -1, prev_y = -1;
        static bool dragging = false;

        MEVENT event;
        if (getmouse(&event) == OK) {
          if (event.bstate & BUTTON4_PRESSED) {
            cube.zoom(3);
          } else if (event.bstate & BUTTON5_PRESSED) {
            cube.zoom(-3);
          } else if (event.bstate & BUTTON1_PRESSED) {
            dragging = true;
            prev_x = event.x;
            prev_y = event.y;
          } else if (event.bstate & BUTTON1_RELEASED) {
            dragging = false;
          } else if (dragging && (event.bstate & REPORT_MOUSE_POSITION)) {
            int dx = event.x - prev_x;
            int dy = event.y - prev_y;
            if (dx != 0 || dy != 0) {
              cube.rotateByMouseDelta(static_cast<float>(4 * dx),
                                      static_cast<float>(8 * dy));
              prev_x = event.x;
              prev_y = event.y;
            }
          }
        }
        int width, height;
        getmaxyx(stdscr, height, width);
        if (width >= 80 && height >= 40) {
          cube.draw(stdscr, width, height, colorCache);
          hint.draw(stdscr, width, height, cube);
          refresh();
        }
        continue;
      }
#endif

      if (ch == 27 || ch == 'q') {
        break;
      } else if (ch == 'h' || ch == 'H') {
        hint.toggle();
      } else if (ch == ' ') {
        if (hint.isEnabled()) {
          hint.drawSolvingMessage(stdscr);
          refresh();
          hint.onSpace(cube);
        }
      } else if (ch == 'c' || ch == 'C') {
        cube.reset();
        hint.onCubeStateChanged();
      } else if (ch == 'x' || ch == 'X') {
        cube.scramble(20);
        hint.onCubeStateChanged();
      } else if (ch == KEY_UP) {
        cube.rotateByMouseDelta(0, -10);
      } else if (ch == KEY_DOWN) {
        cube.rotateByMouseDelta(0, 10);
      } else if (ch == KEY_LEFT) {
        cube.rotateByMouseDelta(-10, 0);
      } else if (ch == KEY_RIGHT) {
        cube.rotateByMouseDelta(10, 0);
      } else if (ch == '+' || ch == '=') {
        cube.zoom(1);
      } else if (ch == '-' || ch == '_') {
        cube.zoom(-1);
      } else if (ch == 'f') {
        cube.rotateViewDirection("F", true);
        hint.onUserMove("F", true, cube);
      } else if (ch == 'F') {
        cube.rotateViewDirection("F", false);
        hint.onUserMove("F", false, cube);
      } else if (ch == 'b') {
        cube.rotateViewDirection("B", true);
        hint.onUserMove("B", true, cube);
      } else if (ch == 'B') {
        cube.rotateViewDirection("B", false);
        hint.onUserMove("B", false, cube);
      } else if (ch == 'l') {
        cube.rotateViewDirection("L", true);
        hint.onUserMove("L", true, cube);
      } else if (ch == 'L') {
        cube.rotateViewDirection("L", false);
        hint.onUserMove("L", false, cube);
      } else if (ch == 'r') {
        cube.rotateViewDirection("R", true);
        hint.onUserMove("R", true, cube);
      } else if (ch == 'R') {
        cube.rotateViewDirection("R", false);
        hint.onUserMove("R", false, cube);
      } else if (ch == 'u') {
        cube.rotateViewDirection("U", true);
        hint.onUserMove("U", true, cube);
      } else if (ch == 'U') {
        cube.rotateViewDirection("U", false);
        hint.onUserMove("U", false, cube);
      } else if (ch == 'd') {
        cube.rotateViewDirection("D", true);
        hint.onUserMove("D", true, cube);
      } else if (ch == 'D') {
        cube.rotateViewDirection("D", false);
        hint.onUserMove("D", false, cube);
      } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
        char lastFace = 0;
        bool lastCw = false;
        bool hasLast = cube.getLastPerformedMove(lastFace, lastCw);
        bool ok = cube.undo();
        hint.onUndo(ok, hasLast, lastFace, lastCw, cube);
      }

      int width, height;
      getmaxyx(stdscr, height, width);
      if (width >= 80 && height >= 40) {
        cube.draw(stdscr, width, height, colorCache);
        hint.draw(stdscr, width, height, cube);
        refresh();
      } else {
        clear();
        std::string msg = "Please resize terminal to at least 80x40";
        mvprintw(height / 2,
                 std::max(0, (width - static_cast<int>(msg.length())) / 2),
                 "%s", msg.c_str());
        refresh();
      }
    }
  } catch (const std::exception &e) {
    endwin();
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
#ifndef _WIN32
  printf("\033[?1003l\n");
#endif

  endwin();
#ifdef _WIN32
  timeEndPeriod(1);
#endif
  std::cout << "Game ended." << std::endl << "Goodbye!" << std::endl;
  return 0;
}
