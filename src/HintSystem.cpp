#include "HintSystem.hpp"
#include "ColorConverter.hpp"
#include "RubiksCube.hpp"
#include <min2phase/min2phase.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <mutex>

void HintSystem::ensureColors() {
  static bool done = false;
  if (done)
    return;
  done = true;

  auto init256 = [](int pair, const RGB &rgb) {
    init_pair(pair, rgb.to256Color(), COLOR_BLACK);
  };

  init_pair(PAIR_CYAN, COLOR_CYAN, COLOR_BLACK); // 中括号
  init256(PAIR_GREY, RGB(140, 140, 140));        // 灰色
  init256(PAIR_BLOOD, RGB(204, 20, 20));         // 血红色（星号）
  init256(PAIR_RED, RGB(176, 96, 96));           // 低饱和红（Back/Invalid）
  init256(PAIR_ORANGE, RGB(255, 135, 0));        // Front
  init256(PAIR_BLUE, RGB(60, 120, 255));         // Left
  init256(PAIR_GREEN, RGB(0, 200, 0));           // Right / Finish
  init256(PAIR_WHITE, RGB(235, 235, 235));       // Up
  init256(PAIR_YELLOW, RGB(235, 235, 0));        // Down
}

int HintSystem::faceColorPair(char face) {
  switch (face) {
  case 'F':
    return PAIR_ORANGE;
  case 'B':
    return PAIR_RED;
  case 'L':
    return PAIR_BLUE;
  case 'R':
    return PAIR_GREEN;
  case 'U':
    return PAIR_WHITE;
  case 'D':
    return PAIR_YELLOW;
  default:
    return PAIR_WHITE;
  }
}

std::string HintSystem::solveCube(const std::string &facelets) {
  static std::once_flag initOnce;
  std::call_once(initOnce, [] { min2phase::init(); });

  uint8_t usedMoves = 0;
  return min2phase::solve(facelets, 23, 1000000, 0, 0, &usedMoves);
}

bool HintSystem::parseSolution(const std::string &sol, std::vector<Move> &out) {
  out.clear();
  if (sol.empty())
    return true;

  bool allDigits = true;
  for (char c : sol) {
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      allDigits = false;
      break;
    }
  }
  if (allDigits)
    return false;

  size_t i = 0;
  while (i < sol.size()) {
    char c = sol[i];
    if (c == ' ' || c == '.') {
      ++i;
      continue;
    }
    char face = c;
    if (face != 'U' && face != 'R' && face != 'F' && face != 'D' &&
        face != 'L' && face != 'B')
      return false;
    if (i + 1 >= sol.size())
      return false;
    int pow;
    switch (sol[i + 1]) {
    case ' ':
      pow = 1;
      break;
    case '2':
      pow = 2;
      break;
    case '\'':
      pow = 3;
      break;
    default:
      return false;
    }
    if (pow == 2) {
      out.push_back({face, true});
      out.push_back({face, true});
    } else {
      out.push_back({face, pow == 1});
    }
    i += 2;
  }
  return true;
}

void HintSystem::toggle() { enabled_ = !enabled_; }

void HintSystem::onSpace(RubiksCube &cube) {
  if (!enabled_)
    return;

  std::string facelets = cube.getFaceletString();
  std::string sol = solveCube(facelets);

  moves_.clear();
  pointer_ = 0;
  message_.clear();

  if (!parseSolution(sol, moves_)) {
    state_ = State::INVALID;
    message_ = "Solver error - press SPACE to retry";
    return;
  }
  if (moves_.empty()) {
    state_ = State::FINISH;
    return;
  }
  state_ = State::VALID;
}

void HintSystem::onUserMove(const std::string &viewDir, bool clockwise,
                            RubiksCube &cube) {
  if (!enabled_)
    return;

  if (state_ == State::FINISH) {
    state_ = State::INVALID;
    message_ = "Invalid - press SPACE for a new solution";
    return;
  }
  if (state_ != State::VALID)
    return;

  const auto &vm = cube.getViewMapping();
  auto it = vm.find(viewDir);
  if (it == vm.end())
    return;

  Move m{it->second[0], clockwise};

  if (pointer_ < moves_.size() && m == moves_[pointer_]) {
    ++pointer_;
    if (pointer_ >= moves_.size()) {
      moves_.clear();
      pointer_ = 0;
      state_ = State::FINISH;
    }
    return;
  }

  if (pointer_ > 0 && m == inverse(moves_[pointer_ - 1])) {
    --pointer_;
    return;
  }

  state_ = State::INVALID;
  message_ = "Invalid - press SPACE for a new solution";
}

void HintSystem::onUndo(bool success, RubiksCube &) {
  if (!enabled_)
    return;

  if (state_ == State::VALID) {
    if (!success)
      return;
    if (pointer_ > 0) {
      --pointer_;
      return;
    }
    state_ = State::INVALID;
    message_ = "Invalid - press SPACE for a new solution";
  } else if (state_ == State::FINISH) {
    if (success) {
      state_ = State::INVALID;
      message_ = "Invalid - press SPACE for a new solution";
    }
  }
}

void HintSystem::onCubeStateChanged() {
  if (!enabled_)
    return;
  if (state_ == State::VALID || state_ == State::FINISH) {
    state_ = State::INVALID;
    message_ = "Invalid - press SPACE for a new solution";
  }
}

