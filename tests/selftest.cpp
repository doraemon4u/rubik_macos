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
  {
    char lf = 0;
    bool lc = false;
    bool hasLast = cube.getLastPerformedMove(lf, lc);
    check(cube.undo(), "undo succeeds after a move");
    hint.onUndo(true, hasLast, lf, lc, cube);
  }
  check(hint.pointer() == 0, "successful undo moves pointer back");

  RubiksCube cube2;
  cube2.scramble(20);
  HintSystem hint2;
  hint2.toggle();
  hint2.onSpace(cube2);
  check(hint2.getState() == HintSystem::State::VALID,
        "fresh scramble -> VALID");
  check(!cube2.undo(), "undo right after scramble fails");
  hint2.onUndo(false, false, 0, false, cube2);
  check(hint2.getState() == HintSystem::State::VALID && hint2.pointer() == 0,
        "failed undo keeps VALID at head");

  // 6.6 容错：错误操作 -> DRIFT（偏离量 1，不再立即失效）
  {
    // 构造一个确定不等于指针期望操作的"错误操作"（本体坐标）
    char wf = 'D';
    bool wc = true;
    if (hint2.moveFace(0) == 'D')
      wf = 'U'; // 面不同，必错
    cube2.rotateViewDirection(std::string(1, wf), wc);
    hint2.onUserMove(std::string(1, wf), wc, cube2);
    check(hint2.getState() == HintSystem::State::DRIFT,
          "wrong move -> DRIFT (not INVALID)");
    check(hint2.deviation() == 1, "deviation is 1 after first wrong move");

    // 6.6b 回正：按回正指引（最近偏离操作的逆操作）-> 偏离归零恢复 VALID
    cube2.rotateViewDirection(std::string(1, wf), !wc);
    hint2.onUserMove(std::string(1, wf), !wc, cube2);
    check(hint2.getState() == HintSystem::State::VALID,
          "inverse of drifted move restores VALID");
    check(hint2.deviation() == 0, "deviation back to 0");

    // 6.6c 退格回正
    cube2.rotateViewDirection(std::string(1, wf), wc); // 又错一步 -> DRIFT
    hint2.onUserMove(std::string(1, wf), wc, cube2);
    check(hint2.getState() == HintSystem::State::DRIFT,
          "wrong move -> DRIFT again");
    {
      char lf = 0;
      bool lc = false;
      bool hasLast = cube2.getLastPerformedMove(lf, lc);
      check(cube2.undo(), "undo in DRIFT succeeds");
      hint2.onUndo(true, hasLast, lf, lc, cube2);
    }
    check(hint2.getState() == HintSystem::State::VALID,
          "undo in DRIFT restores VALID");

    // 6.6d 偏离量累计与阈值：3 次内仍可挽回，第 4 次彻底失效
    for (int k = 1; k <= 3; ++k) {
      cube2.rotateViewDirection(std::string(1, wf), wc);
      hint2.onUserMove(std::string(1, wf), wc, cube2);
      check(hint2.getState() == HintSystem::State::DRIFT,
            "deviation " + std::to_string(k) + " still DRIFT");
      check(hint2.deviation() == k, "deviation == " + std::to_string(k));
    }
    cube2.rotateViewDirection(std::string(1, wf), wc);
    hint2.onUserMove(std::string(1, wf), wc, cube2);
    check(hint2.getState() == HintSystem::State::INVALID,
          "deviation > 3 -> INVALID (permanently)");
    check(hint2.deviation() == 0, "deviation record cleared at INVALID");

    // 6.6e 彻底失效后不再回正（操作不复活列表）
    cube2.rotateViewDirection(std::string(1, wf), !wc);
    hint2.onUserMove(std::string(1, wf), !wc, cube2);
    check(hint2.getState() == HintSystem::State::INVALID,
          "moves in INVALID do not revive the list");
  }
  hint2.onSpace(cube2);
  check(hint2.getState() == HintSystem::State::VALID,
        "space re-solves from INVALID");

  // 6.7 指针已在头部、撤销记录仍可撤回 -> 进入 DRIFT（可挽回）
  {
    char lf = 0;
    bool lc = false;
    bool hasLast = cube2.getLastPerformedMove(lf, lc);
    check(cube2.undo(), "undo succeeds while pointer is at head");
    hint2.onUndo(true, hasLast, lf, lc, cube2);
  }
  check(hint2.getState() == HintSystem::State::DRIFT,
        "undo past head -> DRIFT (recoverable)");
  check(hint2.deviation() == 1, "deviation 1 after head undo");
  check(hint2.pointer() == 0, "pointer stays at head");

  // 6.7b 回归：DRIFT 手动逆操作回正后，undo 栈残留互逆对；
  //        退格撤销的是残留操作，应进 DRIFT 而不是盲目移动指针
  {
    RubiksCube c4;
    c4.scramble(20);
    HintSystem h4;
    h4.toggle();
    h4.onSpace(c4);
    check(h4.getState() == HintSystem::State::VALID, "h4 VALID");
    // 正确执行列表第一步
    const char f0 = h4.moveFace(0);
    const bool cw0 = h4.moveClockwise(0);
    c4.rotateViewDirection(std::string(1, f0), cw0);
    h4.onUserMove(std::string(1, f0), cw0, c4);
    const size_t ptr = h4.pointer();
    check(ptr == 1, "h4 pointer advanced to 1");
    // 错按一步 -> DRIFT
    char wf = 'D';
    if (f0 == 'D')
      wf = 'U';
    c4.rotateViewDirection(std::string(1, wf), true);
    h4.onUserMove(std::string(1, wf), true, c4);
    check(h4.getState() == HintSystem::State::DRIFT, "h4 DRIFT after wrong move");
    // 手动逆操作回正 -> VALID（指针不变，undo 栈残留互逆对）
    c4.rotateViewDirection(std::string(1, wf), false);
    h4.onUserMove(std::string(1, wf), false, c4);
    check(h4.getState() == HintSystem::State::VALID,
          "h4 VALID after manual inverse recovery");
    check(h4.pointer() == ptr, "h4 pointer unchanged after recovery");
    // 退格：撤销的是回正残留，应进 DRIFT 而非 pointer--
    {
      char lf = 0;
      bool lc = false;
      bool hasLast = c4.getLastPerformedMove(lf, lc);
      check(c4.undo(), "h4 undo succeeds");
      h4.onUndo(true, hasLast, lf, lc, c4);
    }
    check(h4.getState() == HintSystem::State::DRIFT,
          "h4 undo of drift residue -> DRIFT (not pointer--)");
    check(h4.pointer() == ptr, "h4 pointer NOT moved by residue undo");
    check(h4.deviation() == 1, "h4 deviation 1");
    // 回正指引 = 撤销残留的逆 = (wf, false)；按它回正
    c4.rotateViewDirection(std::string(1, wf), false);
    h4.onUserMove(std::string(1, wf), false, c4);
    check(h4.getState() == HintSystem::State::VALID,
          "h4 VALID again after following guide");
    check(h4.pointer() == ptr, "h4 pointer still intact after recovery");
  }

  // 6.8 打乱/重置使有效列表失效
  hint2.onSpace(cube2);
  check(hint2.getState() == HintSystem::State::VALID, "re-solved again");
  hint2.onCubeStateChanged();
  check(hint2.getState() == HintSystem::State::INVALID,
        "scramble/reset invalidates VALID list");

  // 6.8b 打乱/重置也使偏离中的列表彻底失效
  cube2.scramble(20);
  hint2.onSpace(cube2);
  check(hint2.getState() == HintSystem::State::VALID, "solved after re-scramble");
  {
    char wf = 'D';
    if (hint2.moveFace(0) == 'D')
      wf = 'U';
    cube2.rotateViewDirection(std::string(1, wf), true);
    hint2.onUserMove(std::string(1, wf), true, cube2);
    check(hint2.getState() == HintSystem::State::DRIFT, "DRIFT again");
  }
  hint2.onCubeStateChanged();
  check(hint2.getState() == HintSystem::State::INVALID,
        "scramble/reset invalidates DRIFT list");

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
