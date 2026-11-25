#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace hypertrack {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

inline float clampf(float v, float lo, float hi) {
  return std::max(lo, std::min(hi, v));
}

inline float lerpf(float a, float b, float t) {
  return a + (b - a) * t;
}

inline float smoothstepf(float e0, float e1, float x) {
  float t = clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

struct Vec2 {
  float x = 0.0f;
  float y = 0.0f;

  Vec2() = default;
  Vec2(float x_, float y_) : x(x_), y(y_) {}

  Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
  Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
  Vec2 operator*(float s) const { return {x * s, y * s}; }
  float length() const { return std::sqrt(x * x + y * y); }
  Vec2 normalized() const {
    float len = length();
    return len > 1e-6f ? Vec2{x / len, y / len} : Vec2{0.0f, 0.0f};
  }
};

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;

  Vec3() = default;
  Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

  Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
  Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
  Vec3 operator-() const { return {-x, -y, -z}; }
  Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
  Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
  Vec3& operator+=(const Vec3& o) {
    x += o.x;
    y += o.y;
    z += o.z;
    return *this;
  }
  Vec3& operator-=(const Vec3& o) {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    return *this;
  }
  Vec3& operator*=(float s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
  }

  float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
  Vec3 cross(const Vec3& o) const {
    return {
        y * o.z - z * o.y,
        z * o.x - x * o.z,
        x * o.y - y * o.x,
    };
  }
  float lengthSq() const { return x * x + y * y + z * z; }
  float length() const { return std::sqrt(lengthSq()); }
  Vec3 normalized() const {
    float len = length();
    return len > 1e-6f ? Vec3{x / len, y / len, z / len} : Vec3{0.0f, 1.0f, 0.0f};
  }
};

inline Vec3 operator*(float s, const Vec3& v) { return {v.x * s, v.y * s, v.z * s}; }

struct Vec4 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float w = 0.0f;

  Vec4() = default;
  Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
};

inline Vec3 lerpVec3(const Vec3& a, const Vec3& b, float t) {
  return a + (b - a) * t;
}

inline Vec3 catmullRom(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t) {
  float t2 = t * t;
  float t3 = t2 * t;
  return 0.5f * ((2.0f * p1) +
                 (-p0 + p2) * t +
                 (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                 (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

struct Mat4 {
  float m[16] = {0};

  static Mat4 identity() {
    Mat4 r;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
  }

  static Mat4 translate(const Vec3& t) {
    Mat4 r = identity();
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
  }

  static Mat4 scale(const Vec3& s) {
    Mat4 r;
    r.m[0] = s.x;
    r.m[5] = s.y;
    r.m[10] = s.z;
    r.m[15] = 1.0f;
    return r;
  }

  static Mat4 rotateX(float rad) {
    Mat4 r = identity();
    float c = std::cos(rad);
    float s = std::sin(rad);
    r.m[5] = c;
    r.m[6] = s;
    r.m[9] = -s;
    r.m[10] = c;
    return r;
  }

  static Mat4 rotateY(float rad) {
    Mat4 r = identity();
    float c = std::cos(rad);
    float s = std::sin(rad);
    r.m[0] = c;
    r.m[2] = -s;
    r.m[8] = s;
    r.m[10] = c;
    return r;
  }

  static Mat4 rotateZ(float rad) {
    Mat4 r = identity();
    float c = std::cos(rad);
    float s = std::sin(rad);
    r.m[0] = c;
    r.m[1] = s;
    r.m[4] = -s;
    r.m[5] = c;
    return r;
  }

  static Mat4 fromBasis(const Vec3& right, const Vec3& up, const Vec3& forward, const Vec3& pos) {
    Mat4 r = identity();
    r.m[0] = right.x;
    r.m[1] = right.y;
    r.m[2] = right.z;
    r.m[4] = up.x;
    r.m[5] = up.y;
    r.m[6] = up.z;
    r.m[8] = forward.x;
    r.m[9] = forward.y;
    r.m[10] = forward.z;
    r.m[12] = pos.x;
    r.m[13] = pos.y;
    r.m[14] = pos.z;
    return r;
  }

  static Mat4 perspective(float fovYRad, float aspect, float zNear, float zFar) {
    Mat4 r;
    float tanHalf = std::tan(fovYRad * 0.5f);
    r.m[0] = 1.0f / (aspect * tanHalf);
    r.m[5] = 1.0f / tanHalf;
    r.m[10] = -(zFar + zNear) / (zFar - zNear);
    r.m[11] = -1.0f;
    r.m[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
    return r;
  }

  static Mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
    Mat4 r = identity();
    r.m[0] = 2.0f / (right - left);
    r.m[5] = 2.0f / (top - bottom);
    r.m[10] = -2.0f / (zFar - zNear);
    r.m[12] = -(right + left) / (right - left);
    r.m[13] = -(top + bottom) / (top - bottom);
    r.m[14] = -(zFar + zNear) / (zFar - zNear);
    return r;
  }

  static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    Vec3 f = (target - eye).normalized();
    Vec3 s = f.cross(up).normalized();
    Vec3 u = s.cross(f);

    Mat4 r = identity();
    r.m[0] = s.x;
    r.m[4] = s.y;
    r.m[8] = s.z;
    r.m[1] = u.x;
    r.m[5] = u.y;
    r.m[9] = u.z;
    r.m[2] = -f.x;
    r.m[6] = -f.y;
    r.m[10] = -f.z;
    r.m[12] = -s.dot(eye);
    r.m[13] = -u.dot(eye);
    r.m[14] = f.dot(eye);
    return r;
  }

  Mat4 operator*(const Mat4& b) const {
    Mat4 r;
    for (int col = 0; col < 4; ++col) {
      for (int row = 0; row < 4; ++row) {
        r.m[col * 4 + row] =
            m[0 * 4 + row] * b.m[col * 4 + 0] +
            m[1 * 4 + row] * b.m[col * 4 + 1] +
            m[2 * 4 + row] * b.m[col * 4 + 2] +
            m[3 * 4 + row] * b.m[col * 4 + 3];
      }
    }
    return r;
  }
};

// Fast deterministic procedural noise for terrain, asphalt, and rain puddles
inline float hash2(int x, int y) {
  uint32_t n = static_cast<uint32_t>(x * 374761393 + y * 668265263);
  n = (n ^ (n >> 13)) * 1274126177u;
  return static_cast<float>((n ^ (n >> 16)) & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

inline float valueNoise2D(float x, float y) {
  int xi = static_cast<int>(std::floor(x));
  int yi = static_cast<int>(std::floor(y));
  float xf = x - static_cast<float>(xi);
  float yf = y - static_cast<float>(yi);
  float u = xf * xf * (3.0f - 2.0f * xf);
  float v = yf * yf * (3.0f - 2.0f * yf);

  float n00 = hash2(xi, yi);
  float n10 = hash2(xi + 1, yi);
  float n01 = hash2(xi, yi + 1);
  float n11 = hash2(xi + 1, yi + 1);

  return lerpf(lerpf(n00, n10, u), lerpf(n01, n11, u), v);
}

inline float fbm2D(float x, float y, int octaves = 4) {
  float sum = 0.0f;
  float amp = 0.5f;
  float freq = 1.0f;
  for (int i = 0; i < octaves; ++i) {
    sum += amp * valueNoise2D(x * freq, y * freq);
    freq *= 2.03f;
    amp *= 0.5f;
  }
  return sum;
}

}  // namespace hypertrack