void HintSystem::drawSolvingMessage(WINDOW *win) {
  if (!enabled_)
    return;
  ensureColors();
  mvwprintw(win, 1, 1, "Solving...");
}

void HintSystem::draw(WINDOW *win, int width, int height,
                      const RubiksCube &cube) {
  if (!enabled_)
    return;
  ensureColors();
  (void)width;

  const int x0 = 1, y0 = 1;

  for (int r = y0; r < y0 + 19 && r < height; r++) {
    for (int c = x0; c < x0 + 16; c++) {
      mvwaddch(win, r, c, ' ');
    }
  }

  if (state_ == State::EMPTY) {
    wattron(win, COLOR_PAIR(PAIR_GREY));
    mvwprintw(win, y0, x0, "Empty");
    wattroff(win, COLOR_PAIR(PAIR_GREY));
    return;
  }
  if (state_ == State::FINISH) {
    wattron(win, COLOR_PAIR(PAIR_GREEN));
    mvwprintw(win, y0, x0, "Finish");
    wattroff(win, COLOR_PAIR(PAIR_GREEN));
    return;
  }
  if (state_ == State::INVALID) {
    wattron(win, COLOR_PAIR(PAIR_RED));
    mvwprintw(win, y0, x0, "Invalid");
    wattroff(win, COLOR_PAIR(PAIR_RED));
    mvwprintw(win, y0 + 1, x0, "%s", message_.c_str());
    return;
  }

  const size_t n = moves_.size();
  if (n == 0)
    return;
  const int numW = static_cast<int>(std::to_string(n - 1).size());
  constexpr int MAX_NAME_LEN = 5;
  const int maxOpLen = 1 + numW + 3 + MAX_NAME_LEN;

  std::map<char, char> faceToView;
  for (const auto &kv : cube.getViewMapping()) {
    faceToView[kv.second[0]] = kv.first[0];
  }
  auto viewName = [](char v) -> const char * {
    switch (v) {
    case 'F':
      return "Front";
    case 'B':
      return "Back";
    case 'L':
      return "Left";
    case 'R':
      return "Right";
    case 'U':
      return "Up";
    case 'D':
      return "Down";
    default:
      return "?";
    }
  };

  int maxRows = height - y0 - 2;
  if (maxRows > 18)
    maxRows = 18;
  if (maxRows < 1)
    maxRows = 1;
  int start = 0;
  int end = static_cast<int>(n);
  if (end > maxRows) {
    start = static_cast<int>(pointer_) - maxRows / 2;
    if (start < 0)
      start = 0;
    if (start + maxRows > end)
      start = end - maxRows;
    end = start + maxRows;
  }

  int yrow = y0;
  for (int i = start; i < end; ++i, ++yrow) {
    const Move &mv = moves_[static_cast<size_t>(i)];
    const bool done = i < static_cast<int>(pointer_);
    const bool current = i == static_cast<int>(pointer_);
    const char *name = viewName(faceToView[mv.face]);
    std::string numStr = std::to_string(i);
    int col = x0;

    wattron(win, COLOR_PAIR(PAIR_CYAN));
    mvwaddch(win, yrow, col, current ? '[' : ' ');
    wattroff(win, COLOR_PAIR(PAIR_CYAN));
    ++col;

    mvwaddch(win, yrow, col++, ' ');

    const std::string numPadded =
        std::string(static_cast<size_t>(numW) - numStr.size(), ' ') + numStr;
    if (done)
      wattron(win, COLOR_PAIR(PAIR_GREY));
    for (int k = 0; k < numW; ++k) {
      mvwaddch(win, yrow, col + k, numPadded[k]);
    }
    if (done)
      wattroff(win, COLOR_PAIR(PAIR_GREY));
    col += numW;

    mvwaddch(win, yrow, col++, ' ');

    if (!mv.cw) {
      if (done)
        wattron(win, COLOR_PAIR(PAIR_GREY));
      else
        wattron(win, COLOR_PAIR(PAIR_BLOOD));
      mvwaddch(win, yrow, col, '*');
      if (done)
        wattroff(win, COLOR_PAIR(PAIR_GREY));
      else
        wattroff(win, COLOR_PAIR(PAIR_BLOOD));
    } else {
      mvwaddch(win, yrow, col, ' ');
    }
    ++col;

    mvwaddch(win, yrow, col++, ' ');

    const int pair = faceColorPair(mv.face);
    if (done)
      wattron(win, COLOR_PAIR(PAIR_GREY));
    else
      wattron(win, COLOR_PAIR(pair));
    mvwprintw(win, yrow, col, "%s", name);
    if (done)
      wattroff(win, COLOR_PAIR(PAIR_GREY));
    else
      wattroff(win, COLOR_PAIR(pair));
    col += static_cast<int>(std::strlen(name));

    if (current) {
      wattron(win, COLOR_PAIR(PAIR_CYAN));
      const int closeCol = x0 + 1 + maxOpLen;
      while (col < closeCol)
        mvwaddch(win, yrow, col++, ' ');
      mvwaddch(win, yrow, col, ']');
      wattroff(win, COLOR_PAIR(PAIR_CYAN));
    }
  }
}
