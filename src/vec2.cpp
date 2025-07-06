#include "vec2.h"

vec2::vec2() : x(0.0f), y(0.0f) {}

vec2::vec2(float x, float y)
    : x(static_cast<float>(x)), y(static_cast<float>(y)) {}

float vec2::dot(const vec2 &other) const { return x * other.x + y * other.y; }

vec2 vec2::normalized() const {
  if (x == 0.0f && y == 0.0f)
    return vec2(0.0f, 0.0f);
  float length = sqrt(x * x + y * y);
  return vec2(x / length, y / length);
};

vec2 vec2::operator+(const vec2 &other) const {
  return vec2(x + other.x, y + other.y);
}

vec2 vec2::operator-(const vec2 &other) const {
  return vec2(x - other.x, y - other.y);
}

vec2 vec2::operator-() const { return vec2(-x, -y); }

vec2 vec2::operator*(float scalar) const {
  return vec2(x * scalar, y * scalar);
}

vec2 vec2::operator/(float scalar) const {
  return vec2(x / scalar, y / scalar);
}

vec2 &vec2::operator+=(const vec2 &other) {
  x += other.x;
  y += other.y;
  return *this;
}

vec2 &vec2::operator-=(const vec2 &other) {
  x -= other.x;
  y -= other.y;
  return *this;
}
