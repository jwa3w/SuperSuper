#include "textures.hpp"
#include "math3d.hpp"
#include <cmath>
#include <cstdint>
#include <vector>

namespace hypertrack {

namespace {

GLuint uploadRGBA8(int w, int h, const std::vector<uint8_t>& pixels) {
  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  return tex;
}

inline float tileableNoise(int x, int y, int period) {
  int wx = ((x % period) + period) % period;
  int wy = ((y % period) + period) % period;
  return hash2(wx, wy);
}

float tileableSmoothNoise(float x, float y, int period) {
  int xi = static_cast<int>(std::floor(x));
  int yi = static_cast<int>(std::floor(y));
  float xf = x - static_cast<float>(xi);
  float yf = y - static_cast<float>(yi);
  float u = xf * xf * (3.0f - 2.0f * xf);
  float v = yf * yf * (3.0f - 2.0f * yf);

  float n00 = tileableNoise(xi, yi, period);
  float n10 = tileableNoise(xi + 1, yi, period);
  float n01 = tileableNoise(xi, yi + 1, period);
  float n11 = tileableNoise(xi + 1, yi + 1, period);
  return lerpf(lerpf(n00, n10, u), lerpf(n01, n11, u), v);
}

GLuint buildNormalMapFromHeight(int size, const std::vector<float>& height, float strength) {
  std::vector<uint8_t> pixels(size * size * 4);
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      int xm = (x - 1 + size) % size;
      int xp = (x + 1) % size;
      int ym = (y - 1 + size) % size;
      int yp = (y + 1) % size;

      float dx = (height[y * size + xp] - height[y * size + xm]) * strength;
      float dy = (height[yp * size + x] - height[ym * size + x]) * strength;
      Vec3 n = Vec3{-dx, -dy, 1.0f}.normalized();

      int idx = (y * size + x) * 4;
      pixels[idx + 0] = static_cast<uint8_t>(clampf((n.x * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
      pixels[idx + 1] = static_cast<uint8_t>(clampf((n.y * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
      pixels[idx + 2] = static_cast<uint8_t>(clampf((n.z * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
      pixels[idx + 3] = 255;
    }
  }
  return uploadRGBA8(size, size, pixels);
}

}  // namespace

void TextureSet::init() {
  // 1. Asphalt Albedo & Normal (256x256 micro-aggregate tarmac)
  {
    const int S = 256;
    std::vector<uint8_t> albedo(S * S * 4);
    std::vector<float> height(S * S);
    for (int y = 0; y < S; ++y) {
      for (int x = 0; x < S; ++x) {
        float u = static_cast<float>(x) / S;
        float v = static_cast<float>(y) / S;
        float fine = tileableNoise(x, y, S);
        float med = tileableSmoothNoise(u * 32.0f, v * 32.0f, 32);
        float coarse = tileableSmoothNoise(u * 8.0f, v * 8.0f, 8);
        float h = fine * 0.45f + med * 0.35f + coarse * 0.20f;
        height[y * S + x] = h;

        float base = 0.16f + h * 0.14f;
        // Subtle aggregate mineral specks
        float speck = (fine > 0.87f) ? 0.06f : 0.0f;
        uint8_t r = static_cast<uint8_t>(clampf((base + speck) * 255.0f, 0.0f, 255.0f));
        uint8_t g = static_cast<uint8_t>(clampf((base + speck * 0.95f) * 255.0f, 0.0f, 255.0f));
        uint8_t b = static_cast<uint8_t>(clampf((base * 1.04f + speck) * 255.0f, 0.0f, 255.0f));

        int idx = (y * S + x) * 4;
        albedo[idx + 0] = r;
        albedo[idx + 1] = g;
        albedo[idx + 2] = b;
        albedo[idx + 3] = 255;
      }
    }
    asphaltAlbedo = uploadRGBA8(S, S, albedo);
    asphaltNormal = buildNormalMapFromHeight(S, height, 2.2f);
  }

  // 2. FIA Painted Kerb Albedo & Normal (128x128)
  {
    const int S = 128;
    std::vector<uint8_t> albedo(S * S * 4);
    std::vector<float> height(S * S);
    for (int y = 0; y < S; ++y) {
      for (int x = 0; x < S; ++x) {
        float u = static_cast<float>(x) / S;
        float v = static_cast<float>(y) / S;
        bool redBand = (static_cast<int>(v * 2.0f) % 2) == 0;
        float scuff = tileableSmoothNoise(u * 16.0f, v * 16.0f, 16) * 0.25f;
        float ridge = std::sin(v * kTwoPi * 4.0f) * 0.5f + 0.5f;
        height[y * S + x] = ridge * 0.7f + tileableNoise(x, y, S) * 0.3f;

        Vec3 col = redBand ? Vec3{0.82f, 0.10f, 0.14f} : Vec3{0.92f, 0.92f, 0.94f};
        col = col * (1.0f - scuff * 0.55f);
        int idx = (y * S + x) * 4;
        albedo[idx + 0] = static_cast<uint8_t>(clampf(col.x * 255.0f, 0.0f, 255.0f));
        albedo[idx + 1] = static_cast<uint8_t>(clampf(col.y * 255.0f, 0.0f, 255.0f));
        albedo[idx + 2] = static_cast<uint8_t>(clampf(col.z * 255.0f, 0.0f, 255.0f));
        albedo[idx + 3] = 255;
      }
    }
    kerbAlbedo = uploadRGBA8(S, S, albedo);
    kerbNormal = buildNormalMapFromHeight(S, height, 1.8f);
  }

  // 3. Carbon Fiber 2x2 Twill Weave (128x128)
  {
    const int S = 128;
    std::vector<uint8_t> albedo(S * S * 4);
    std::vector<float> height(S * S);
    for (int y = 0; y < S; ++y) {
      for (int x = 0; x < S; ++x) {
        int tx = (x / 4) % 4;
        int ty = (y / 4) % 4;
        bool warp = ((tx + ty) % 4) < 2;
        float localU = static_cast<float>(x % 4) / 4.0f;
        float localV = static_cast<float>(y % 4) / 4.0f;
        float sheen = warp ? std::sin(localU * kPi) : std::sin(localV * kPi);
        height[y * S + x] = sheen * (warp ? 0.8f : 0.55f);

        float val = (warp ? 0.14f : 0.07f) + sheen * 0.09f;
        int idx = (y * S + x) * 4;
        albedo[idx + 0] = static_cast<uint8_t>(val * 255.0f);
        albedo[idx + 1] = static_cast<uint8_t>(val * 255.0f);
        albedo[idx + 2] = static_cast<uint8_t>((val * 1.08f) * 255.0f);
        albedo[idx + 3] = 255;
      }
    }
    carbonAlbedo = uploadRGBA8(S, S, albedo);
    carbonNormal = buildNormalMapFromHeight(S, height, 1.5f);
  }

  // 4. Terrain Grass / Gravel (256x256)
  {
    const int S = 256;
    std::vector<uint8_t> albedo(S * S * 4);
    std::vector<float> height(S * S);
    for (int y = 0; y < S; ++y) {
      for (int x = 0; x < S; ++x) {
        float u = static_cast<float>(x) / S;
        float v = static_cast<float>(y) / S;
        float n1 = tileableSmoothNoise(u * 12.0f, v * 12.0f, 12);
        float n2 = tileableSmoothNoise(u * 40.0f, v * 40.0f, 40);
        float n3 = tileableNoise(x, y, S);
        float h = n1 * 0.45f + n2 * 0.35f + n3 * 0.20f;
        height[y * S + x] = h;

        float g = 0.24f + h * 0.22f;
        float r = 0.14f + h * 0.13f;
        float b = 0.10f + h * 0.07f;
        int idx = (y * S + x) * 4;
        albedo[idx + 0] = static_cast<uint8_t>(clampf(r * 255.0f, 0.0f, 255.0f));
        albedo[idx + 1] = static_cast<uint8_t>(clampf(g * 255.0f, 0.0f, 255.0f));
        albedo[idx + 2] = static_cast<uint8_t>(clampf(b * 255.0f, 0.0f, 255.0f));
        albedo[idx + 3] = 255;
      }
    }
    terrainAlbedo = uploadRGBA8(S, S, albedo);
    terrainNormal = buildNormalMapFromHeight(S, height, 1.4f);
  }
}

void TextureSet::destroy() {
  GLuint texs[] = {asphaltAlbedo, asphaltNormal, kerbAlbedo, kerbNormal,
                   carbonAlbedo, carbonNormal, terrainAlbedo, terrainNormal};
  glDeleteTextures(8, texs);
  asphaltAlbedo = asphaltNormal = kerbAlbedo = kerbNormal = 0;
  carbonAlbedo = carbonNormal = terrainAlbedo = terrainNormal = 0;
}

}  // namespace hypertrack
