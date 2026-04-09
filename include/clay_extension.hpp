#pragma once

enum class BorderType {
  INSIDE,
  OUTSIDE,
  MIDDLE,
};

struct Clay_DropShadowConfig {
  // Draw if blur_radius > 0 or shadow_offset_x or shadow_offset_y are non-zero,
  // no need for flag? but that doesn't work for transparent panels
  float blur_radius = 0.0f;
  float shadow_offset_x = 0.0f;
  float shadow_offset_y = 0.0f;
};

struct Clay_PivotConfig {
  float pivot_x = 0.0f;
  float pivot_y = 0.0f;
};

struct Clay_TransformConfig {
  Clay_PivotConfig pivot;
  float scale = 1.0f;
  float rotation = 0.0f;
};

struct Clay_ExtensionConfig {
  Clay_DropShadowConfig dropShadow;
  BorderType borderType = BorderType::INSIDE;
  Clay_TransformConfig transform;
};
