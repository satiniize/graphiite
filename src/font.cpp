#include "font.hpp"

#include <fstream>

#include <SDL3/SDL_log.h>
#include <stb_rect_pack.h>
#include <stb_truetype.h>

static std::vector<uint8_t> load_font(const std::string &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    SDL_Log("Failed to load font from disk! %s", path.c_str());
    return {};
  }
  size_t file_size = file.tellg();
  std::vector<uint8_t> font_buffer(file_size);
  file.seekg(0, std::ios::beg);
  file.read(reinterpret_cast<char *>(font_buffer.data()), file_size);
  return font_buffer;
}

Font::Font(const std::filesystem::path &font_path) {
  this->font_path = font_path;
}

Image Font::generate_atlas() {
  constexpr int FIRST_CODEPOINT = 32;
  constexpr int LAST_CODEPOINT = 126;
  constexpr int NUM_GLYPHS = LAST_CODEPOINT - FIRST_CODEPOINT + 1;
  constexpr int ATLAS_SIZE = 512;

  struct GlyphBitmap {
    int codepoint;
    int width, height; // bitmap dimensions (without padding)
    int xoff, yoff;    // bearing from stbtt
    uint8_t *pixels;   // owned, freed after upload
  };

  std::vector<uint8_t> font_buffer = load_font(font_path);
  stbtt_fontinfo font_info;
  stbtt_InitFont(&font_info, font_buffer.data(), 0);
  int ascent_raw, descent_raw, line_gap_raw;
  stbtt_GetFontVMetrics(&font_info, &ascent_raw, &descent_raw, &line_gap_raw);
  font_scale = stbtt_ScaleForMappingEmToPixels(&font_info, sample_point_size);

  ascent = static_cast<int>(roundf(ascent_raw * font_scale));
  descent = static_cast<int>(roundf(descent_raw * font_scale));
  line_height = ascent - descent; // TODO: sample size

  std::vector<GlyphBitmap> bitmaps;
  bitmaps.reserve(NUM_GLYPHS);
  std::vector<stbrp_rect> pack_rects;
  pack_rects.reserve(NUM_GLYPHS);

  uint32_t available_pixels = ATLAS_SIZE * ATLAS_SIZE;
  uint32_t used_pixels = 0;

  for (int codepoint = FIRST_CODEPOINT; codepoint <= LAST_CODEPOINT;
       codepoint++) {
    GlyphBitmap glyph_bitmap{};
    glyph_bitmap.codepoint = codepoint;
    glyph_bitmap.pixels =
        stbtt_GetCodepointSDF( // TODO: Smoothing is not scaled properly
            &font_info, font_scale, codepoint, glyph_padding, 128, 64,
            &glyph_bitmap.width, &glyph_bitmap.height, &glyph_bitmap.xoff,
            &glyph_bitmap.yoff);
    bitmaps.push_back(glyph_bitmap);

    stbrp_rect rect{};
    rect.id = codepoint;
    rect.w = glyph_bitmap.width;
    rect.h = glyph_bitmap.height;
    pack_rects.push_back(rect);

    used_pixels += glyph_bitmap.width * glyph_bitmap.height;
  }

  SDL_Log("Atlas usage for font %s: %.2f%%", font_path.c_str(),
          static_cast<float>(used_pixels) /
              static_cast<float>(available_pixels) * 100.0f);

  stbrp_context pack_ctx{};
  std::vector<stbrp_node> pack_nodes(ATLAS_SIZE);
  stbrp_init_target(&pack_ctx, ATLAS_SIZE, ATLAS_SIZE, pack_nodes.data(),
                    ATLAS_SIZE);
  stbrp_pack_rects(&pack_ctx, pack_rects.data(),
                   static_cast<int>(pack_rects.size()));

  Image atlas{};
  atlas.width = ATLAS_SIZE;
  atlas.height = ATLAS_SIZE;
  atlas.channels = 4;
  atlas.pixel_format = PixelFormat::RGBA8;
  atlas.pixels.resize(ATLAS_SIZE * ATLAS_SIZE * atlas.channels, 0);

  for (int i = 0; i < NUM_GLYPHS; i++) {
    const GlyphBitmap &glyph_bitmap = bitmaps[i];
    const stbrp_rect &rect = pack_rects[i];

    int adv_raw, lsb_raw;
    stbtt_GetCodepointHMetrics(&font_info, glyph_bitmap.codepoint, &adv_raw,
                               &lsb_raw);

    GlyphMetrics glyph_metric{};
    glyph_metric.size = glm::vec2(glyph_bitmap.width, glyph_bitmap.height);
    glyph_metric.bearing = glm::vec2(glyph_bitmap.xoff, glyph_bitmap.yoff);
    glyph_metric.advance = roundf(adv_raw * font_scale);

    if (!rect.was_packed) {
      glyph_metric.uv_rect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
      glyph_metrics[glyph_bitmap.codepoint] = glyph_metric;
      continue;
    }

    for (int gy = 0; gy < glyph_bitmap.height; gy++) {
      for (int gx = 0; gx < glyph_bitmap.width; gx++) {
        int ax = rect.x + gx;
        int ay = rect.y + gy;
        if (ax < 0 || ax >= ATLAS_SIZE || ay < 0 || ay >= ATLAS_SIZE)
          continue;
        int idx = (ay * ATLAS_SIZE + ax) * atlas.channels;
        atlas.pixels[idx + 0] = 255;
        atlas.pixels[idx + 1] = 255;
        atlas.pixels[idx + 2] = 255;
        atlas.pixels[idx + 3] =
            glyph_bitmap.pixels[gy * glyph_bitmap.width + gx];
      }
    }

    glyph_metric.uv_rect =
        glm::vec4(static_cast<float>(rect.x) / ATLAS_SIZE,
                  static_cast<float>(rect.y) / ATLAS_SIZE,
                  static_cast<float>(rect.x + rect.w) / ATLAS_SIZE,
                  static_cast<float>(rect.y + rect.h) / ATLAS_SIZE);
    glyph_metrics[glyph_bitmap.codepoint] = glyph_metric;

    if (glyph_bitmap.pixels) {
      stbtt_FreeBitmap(glyph_bitmap.pixels, nullptr);
    }
  }
  return atlas;
}
