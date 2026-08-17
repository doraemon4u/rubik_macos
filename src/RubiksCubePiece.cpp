#include "RubiksCubePiece.hpp"
#include <map>
#include <sstream>

// 面字母 -> initialColors 下标（F,B,L,R,U,D）
static int faceColorIndex(char face) {
  switch (face) {
  case 'F':
    return 0;
  case 'B':
    return 1;
  case 'L':
    return 2;
  case 'R':
    return 3;
  case 'U':
    return 4;
  case 'D':
    return 5;
  default:
    return -1;
  }
}

RubiksCubePiece::RubiksCubePiece(const Vector3 &position, PieceType type)
    : initialPosition(position), currentPosition(position), pieceType(type),
      localRotation(1, 0, 0, 0) {
  initialColors.fill(_COLOR_NONE);
  initColors();
}

void RubiksCubePiece::initColors() {
  float x = initialPosition.x;
  float y = initialPosition.y;
  float z = initialPosition.z;

  if (pieceType == PIECE_CENTER) {
    if (x == -1.0f)
      initialColors[2] = static_cast<Color>(_COLOR_BLUE); // L
    else if (x == 1.0f)
      initialColors[3] = static_cast<Color>(_COLOR_GREEN); // R
    else if (y == -1.0f)
      initialColors[5] = static_cast<Color>(_COLOR_YELLOW); // D
    else if (y == 1.0f)
      initialColors[4] = static_cast<Color>(_COLOR_WHITE); // U
    else if (z == -1.0f)
      initialColors[1] = static_cast<Color>(_COLOR_ORANGE); // B
    else if (z == 1.0f)
      initialColors[0] = static_cast<Color>(_COLOR_RED); // F
    return;
  }

  if (x == -1.0f)
    initialColors[2] = static_cast<Color>(_COLOR_BLUE); // L
  else if (x == 1.0f)
    initialColors[3] = static_cast<Color>(_COLOR_GREEN); // R

  if (y == -1.0f)
    initialColors[5] = static_cast<Color>(_COLOR_YELLOW); // D
  else if (y == 1.0f)
    initialColors[4] = static_cast<Color>(_COLOR_WHITE); // U

  if (z == -1.0f)
    initialColors[1] = static_cast<Color>(_COLOR_ORANGE); // B
  else if (z == 1.0f)
    initialColors[0] = static_cast<Color>(_COLOR_RED); // F
}

void RubiksCubePiece::rotate(const Vector3 &axis, float angle) {
  Quaternion rotation = Quaternion::fromAxisAngle(axis, angle);
  currentPosition = rotation.rotateVector(currentPosition);
  localRotation = rotation.multiply(localRotation).normalize();
}

std::vector<Vector3>
RubiksCubePiece::getFaceCorners(const std::string &faceName) const {
  static const std::map<std::string,
                        std::vector<std::tuple<float, float, float>>>
      FACE_CORNERS = {{"F",
                       {{-0.5f, -0.5f, 0.5f},
                        {0.5f, -0.5f, 0.5f},
                        {0.5f, 0.5f, 0.5f},
                        {-0.5f, 0.5f, 0.5f}}},
                      {"B",
                       {{-0.5f, -0.5f, -0.5f},
                        {-0.5f, 0.5f, -0.5f},
                        {0.5f, 0.5f, -0.5f},
                        {0.5f, -0.5f, -0.5f}}},
                      {"L",
                       {{-0.5f, -0.5f, -0.5f},
                        {-0.5f, -0.5f, 0.5f},
                        {-0.5f, 0.5f, 0.5f},
                        {-0.5f, 0.5f, -0.5f}}},
                      {"R",
                       {{0.5f, -0.5f, 0.5f},
                        {0.5f, -0.5f, -0.5f},
                        {0.5f, 0.5f, -0.5f},
                        {0.5f, 0.5f, 0.5f}}},
                      {"U",
                       {{-0.5f, 0.5f, 0.5f},
                        {0.5f, 0.5f, 0.5f},
                        {0.5f, 0.5f, -0.5f},
                        {-0.5f, 0.5f, -0.5f}}},
                      {"D",
                       {{-0.5f, -0.5f, -0.5f},
                        {0.5f, -0.5f, -0.5f},
                        {0.5f, -0.5f, 0.5f},
                        {-0.5f, -0.5f, 0.5f}}}};

  auto it = FACE_CORNERS.find(faceName);
  if (it == FACE_CORNERS.end()) {
    return {};
  }

  std::vector<Vector3> corners;
  for (const auto &corner : it->second) {
    Vector3 cornerVec(std::get<0>(corner), std::get<1>(corner),
                      std::get<2>(corner));
    Vector3 rotatedCorner = localRotation.rotateVector(cornerVec);
    corners.push_back(rotatedCorner);
  }

  return corners;
}

Color RubiksCubePiece::getCurrentFaceColor(const std::string &faceName) const {
  int idx = faceColorIndex(faceName.empty() ? '\0' : faceName[0]);
  return idx >= 0 ? initialColors[idx] : _COLOR_NONE;
}

void RubiksCubePiece::reset() {
  currentPosition = initialPosition;
  localRotation = Quaternion(1, 0, 0, 0);
}

std::string RubiksCubePiece::toString() const {
  std::ostringstream oss;
  oss << (pieceType == PIECE_CORNER ? "Corner"
          : pieceType == PIECE_EDGE ? "Edge"
                                    : "Center");
  oss << currentPosition.toString();
  return oss.str();
}
