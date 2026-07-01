#ifndef COLOR_CONVERTER_HPP
#define COLOR_CONVERTER_HPP

#include <cmath>
#include <cstdint>

/**
 * @struct HSL
 * @brief HSL 颜色空间（色相/饱和度/明度），用于不损失色相的着色
 */
struct HSL {
  float h, s, l; ///< h∈[0,1], s∈[0,1], l∈[0,1]
  HSL(float h = 0, float s = 0, float l = 0) : h(h), s(s), l(l) {}
};

/**
 * @struct RGB
 * @brief 表示RGB颜色，包含红绿蓝三个分量
 */
struct RGB {
  uint8_t r, g, b; ///< 红绿蓝分量，范围0-255

  /**
   * @brief 构造函数，初始化RGB颜色
   * @param r 红色分量，默认为0
   * @param g 绿色分量，默认为0
   * @param b 蓝色分量，默认为0
   */
  RGB(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0) : r(r), g(g), b(b) {}

  /**
   * @brief 整体乘以标量（用于光照计算）
   */
  RGB multiply(float factor) const;

  /**
   * @brief 与另一RGB值相加（饱和到255）
   */
  RGB add(const RGB &other) const;

  /**
   * @brief 将 RGB 转换为 HSL 颜色空间
   */
  HSL toHSL() const;

  /**
   * @brief 从 HSL 颜色空间构造 RGB
   */
  static RGB fromHSL(const HSL &hsl);

  /**
   * @brief 在 HSL 空间中按因子调整明度，保留色相和饱和度
   * @param lightFactor 明度缩放因子（0=全黑, 0.25=暗, 1=原色, >1=过曝白）
   * @return 着色后的 RGB
   */
  RGB shadeInHSL(float lightFactor) const;

  /**
   * @brief 将RGB颜色转换为256色终端颜色索引
   * @return 256色终端颜色索引（16-255）
   */
  int to256Color() const;
};

/**
 * @class ColorConverter
 * @brief 颜色转换工具类，提供静态颜色转换方法
 */
class ColorConverter {
public:
  /**
   * @brief 将RGB颜色转换为256色终端颜色索引
   * @param rgb RGB颜色
   * @return 256色终端颜色索引
   */
  static int rgbTo256Color(const RGB &rgb);
};

#endif
