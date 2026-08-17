#include "RubiksCube.hpp"
#include <min2phase/min2phase.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#define PDC_NCMOUSE
#include <pdcurses.h>
#else
#include <curses.h>
#endif

struct Move {
  char face; ///< 面：U R F D L B（本体坐标）
  bool cw;   ///< 是否顺时针
};

/** 解析 min2phase 输出（"U  R  U' F2" 格式，180° 展开为两步） */
static bool parseSolution(const std::string &sol, std::vector<Move> &out) {
  out.clear();
  size_t i = 0;
  while (i < sol.size()) {
    char c = sol[i];
    if (c == ' ' || c == '.') {
      ++i;
      continue;
    }
    if (std::strchr("URFDLB", c) == nullptr)
      return false;
    if (i + 1 >= sol.size())
      return false;
    switch (sol[i + 1]) {
    case '2':
      out.push_back({c, true});
      out.push_back({c, true});
      break;
    case '\'':
      out.push_back({c, false});
      break;
    default:
      out.push_back({c, true});
      break;
    }
    i += 2;
  }
  return true;
}

static void printIntro(int scrambleMoves, int delayMs) {
  std::cout << "======================================" << std::endl;
  std::cout << "   Rubik's Cube Auto-Solver           " << std::endl;
  std::cout << "======================================" << std::endl;
  std::cout << "  Scrambles the cube (" << scrambleMoves << " moves), then"
            << std::endl;
  std::cout << "  solves it back automatically." << std::endl;
  std::cout << "  Each move animates with " << delayMs << "ms delay." << std::endl;
  std::cout << "  Press q/ESC anytime to quit." << std::endl;
  std::cout << std::endl;
  std::cout << "  Usage: rubik_autosolve [scramble_moves=20] [delay_ms=1000]"
            << std::endl;
  std::cout << "======================================" << std::endl;
  std::cout << std::endl;
  std::cout << "Press Enter to start..." << std::endl;
  std::cin.ignore();
}

int main(int argc, char *argv[]) {
  int scrambleMoves = 20;
  int delayMs = 1000;
  if (argc > 1)
    scrambleMoves = std::atoi(argv[1]);
  if (argc > 2)
    delayMs = std::atoi(argv[2]);
  if (scrambleMoves < 0)
    scrambleMoves = 0;
  if (delayMs < 16)
    delayMs = 16;

  printIntro(scrambleMoves, delayMs);

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

  start_color();
#ifndef _WIN32
  use_default_colors();
#endif

  RubiksCube cube;
  std::unordered_map<int, int> colorCache;
  colorCache.reserve(64);

  if (scrambleMoves > 0)
    cube.scramble(scrambleMoves);

  int w, h;
  getmaxyx(stdscr, h, w);
  if (w >= 80 && h >= 40) {
    cube.draw(stdscr, w, h, colorCache);
    mvprintw(1, 1, "Scrambled with %d moves - solving...", scrambleMoves);
  } else {
    mvprintw(h / 2, 0, "Please resize terminal to at least 80x40");
  }
  refresh();

  min2phase::init();
  uint8_t usedMoves = 0;
  std::string sol =
      min2phase::solve(cube.getFaceletString(), 23, 1000000, 0, 0, &usedMoves);

  std::vector<Move> moves;
  if (!parseSolution(sol, moves)) {
    endwin();
    std::cerr << "Solver error: [" << sol << "]" << std::endl;
    return 1;
  }
  if (moves.empty()) {
    mvprintw(1, 1, "Already solved!");
    refresh();
    endwin();
    std::cout << "Cube was already solved." << std::endl;
    return 0;
  }

  const size_t total = moves.size();
  for (size_t i = 0; i < total; ++i) {
    cube.rotateViewDirection(std::string(1, moves[i].face), moves[i].cw);

    auto start = std::chrono::steady_clock::now();
    while (true) {
      int ch = getch();
      if (ch == 27 || ch == 'q') {
        endwin();
        std::cout << "Quit after " << i << "/" << total << " moves." << std::endl;
        return 0;
      }

      int ww, hh;
      getmaxyx(stdscr, hh, ww);
      if (ww >= 80 && hh >= 40) {
        cube.draw(stdscr, ww, hh, colorCache);
        mvprintw(1, 1, "Step %zu/%zu: %c%s   ", i + 1, total, moves[i].face,
                 moves[i].cw ? "" : "'");
      }
      refresh();

      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now() - start)
                         .count();
      if (elapsed >= delayMs)
        break;
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
  }

  getmaxyx(stdscr, h, w);
  if (w >= 80 && h >= 40) {
    cube.draw(stdscr, w, h, colorCache);
    mvprintw(1, 1, "SOLVED! (%zu moves) - press any key to exit", total);
  }
  refresh();

  while (getch() == ERR) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  endwin();
  std::cout << "Solved in " << total << " moves." << std::endl;
  return 0;
}