#include <image.hpp>

enum class GradientType {
  LINEAR,
  RADIAL,
  ANGULAR,
};

struct GradientStop {
  float position;
  // Color color;
};

class Gradient {
public:
  static Image createLinear();
  static Image createRadial();
  static Image createAngular();
};
