#ifndef HINTSYSTEM_HPP
#define HINTSYSTEM_HPP

#include "Enums.hpp"
#include <cstddef>
#include <string>
#include <vector>

#ifdef _WIN32
#define PDC_NCMOUSE
#include <pdcurses.h>
#else
#include <curses.h>
#endif

class RubiksCube;

/**
 * @class HintSystem
 * @brief 提示系统：H 键开关，空格键根据当前魔方状态调用 min2phase 求解，
 *        在左上角竖排显示操作列表，并用指针（青色中括号）指引下一步操作。
 *
 * 状态机：
 *   EMPTY  —— 面板开启但尚未按过空格（显示灰色 Empty）
 *   VALID  —— 列表有效，维护指针（前进/回退/失效判定）
 *   INVALID—— 用户偏离了预定操作，显示英文提示；按空格可重新求解
 *   FINISH —— 指针到达列表尾部（魔方已还原），列表清空显示 Finish
 *
 * 指针规则（仅 VALID 维护）：
 *   - 操作 == 指针所指（本体坐标比较）-> 指针后移；到尾部 -> FINISH
 *   - 退格/上一步的逆操作成功 -> 指针前移；指针已在头部仍回退 -> INVALID
 *   - 其他操作 -> INVALID
 */
class HintSystem {
public:
  enum class State { EMPTY, VALID, INVALID, FINISH };

  HintSystem() = default;

  bool isEnabled() const { return enabled_; }
  State getState() const { return state_; }

  // 供测试/调试使用的最小只读访问器
  size_t moveCount() const { return moves_.size(); }
  size_t pointer() const { return pointer_; }
  char moveFace(size_t i) const { return moves_[i].face; }
  bool moveClockwise(size_t i) const { return moves_[i].cw; }

  /** H 键：开关面板（状态保留） */
  void toggle();

  /** 空格键：根据当前状态求解并更新提示列表 */
  void onSpace(RubiksCube &cube);

  /** 用户执行了一次面旋转（视图坐标），由 main 在 cube 旋转后调用 */
  void onUserMove(const std::string &viewDir, bool clockwise,
                  RubiksCube &cube);

  /** 用户撤销（cube.undo() 已执行），success 表示撤销是否成功 */
  void onUndo(bool success, RubiksCube &cube);

  /** 打乱 / 重置后调用：当前列表不再匹配，标记失效 */
  void onCubeStateChanged();

  /** 解算前绘制 "Solving..." 反馈（解算可能耗时） */
  void drawSolvingMessage(WINDOW *win);

  /** 每帧绘制面板（仅在开启时绘制） */
  void draw(WINDOW *win, int width, int height, const RubiksCube &cube);

private:
  /** 一次旋转操作（本体坐标） */
  struct Move {
    char face; ///< 本体面：F B L R U D
    bool cw;   ///< 是否顺时针
    bool operator==(const Move &o) const {
      return face == o.face && cw == o.cw;
    }
  };

  static Move inverse(const Move &m) { return Move{m.face, !m.cw}; }

  /** 颜色对编号（固定高位，避开 cube 动态缓存 1..N） */
  enum ColorPair {
    PAIR_CYAN = 51,   ///< 指针中括号
    PAIR_GREY = 52,   ///< 灰色（Empty / 已完成行）
    PAIR_BLOOD = 53,  ///< 血红色（星号）
    PAIR_RED = 54,    ///< 低饱和红（Back 面名 / Invalid）
    PAIR_ORANGE = 55, ///< Front 面名
    PAIR_BLUE = 56,   ///< Left 面名
    PAIR_GREEN = 57,  ///< Right 面名 / Finish
    PAIR_WHITE = 58,  ///< Up 面名
    PAIR_YELLOW = 59, ///< Down 面名
  };

  /** 惰性初始化颜色对（start_color 之后调用一次） */
  static void ensureColors();

  /** 面名对应的颜色对 */
  static int faceColorPair(char face);

  /** 解析 min2phase 解算输出（"X1 X3 ..."，180° 展开为两步） */
  static bool parseSolution(const std::string &sol, std::vector<Move> &out);

  /** 调用解算器（惰性初始化表），返回原始解串 */
  static std::string solveCube(const std::string &facelets);

  bool enabled_ = false;
  State state_ = State::EMPTY;
  std::vector<Move> moves_; ///< 本体坐标下的操作序列
  size_t pointer_ = 0;      ///< 下一个应执行操作的序号（0=列表头）
  std::string message_;     ///< INVALID 状态的英文提示
};

#endif // HINTSYSTEM_HPP
