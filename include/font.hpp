#pragma once

#include <filesystem>

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
  // In px, ie. font units * font_scale
  int ascent;
  int descent;
  int line_height;
  int line_gap;

  int glyph_padding = 16;

  // This is actually in pixels, the font now occupies px in height
  float sample_point_size = static_cast<float>(FontSize::MEDIUM);
  float font_scale;

  std::filesystem::path font_path;

  Texture font_atlas;
  std::unordered_map<int, GlyphMetrics> glyph_metrics;

  Font(const std::filesystem::path &font_path);
  // ~Font();

  int get_ascent() const { return ascent; }
  int get_descent() const { return descent; }
  int get_line_gap() const { return line_gap; }
  int get_line_height() const { return line_height; }

  Image generate_atlas();
};
