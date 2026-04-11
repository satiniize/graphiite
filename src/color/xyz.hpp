#include <glm/glm.hpp>

static constexpr glm::mat3x3 linear_srgb_to_lms = glm::mat3x3(
    0.4122214708, 0.2119034982, 0.0883024619, 0.5363325363, 0.6806995451,
    0.2817188376, 0.0514459929, 0.1073969566, 0.6299787005);

static constexpr glm::mat3x3 linear_srgb_to_xyz =
    glm::mat3x3(0.4124564, 0.2126729, 0.0193339, 0.3575761, 0.7151522,
                0.1191920, 0.1804375, 0.0721750, 0.9503041);

static constexpr glm::mat3x3 xyz_to_lms = glm::mat3x3(
    0.8189330101, 0.0329845436, 0.0482003018, 0.3618667424, 0.9293118715,
    0.2643662691, -0.1288597137, 0.0361456387, 0.6338517070);

static constexpr glm::mat3x3 cube_root_lms_to_oklab = glm::mat3x3(
    0.2104542553, 1.9779984951, 0.0259040371, 0.7936177850, -2.4285922050,
    0.7827717662, -0.0040720468, 0.4505937099, -0.8086757660);

class xyz {
public:
  float x, y, z;

  glm::vec3 to_oklab() {
    glm::vec3 cube_root_lms = xyz_to_lms * glm::vec3(x, y, z);
    cube_root_lms = glm::pow(cube_root_lms, glm::vec3(1.0f / 3.0f));
    return cube_root_lms_to_oklab * cube_root_lms;
  }

  xyz(float x, float y, float z) : x(x), y(y), z(z) {};
}
