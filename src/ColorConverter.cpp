#include "ColorConverter.hpp"
#include <algorithm>
#include <array>

HSL RGB::toHSL() const {
  float rf = r / 255.0f;
  float gf = g / 255.0f;
  float bf = b / 255.0f;

  float mx = std::max({rf, gf, bf});
  float mn = std::min({rf, gf, bf});
  float l = (mx + mn) / 2.0f;

  float h = 0, s = 0;
  float d = mx - mn;
  if (d > 0.0001f) {
    s = l > 0.5f ? d / (2.0f - mx - mn) : d / (mx + mn);
    if (mx == rf)
      h = (gf - bf) / d + (gf < bf ? 6.0f : 0.0f);
    else if (mx == gf)
      h = (bf - rf) / d + 2.0f;
    else
      h = (rf - gf) / d + 4.0f;
    h /= 6.0f;
  }
  return HSL(h, s, l);
}

RGB RGB::fromHSL(const HSL &hsl) {
  float h = hsl.h, s = hsl.s, l = hsl.l;

  if (s < 0.0001f) {
    uint8_t v = static_cast<uint8_t>(std::clamp(l * 255.0f, 0.0f, 255.0f));
    return RGB(v, v, v);
  }

  auto hue2rgb = [](float p, float q, float t) -> float {
    if (t < 0.0f)
      t += 1.0f;
    if (t > 1.0f)
      t -= 1.0f;
    if (t < 1.0f / 6.0f)
      return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f)
      return q;
    if (t < 2.0f / 3.0f)
      return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    return p;
  };

  float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
  float p = 2.0f * l - q;

  return RGB(
      static_cast<uint8_t>(
          std::clamp(hue2rgb(p, q, h + 1.0f / 3.0f) * 255.0f, 0.0f, 255.0f)),
      static_cast<uint8_t>(std::clamp(hue2rgb(p, q, h) * 255.0f, 0.0f, 255.0f)),
      static_cast<uint8_t>(
          std::clamp(hue2rgb(p, q, h - 1.0f / 3.0f) * 255.0f, 0.0f, 255.0f)));
}

RGB RGB::shadeInHSL(float lightFactor) const {
  HSL hsl = toHSL();

  float newL = std::clamp(hsl.l * (0.4f + 0.6f * lightFactor), 0.12f, 1.0f);

  float satScale = std::clamp(0.35f + 0.65f * lightFactor, 0.35f, 1.0f);
  float newS = std::clamp(hsl.s * satScale, 0.0f, 1.0f);

  hsl.l = newL;
  hsl.s = newS;
  return fromHSL(hsl);
}

RGB RGB::multiply(float factor) const {
  auto clamp = [](float v) -> uint8_t {
    if (v < 0)
      return 0;
    if (v > 255)
      return 255;
    return static_cast<uint8_t>(v);
  };
  return RGB(clamp(r * factor), clamp(g * factor), clamp(b * factor));
}

RGB RGB::add(const RGB &other) const {
  auto clamp = [](int v) -> uint8_t {
    if (v < 0)
      return 0;
    if (v > 255)
      return 255;
    return static_cast<uint8_t>(v);
  };
  return RGB(clamp(r + other.r), clamp(g + other.g), clamp(b + other.b));
}

int RGB::to256Color() const {
  int r_val = std::clamp(static_cast<int>(r), 0, 255);
  int g_val = std::clamp(static_cast<int>(g), 0, 255);
  int b_val = std::clamp(static_cast<int>(b), 0, 255);

  static const std::array<uint8_t, 6> level = {0, 51, 102, 153, 204, 255};

  auto dist = [](int r1, int g1, int b1, int r2, int g2, int b2) -> float {
    float dr = (r1 - r2) * 0.299f;
    float dg = (g1 - g2) * 0.587f;
    float db = (b1 - b2) * 0.114f;

    return dr * dr + dg * dg + db * db;
  };

  int ri = std::clamp(static_cast<int>(std::round(r_val / 51.0f)), 0, 5);
  int gi = std::clamp(static_cast<int>(std::round(g_val / 51.0f)), 0, 5);
  int bi = std::clamp(static_cast<int>(std::round(b_val / 51.0f)), 0, 5);

  int bestIdx = 36 * ri + 6 * gi + bi + 16;
  float bestDist = dist(r_val, g_val, b_val, level[ri], level[gi], level[bi]);

  for (int dr = -1; dr <= 1; dr++) {
    for (int dg = -1; dg <= 1; dg++) {
      for (int db = -1; db <= 1; db++) {
        int nr = ri + dr, ng = gi + dg, nb = bi + db;
        if (nr < 0 || nr > 5 || ng < 0 || ng > 5 || nb < 0 || nb > 5)
          continue;
        float d = dist(r_val, g_val, b_val, level[nr], level[ng], level[nb]);
        if (d < bestDist) {
          bestDist = d;
          bestIdx = 36 * nr + 6 * ng + nb + 16;
        }
      }
    }
  }

  if (std::abs(r_val - g_val) < 16 && std::abs(g_val - b_val) < 16 &&
      std::abs(r_val - b_val) < 16) {
    int avg = (r_val + g_val + b_val) / 3;
    int grayIdx;
    if (avg < 5)
      grayIdx = 232;
    else if (avg > 250)
      grayIdx = 255;
    else
      grayIdx = 232 + static_cast<int>((avg - 5) / 245.0f * 23.0f);

    int gv = grayIdx == 232 ? 0 : (grayIdx - 232) * 255 / 23;
    float gd = dist(r_val, g_val, b_val, gv, gv, gv);
    if (gd < bestDist) {
      bestIdx = grayIdx;
    }
  }

  return bestIdx;
}
