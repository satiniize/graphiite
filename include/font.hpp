#pragma once

#include <glm/glm.hpp>

#include "image.hpp"
#include "texture.hpp"
#include "theme.hpp"

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

  int glyph_padding = 16;

  float sample_point_size = static_cast<float>(FontSize::MEDIUM);
  float font_scale;

  Texture font_atlas;
  std::unordered_map<int, GlyphMetrics> glyph_metrics;

  // Font() = default;
  // ~Font();

  int get_ascent() const { return ascent; }
  int get_descent() const { return descent; }
  int get_line_gap() const { return line_gap; }
  int get_line_height() const { return ascent + descent + line_gap; }

  Image generate_atlas(const std::string &font_path);
};
