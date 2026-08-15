/**
 * min2phase http 模块的空实现（stub）。
 *
 * min2phaseCXX 仓库自带的 src/http.cpp 实现了可选的远程解算服务器
 * （webSearch/server/stop），依赖 POSIX 接口（unistd.h、sys/socket.h、
 * fork、SIGCHLD 等），对 Windows 不友好，且本终端应用并不需要该功能。
 *
 * 本文件以空实现提供 min2phase::http 的三个符号，保证 min2phase.cpp
 * 能正常链接；跨平台（Linux / Windows）行为一致：始终返回失败/空串。
 */
#include <min2phase/min2phase.h>

namespace min2phase {
namespace http {

std::string webSolver(const std::string &, int32_t, const std::string &,
                      int8_t, int32_t, int32_t, int8_t, uint8_t *,
                      std::string *) {
  return std::string();
}

bool init(uint16_t, uint16_t) {
  return false;
}

bool stop() {
  return false;
}

}  // namespace http
}  // namespace min2phase
