#pragma once
#include <cmath>

class vec2 {
public:
  vec2();
  vec2(float x, float y);
  float x;
  float y;

  float dot(const vec2 &other) const;
  float length() const { return sqrt(x * x + y * y); }
  vec2 normalized() const;

  vec2 operator+(const vec2 &other) const;
  vec2 operator-(const vec2 &other) const;
  vec2 operator-() const;
  vec2 operator*(float scalar) const;
  vec2 operator/(float scalar) const;
  vec2 &operator+=(const vec2 &other);
  vec2 &operator-=(const vec2 &other);
};
