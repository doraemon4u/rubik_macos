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
  init256(PAIR_GREEN, RGB(0, 200, 0));           // Right / Finish / Deviation
  init256(PAIR_WHITE, RGB(235, 235, 235));       // Up
  init256(PAIR_YELLOW, RGB(235, 235, 0));        // Down
}

int HintSystem::faceColorPair(char face) {
  switch (face) {
  case 'F':
    return PAIR_ORANGE; // F = 橙
  case 'B':
    return PAIR_RED; // B = 红
  case 'L':
    return PAIR_BLUE; // L = 蓝
  case 'R':
    return PAIR_GREEN; // R = 绿
  case 'U':
    return PAIR_WHITE; // U = 白
  case 'D':
    return PAIR_YELLOW; // D = 黄
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

void HintSystem::invalidate() {
  state_ = State::INVALID;
  deviation_ = 0;
  driftStack_.clear();
  message_ = "Invalid - press SPACE for a new solution";
}

void HintSystem::toggle() { enabled_ = !enabled_; }

void HintSystem::onSpace(RubiksCube &cube) {
  if (!enabled_)
    return;

  std::string facelets = cube.getFaceletString();
  std::string sol = solveCube(facelets);

  moves_.clear();
  pointer_ = 0;
  deviation_ = 0;
  driftStack_.clear();
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
    invalidate();
    return;
  }
  if (state_ != State::VALID && state_ != State::DRIFT)
    return;

  const auto &vm = cube.getViewMapping();
  auto it = vm.find(viewDir);
  if (it == vm.end())
    return;

  Move m{it->second[0], clockwise};

  if (state_ == State::VALID) {
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
    state_ = State::DRIFT;
    deviation_ = 1;
    driftStack_.clear();
    driftStack_.push_back(m);
    return;
  }

  if (m == inverse(driftStack_.back())) {
    driftStack_.pop_back();
    --deviation_;
    if (deviation_ == 0) {
      state_ = State::VALID;
    }
    return;
  }
  driftStack_.push_back(m);
  ++deviation_;
  if (deviation_ > MAX_DEVIATION) {
    invalidate();
  }
}

void HintSystem::onUndo(bool success, bool hasLast, char lastFace, bool lastCw,
                        RubiksCube &) {
  if (!enabled_)
    return;

  if (state_ == State::VALID) {
    if (!success)
      return;
    if (!hasLast) {
      invalidate();
      return;
    }
    Move x{lastFace, lastCw};
    if (pointer_ > 0 && x == moves_[pointer_ - 1]) {
      --pointer_;
      return;
    }
    state_ = State::DRIFT;
    deviation_ = 1;
    driftStack_.clear();
    driftStack_.push_back(inverse(x));
    return;
  } else if (state_ == State::DRIFT) {
    if (!success || !hasLast)
      return;
    Move x{lastFace, lastCw};
    if (x == driftStack_.back()) {
      driftStack_.pop_back();
      --deviation_;
      if (deviation_ == 0) {
        state_ = State::VALID;
      }
    } else {
      driftStack_.push_back(inverse(x));
      ++deviation_;
      if (deviation_ > MAX_DEVIATION) {
        invalidate();
      }
    }
  } else if (state_ == State::FINISH) {
    if (success) {
      invalidate();
    }
  }
}

void HintSystem::onCubeStateChanged() {
  if (!enabled_)
    return;
  if (state_ == State::VALID || state_ == State::FINISH ||
      state_ == State::DRIFT) {
    invalidate();
  }
}

void HintSystem::drawSolvingMessage(WINDOW *win) {
  if (!enabled_)
    return;
  ensureColors();
  mvwprintw(win, 1, 1, "Solving...");
}

void HintSystem::drawMoveRow(WINDOW *win, int yrow, int x0, char viewDir,
                             bool cw, const std::string &numStr, int numW,
                             int maxOpLen, bool done, bool current) const {
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

  if (!cw) {
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

  const char *name = viewName(viewDir);
  const int pair = faceColorPair(viewDir);
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
  if (state_ == State::DRIFT) {
    wattron(win, COLOR_PAIR(PAIR_RED));
    mvwprintw(win, y0, x0, "Invalid");
    wattroff(win, COLOR_PAIR(PAIR_RED));

    wattron(win, COLOR_PAIR(PAIR_GREEN));
    mvwprintw(win, y0 + 1, x0, "Deviation: %d", deviation_);
    wattroff(win, COLOR_PAIR(PAIR_GREEN));

    if (!driftStack_.empty()) {
      const Move guide = inverse(driftStack_.back());
      std::map<char, char> faceToView;
      for (const auto &kv : cube.getViewMapping()) {
        faceToView[kv.second[0]] = kv.first[0];
      }
      auto it = faceToView.find(guide.face);
      const char viewDir = it != faceToView.end() ? it->second : guide.face;
      drawMoveRow(win, y0 + 2, x0, viewDir, guide.cw, "0", 1, 12, false, true);
    }
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
    auto it = faceToView.find(mv.face);
    const char viewDir = it != faceToView.end() ? it->second : mv.face;
    drawMoveRow(win, yrow, x0, viewDir, mv.cw, std::to_string(i), numW,
                maxOpLen, done, current);
  }
}
