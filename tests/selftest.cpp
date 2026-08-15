/**
 * rubik_selftest：验证 RubiksCube::getFaceletString 导出的 facelet 与
 * min2phase 解算器的闭环正确性，以及 undo() 的返回值语义。
 *
 * 用法：./rubik_selftest  （无需终端，纯逻辑测试）
 */
#include "HintSystem.hpp"
#include "RubiksCube.hpp"
#include <min2phase/min2phase.h>
#include <min2phase/tools.h>
#include <cctype>
#include <iostream>
#include <string>

static int g_checks = 0;
static int g_failures = 0;

static void check(bool cond, const std::string &msg) {
  ++g_checks;
  if (cond) {
    std::cout << "  ok:   " << msg << "\n";
  } else {
    ++g_failures;
    std::cout << "  FAIL: " << msg << "\n";
  }
}

/** 将解串（形如 "U  R  U' F2"，move2str 格式）应用到魔方；不旋转视角 */
static void applySolution(RubiksCube &cube, const std::string &sol) {
  size_t i = 0;
  while (i < sol.size()) {
    char c = sol[i];
    if (c == ' ' || c == '.') {
      ++i;
      continue;
    }
    std::string face(1, c);
    if (i + 1 >= sol.size())
      break;
    char dir = sol[i + 1];
    bool cw = (dir != '\'');
    cube.rotateViewDirection(face, cw);
    if (dir == '2') // 180° = 两次 90°
      cube.rotateViewDirection(face, true);
    i += 2;
  }
}

/** HintSystem 状态机测试（不依赖终端渲染） */
static void runHintStateTests() {
  RubiksCube cube;

  // 6.1 开启后为 EMPTY
  HintSystem hint;
  check(hint.getState() == HintSystem::State::EMPTY,
        "hint initial state EMPTY");
  hint.toggle();
  check(hint.isEnabled(), "hint enabled after toggle");
  check(hint.getState() == HintSystem::State::EMPTY,
        "hint still EMPTY before first space");
  hint.onCubeStateChanged();
  check(hint.getState() == HintSystem::State::EMPTY,
        "scramble/reset keeps EMPTY before space");

  // 6.2 还原态按空格 -> FINISH
  hint.onSpace(cube);
  check(hint.getState() == HintSystem::State::FINISH,
        "space on solved cube -> FINISH");

  // 6.3 打乱后按空格 -> VALID，指针指向列表头
  cube.rotateViewDirection("R", true);
  cube.rotateViewDirection("U", true);
  cube.rotateViewDirection("F", false);
  hint.onSpace(cube);
  check(hint.getState() == HintSystem::State::VALID,
        "space after scramble -> VALID");
  check(hint.pointer() == 0, "pointer starts at head");
  check(hint.moveCount() > 0, "hint list is non-empty");

  // 6.4 前进 / 上一步的逆操作回退
  const char f0 = hint.moveFace(0);
  const bool cw0 = hint.moveClockwise(0);
  cube.rotateViewDirection(std::string(1, f0), cw0);
  hint.onUserMove(std::string(1, f0), cw0, cube);
  check(hint.pointer() == 1, "correct move advances pointer");
  cube.rotateViewDirection(std::string(1, f0), !cw0);
  hint.onUserMove(std::string(1, f0), !cw0, cube);
  check(hint.pointer() == 0, "inverse of previous step moves pointer back");

  // 6.5 撤销成功回退指针；打乱后撤销失败指针不动
  cube.rotateViewDirection(std::string(1, f0), cw0);
  hint.onUserMove(std::string(1, f0), cw0, cube);
  check(hint.pointer() == 1, "redo move -> pointer 1");
  check(cube.undo(), "undo succeeds after a move");
  hint.onUndo(true, cube);
  check(hint.pointer() == 0, "successful undo moves pointer back");

  RubiksCube cube2;
  cube2.scramble(20);
  HintSystem hint2;
  hint2.toggle();
  hint2.onSpace(cube2);
  check(hint2.getState() == HintSystem::State::VALID,
        "fresh scramble -> VALID");
  check(!cube2.undo(), "undo right after scramble fails");
  hint2.onUndo(false, cube2);
  check(hint2.getState() == HintSystem::State::VALID && hint2.pointer() == 0,
        "failed undo keeps VALID at head");

  // 6.6 错误操作 -> INVALID（且不可复活）
  cube2.rotateViewDirection("D", true);
  hint2.onUserMove("D", true, cube2);
  check(hint2.getState() == HintSystem::State::INVALID,
        "wrong move -> INVALID");
  hint2.onUserMove("D", false, cube2);
  check(hint2.getState() == HintSystem::State::INVALID,
        "moves in INVALID do not revive the list");
  hint2.onSpace(cube2);
  check(hint2.getState() == HintSystem::State::VALID,
        "space re-solves from INVALID");

  // 6.7 指针已在头部、撤销记录仍可撤回 -> INVALID
  check(cube2.undo(), "undo succeeds while pointer is at head");
  hint2.onUndo(true, cube2);
  check(hint2.getState() == HintSystem::State::INVALID,
        "undo past head -> INVALID");

  // 6.8 打乱/重置使有效列表失效
  hint2.onSpace(cube2);
  check(hint2.getState() == HintSystem::State::VALID, "re-solved again");
  hint2.onCubeStateChanged();
  check(hint2.getState() == HintSystem::State::INVALID,
        "scramble/reset invalidates VALID list");

  // 6.9 按提示走完整个列表 -> FINISH 且列表清空、魔方还原
  RubiksCube cube3;
  cube3.rotateViewDirection("B", true);
  cube3.rotateViewDirection("L", false);
  HintSystem hint3;
  hint3.toggle();
  hint3.onSpace(cube3);
  check(hint3.getState() == HintSystem::State::VALID, "hint3 VALID");
  const size_t n3 = hint3.moveCount();
  for (size_t i = 0; i < n3; ++i) {
    const char f = hint3.moveFace(i);
    const bool cw = hint3.moveClockwise(i);
    cube3.rotateViewDirection(std::string(1, f), cw);
    hint3.onUserMove(std::string(1, f), cw, cube3);
    if (i + 1 < n3)
      check(hint3.pointer() == i + 1, "pointer follows the list");
  }
  check(hint3.getState() == HintSystem::State::FINISH,
        "completing the list -> FINISH");
  check(hint3.moveCount() == 0, "list cleared at FINISH");
  check(cube3.getFaceletString() ==
            "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB",
        "cube is solved after following the hints");

  // 6.10 关闭开关
  hint3.toggle();
  check(!hint3.isEnabled(), "toggle off disables hints");
}

