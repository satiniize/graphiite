#include <image.hpp>

enum class GradientType {
  Linear,
  Radial,
  Angular,
};

class Gradient {
public:
  static Image create(GradientType type);
};
