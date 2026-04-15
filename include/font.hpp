#pragma once

#include <glm/glm.hpp>

#include "texture.hpp"

struct GlyphMetrics {
  glm::vec4 uv_rect; // normalized 0..1 atlas UVs (x0, y0, x1, y1)
  glm::vec2 size;    // bitmap size in pixels
  glm::vec2 bearing; // offset from cursor origin to top-left of bitmap
  float advance;     // cursor advance in pixels
};

class Font {
public:
  int ascent;
  int descent;
  int line_height;
  int line_gap;

  int glyph_padding;

  float sample_point_size;

  Texture font_atlas;
  std::unordered_map<int, GlyphMetrics> _glyph_metrics;

  int get_line_height() const { return ascent + descent + line_gap; }
};