int main() {
  RubiksCube cube;

  // 1. 还原态 facelet
  const std::string solvedFacelet =
      "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB";
  std::string f0 = cube.getFaceletString();
  check(f0 == solvedFacelet, "solved cube exports the canonical facelet");

  // 2. 已知序列 R U R' U'
  cube.rotateViewDirection("R", true);
  cube.rotateViewDirection("U", true);
  cube.rotateViewDirection("R", false);
  cube.rotateViewDirection("U", false);
  std::string f1 = cube.getFaceletString();
  check(f1 != solvedFacelet, "scrambled cube facelet differs from solved");

  min2phase::init();

  // 2b. 与官方 tools::fromScramble 交叉验证 facelet 导出（含面内贴纸顺序）
  {
    const char *seqs[] = {"R",         "U",        "F",
                          "B",         "L",        "D",
                          "R U R' U'", "F B' L D", "R2 U' F B R L U2 D"};
    bool allMatch = true;
    for (const char *seq : seqs) {
      RubiksCube c;
      // 用与 tools::fromScramble 相同的记号逐个应用（视图映射恒等）
      std::string m = seq;
      size_t i = 0;
      while (i < m.size()) {
        char ch = m[i];
        if (ch == ' ' || ch == '\'') {
          ++i;
          continue;
        }
        std::string face(1, ch);
        bool cw = true;
        if (i + 1 < m.size() && m[i + 1] == '\'')
          cw = false;
        c.rotateViewDirection(face, cw);
        if (i + 1 < m.size() && m[i + 1] == '2') {
          c.rotateViewDirection(face, true);
          ++i;
        }
        i += 2;
      }
      std::string ref = min2phase::tools::fromScramble(seq);
      if (ref != c.getFaceletString()) {
        allMatch = false;
        std::cout << "    seq [" << seq << "]\n      ref : " << ref << "\n"
                  << "      mine: " << c.getFaceletString() << "\n";
      }
    }
    check(allMatch, "facelet export matches tools::fromScramble for all seqs");
  }

  // 3. 解算闭环：把解应用到魔方应回到还原态
  uint8_t used = 0;
  std::string sol = min2phase::solve(f1, 23, 1000000, 0, 0, &used);
  std::cout << "  solution: [" << sol << "] (" << static_cast<int>(used)
            << " moves)\n";
  check(!sol.empty() && !std::isdigit(static_cast<unsigned char>(sol[0])),
        "solver returns a move string (not an error code)");
  applySolution(cube, sol);
  check(cube.getFaceletString() == solvedFacelet,
        "applying the solution restores the solved state");

  // 4. 随机打乱 + 解算闭环（多轮）
  for (int round = 0; round < 5; ++round) {
    cube.scramble(20);
    std::string scrambled = cube.getFaceletString();
    check(scrambled != solvedFacelet,
          "round " + std::to_string(round) + ": scrambled state differs");
    std::string sol2 = min2phase::solve(scrambled, 23, 1000000, 0, 0, &used);
    applySolution(cube, sol2);
    check(cube.getFaceletString() == solvedFacelet,
          "round " + std::to_string(round) + ": solved back (" +
              std::to_string(static_cast<int>(used)) + " moves)");
  }

  // 5. undo() 返回值语义
  cube.reset();
  check(!cube.undo(), "undo on empty history returns false");
  cube.rotateViewDirection("F", true);
  check(cube.undo(), "undo after a move returns true");
  check(cube.getFaceletString() == solvedFacelet,
        "undo restores the solved state");
  cube.scramble(20);
  check(!cube.undo(), "undo right after scramble returns false");

  // 6. HintSystem 状态机
  runHintStateTests();

  std::cout << "\n"
            << (g_failures == 0 ? "ALL PASSED" : "FAILED") << " (" << g_checks
            << " checks, " << g_failures << " failures)\n";
  return g_failures == 0 ? 0 : 1;
}
