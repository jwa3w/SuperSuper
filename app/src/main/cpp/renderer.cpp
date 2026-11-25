#include "renderer.hpp"
#include <android/log.h>
#include <cmath>
#include <ctime>

#define LOG_TAG "HyperTrackRenderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace hypertrack {

namespace {

double nowSeconds() {
  timespec ts{};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) * 1e-9;
}

float rand01(uint32_t& seed) {
  seed = seed * 1664525u + 1013904223u;
  return static_cast<float>((seed >> 8) & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

void computeNormalMatrix(const Mat4& model, float out3x3[9]) {
  // Upper 3x3 inverse transpose (for rigid + uniform scale, inverse transpose of orthogonal is itself,
  // but computing full 3x3 adjugate/det handles any scale cleanly)
  float a00 = model.m[0], a01 = model.m[4], a02 = model.m[8];
  float a10 = model.m[1], a11 = model.m[5], a12 = model.m[9];
  float a20 = model.m[2], a21 = model.m[6], a22 = model.m[10];

  float c00 = a11 * a22 - a12 * a21;
  float c01 = -(a10 * a22 - a12 * a20);
  float c02 = a10 * a21 - a11 * a20;
  float det = a00 * c00 + a01 * c01 + a02 * c02;
  float invDet = std::abs(det) > 1e-6f ? (1.0f / det) : 1.0f;

  // Transpose of cofactor matrix * invDet (column-major for GLSL mat3)
  out3x3[0] = c00 * invDet;
  out3x3[1] = c01 * invDet;
  out3x3[2] = c02 * invDet;
  out3x3[3] = -(a01 * a22 - a02 * a21) * invDet;
  out3x3[4] = (a00 * a22 - a02 * a20) * invDet;
  out3x3[5] = -(a00 * a21 - a01 * a20) * invDet;
  out3x3[6] = (a01 * a12 - a02 * a11) * invDet;
  out3x3[7] = -(a00 * a12 - a02 * a10) * invDet;
  out3x3[8] = (a00 * a11 - a01 * a10) * invDet;
}

struct LightingEnv {
  Vec3 sunDir;
  Vec3 sunColor;
  Vec3 skyZenith;
  Vec3 skyHorizon;
  float nightFactor;
  float cloudDensity;
  Vec3 barrierEmissive;
};

LightingEnv getLightingEnv(TimeOfDay tod, WeatherMode weather) {
  LightingEnv env;
  if (tod == TimeOfDay::GOLDEN_SUNSET) {
    env.sunDir = Vec3{0.65f, 0.21f, -0.73f}.normalized();
    env.sunColor = Vec3{2.8f, 1.45f, 0.72f};
    env.skyZenith = Vec3{0.08f, 0.16f, 0.36f};
    env.skyHorizon = Vec3{0.82f, 0.42f, 0.24f};
    env.nightFactor = 0.22f;
    env.cloudDensity = 0.45f;
    env.barrierEmissive = Vec3{1.1f, 0.42f, 0.12f};
  } else if (tod == TimeOfDay::MIDNIGHT_NEON) {
    env.sunDir = Vec3{-0.4f, 0.48f, -0.78f}.normalized();
    env.sunColor = Vec3{0.55f, 0.72f, 1.05f};
    env.skyZenith = Vec3{0.015f, 0.025f, 0.065f};
    env.skyHorizon = Vec3{0.05f, 0.10f, 0.18f};
    env.nightFactor = 1.0f;
    env.cloudDensity = 0.35f;
    env.barrierEmissive = Vec3{0.05f, 1.4f, 1.9f};
  } else if (tod == TimeOfDay::ALPINE_OVERCAST) {
    env.sunDir = Vec3{0.35f, 0.62f, -0.70f}.normalized();
    env.sunColor = Vec3{1.25f, 1.32f, 1.45f};
    env.skyZenith = Vec3{0.22f, 0.28f, 0.36f};
    env.skyHorizon = Vec3{0.48f, 0.54f, 0.60f};
    env.nightFactor = 0.15f;
    env.cloudDensity = 0.85f;
    env.barrierEmissive = Vec3{0.9f, 0.22f, 0.28f};
  } else {
    // HIGH_NOON
    env.sunDir = Vec3{0.35f, 0.82f, -0.45f}.normalized();
    env.sunColor = Vec3{2.7f, 2.55f, 2.35f};
    env.skyZenith = Vec3{0.12f, 0.36f, 0.78f};
    env.skyHorizon = Vec3{0.56f, 0.74f, 0.90f};
    env.nightFactor = 0.0f;
    env.cloudDensity = 0.25f;
    env.barrierEmissive = Vec3{0.7f, 0.18f, 0.22f};
  }

  if (weather == WeatherMode::RAIN_SHOWER) {
    env.sunColor *= 0.65f;
    env.skyZenith = lerpVec3(env.skyZenith, Vec3{0.09f, 0.12f, 0.17f}, 0.55f);
    env.skyHorizon = lerpVec3(env.skyHorizon, Vec3{0.24f, 0.28f, 0.34f}, 0.5f);
    env.cloudDensity = std::max(env.cloudDensity, 0.88f);
  }
  return env;
}

MaterialParams getLiveryMaterial(CarLivery livery) {
  MaterialParams m;
  m.surfaceType = 5;
  m.clearcoat = 1.0f;
  m.clearcoatRoughness = 0.045f;
  switch (livery) {
    case CarLivery::ROSSO_CORSA:
      m.baseColor = {0.82f, 0.045f, 0.07f};
      m.metallic = 0.36f;
      m.roughness = 0.18f;
      break;
    case CarLivery::STEALTH_CARBON:
      m.baseColor = {0.07f, 0.08f, 0.10f};
      m.metallic = 0.65f;
      m.roughness = 0.28f;
      m.clearcoat = 0.45f;
      m.clearcoatRoughness = 0.12f;
      break;
    case CarLivery::LIQUID_SILVER:
      m.baseColor = {0.80f, 0.83f, 0.87f};
      m.metallic = 0.94f;
      m.roughness = 0.11f;
      break;
    case CarLivery::APEX_CYAN:
      m.baseColor = {0.02f, 0.56f, 0.86f};
      m.metallic = 0.52f;
      m.roughness = 0.16f;
      break;
    case CarLivery::MIDNIGHT_EMERALD:
      m.baseColor = {0.025f, 0.32f, 0.18f};
      m.metallic = 0.54f;
      m.roughness = 0.17f;
      break;
  }
  return m;
}

}  // namespace

void HyperTrackEngine::initGL() {
  std::lock_guard<std::mutex> lock(mutex_);
  destroyGL();

  skyProg_.id = linkProgram(kSkyVert, kSkyFrag);
  shadowProg_.id = linkProgram(kShadowVert, kShadowFrag);
  pbrProg_.id = linkProgram(kPbrVert, kPbrFrag);
  particleProg_.id = linkProgram(kParticleVert, kParticleFrag);
  brightProg_.id = linkProgram(kQuadVert, kBrightPassFrag);
  blurProg_.id = linkProgram(kQuadVert, kBlurFrag);
  compositeProg_.id = linkProgram(kQuadVert, kCompositeFrag);

  textures_.init();
  setupSkyAndQuad();

  track_.build(activeTrack_);
  carMesh_.build();
  physics_.resetToTrack(track_);

  // Particle pool & dynamic buffers (up to 400 quads)
  particles_.resize(400);
  particleVertData_.resize(400 * 4 * 9);
  std::vector<uint32_t> pIdx;
  pIdx.reserve(400 * 6);
  for (uint32_t i = 0; i < 400; ++i) {
    uint32_t b = i * 4;
    pIdx.push_back(b + 0); pIdx.push_back(b + 1); pIdx.push_back(b + 2);
    pIdx.push_back(b + 0); pIdx.push_back(b + 2); pIdx.push_back(b + 3);
  }
  glGenVertexArrays(1, &particleVao_);
  glGenBuffers(1, &particleVbo_);
  glGenBuffers(1, &particleEbo_);
  glBindVertexArray(particleVao_);
  glBindBuffer(GL_ARRAY_BUFFER, particleVbo_);
  glBufferData(GL_ARRAY_BUFFER, particleVertData_.size() * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, particleEbo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, pIdx.size() * sizeof(uint32_t), pIdx.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                        reinterpret_cast<void*>(3 * sizeof(float)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                        reinterpret_cast<void*>(5 * sizeof(float)));
  glBindVertexArray(0);

  setupFramebuffers();

  lastTimeSec_ = nowSeconds();
  glReady_ = true;
  LOGI("HyperTrackEngine OpenGL ES 3.0 initialized successfully");
}

void HyperTrackEngine::setupSkyAndQuad() {
  static const float skyVerts[] = {
      -1, 1, -1, -1, -1, -1, 1, -1, -1, 1, -1, -1, 1, 1, -1, -1, 1, -1,
      -1, -1, 1, -1, -1, -1, -1, 1, -1, -1, 1, -1, -1, 1, 1, -1, -1, 1,
      1, -1, -1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, -1, -1,
      -1, -1, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 1, -1, -1, 1,
      -1, 1, -1, 1, 1, -1, 1, 1, 1, 1, 1, 1, -1, 1, 1, -1, 1, -1,
      -1, -1, -1, -1, -1, 1, 1, -1, -1, 1, -1, -1, -1, -1, 1, 1, -1, 1};
  glGenVertexArrays(1, &skyVao_);
  glGenBuffers(1, &skyVbo_);
  glBindVertexArray(skyVao_);
  glBindBuffer(GL_ARRAY_BUFFER, skyVbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyVerts), skyVerts, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glBindVertexArray(0);

  static const float quadVerts[] = {
      -1, -1, 0, 0,
       1, -1, 1, 0,
       1,  1, 1, 1,
      -1, -1, 0, 0,
       1,  1, 1, 1,
      -1,  1, 0, 1};
  glGenVertexArrays(1, &quadVao_);
  glGenBuffers(1, &quadVbo_);
  glBindVertexArray(quadVao_);
  glBindBuffer(GL_ARRAY_BUFFER, quadVbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        reinterpret_cast<void*>(2 * sizeof(float)));
  glBindVertexArray(0);
}

void HyperTrackEngine::setupFramebuffers() {
  destroyFramebuffers();

  // 1. Shadow Depth FBO (1024x1024)
  glGenFramebuffers(1, &shadowFbo_);
  glGenTextures(1, &shadowTex_);
  glBindTexture(GL_TEXTURE_2D, shadowTex_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1024, 1024, 0, GL_DEPTH_COMPONENT,
               GL_UNSIGNED_INT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex_, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    glBindTexture(GL_TEXTURE_2D, shadowTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 1024, 1024, 0, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_SHORT, nullptr);
  }

  // 2. Main Scene FBO (try RGBA16F, gracefully fall back to RGBA8 if unsupported)
  glGenFramebuffers(1, &sceneFbo_);
  glGenTextures(1, &sceneColorTex_);
  glGenRenderbuffers(1, &sceneDepthRbo_);

  glBindTexture(GL_TEXTURE_2D, sceneColorTex_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width_, height_, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthRbo_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width_, height_);

  glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTex_, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, sceneDepthRbo_);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    glBindTexture(GL_TEXTURE_2D, sceneColorTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  }

  // 3. Ping-pong Bloom FBOs (quarter-res)
  bloomW_ = std::max(width_ / 4, 64);
  bloomH_ = std::max(height_ / 4, 64);
  glGenFramebuffers(2, bloomFbo_);
  glGenTextures(2, bloomTex_);
  for (int i = 0; i < 2; ++i) {
    glBindTexture(GL_TEXTURE_2D, bloomTex_[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bloomW_, bloomH_, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo_[i]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomTex_[i], 0);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void HyperTrackEngine::destroyFramebuffers() {
  if (shadowTex_) glDeleteTextures(1, &shadowTex_);
  if (shadowFbo_) glDeleteFramebuffers(1, &shadowFbo_);
  if (sceneColorTex_) glDeleteTextures(1, &sceneColorTex_);
  if (sceneDepthRbo_) glDeleteRenderbuffers(1, &sceneDepthRbo_);
  if (sceneFbo_) glDeleteFramebuffers(1, &sceneFbo_);
  glDeleteTextures(2, bloomTex_);
  glDeleteFramebuffers(2, bloomFbo_);
  shadowTex_ = shadowFbo_ = sceneColorTex_ = sceneDepthRbo_ = sceneFbo_ = 0;
  bloomTex_[0] = bloomTex_[1] = bloomFbo_[0] = bloomFbo_[1] = 0;
}

void HyperTrackEngine::destroyGL() {
  glReady_ = false;
  destroyFramebuffers();
  textures_.destroy();
  track_.destroy();
  carMesh_.destroy();
  if (skyVao_) glDeleteVertexArrays(1, &skyVao_);
  if (skyVbo_) glDeleteBuffers(1, &skyVbo_);
  if (quadVao_) glDeleteVertexArrays(1, &quadVao_);
  if (quadVbo_) glDeleteBuffers(1, &quadVbo_);
  if (particleVao_) glDeleteVertexArrays(1, &particleVao_);
  if (particleVbo_) glDeleteBuffers(1, &particleVbo_);
  if (particleEbo_) glDeleteBuffers(1, &particleEbo_);
  skyVao_ = skyVbo_ = quadVao_ = quadVbo_ = particleVao_ = particleVbo_ = particleEbo_ = 0;

  GLuint progs[] = {skyProg_.id, shadowProg_.id, pbrProg_.id, particleProg_.id,
                    brightProg_.id, blurProg_.id, compositeProg_.id};
  for (GLuint p : progs) {
    if (p) glDeleteProgram(p);
  }
  skyProg_.id = shadowProg_.id = pbrProg_.id = particleProg_.id = 0;
  brightProg_.id = blurProg_.id = compositeProg_.id = 0;
}

void HyperTrackEngine::resize(int width, int height) {
  std::lock_guard<std::mutex> lock(mutex_);
  width_ = std::max(width, 64);
  height_ = std::max(height, 64);
  if (glReady_) {
    setupFramebuffers();
  }
}

void HyperTrackEngine::setDriverInput(const DriverInput& input) {
  std::lock_guard<std::mutex> lock(mutex_);
  driverInput_ = input;
}

void HyperTrackEngine::addOrbitTouch(float dx, float dy, float pinchScale) {
  std::lock_guard<std::mutex> lock(mutex_);
  orbitYaw_ -= dx * 0.0065f;
  orbitPitch_ = clampf(orbitPitch_ + dy * 0.005f, 0.05f, 1.15f);
  if (pinchScale > 0.01f) {
    orbitDist_ = clampf(orbitDist_ / pinchScale, 3.2f, 16.0f);
  }
  orbitIdleTime_ = 0.0f;
}

void HyperTrackEngine::setTrack(TrackId id) {
  std::lock_guard<std::mutex> lock(mutex_);
  pendingTrack_ = id;
}

void HyperTrackEngine::setLivery(CarLivery livery) {
  std::lock_guard<std::mutex> lock(mutex_);
  livery_ = livery;
}

void HyperTrackEngine::setWeather(WeatherMode weather) {
  std::lock_guard<std::mutex> lock(mutex_);
  weather_ = weather;
}

void HyperTrackEngine::setTimeOfDay(TimeOfDay tod) {
  std::lock_guard<std::mutex> lock(mutex_);
  timeOfDay_ = tod;
}

void HyperTrackEngine::setCameraMode(CameraMode cam) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (cam == CameraMode::SHOWCASE_CURB && cameraMode_ != CameraMode::SHOWCASE_CURB) {
    int idx = 0;
    track_.closestSample(physics_.state().position, idx);
    curbAnchorIdx_ = (idx + 18) % static_cast<int>(track_.samples().size());
  }
  cameraMode_ = cam;
}

void HyperTrackEngine::setShowcaseCruise(bool cruising) {
  std::lock_guard<std::mutex> lock(mutex_);
  showcaseCruising_ = cruising;
}

void HyperTrackEngine::setGraphicsSettings(const GraphicsSettings& settings) {
  std::lock_guard<std::mutex> lock(mutex_);
  graphics_ = settings;
}

void HyperTrackEngine::resetCar() {
  std::lock_guard<std::mutex> lock(mutex_);
  physics_.resetToTrack(track_);
  for (auto& p : particles_) p.active = false;
}

void HyperTrackEngine::spawnParticle(const Vec3& pos, const Vec3& vel, const Vec4& color,
                                     float size, float stretch, float maxLife) {
  for (auto& p : particles_) {
    if (!p.active) {
      p.active = true;
      p.pos = pos;
      p.vel = vel;
      p.color = color;
      p.size = size;
      p.stretch = stretch;
      p.life = 0.0f;
      p.maxLife = maxLife;
      return;
    }
  }
}

void HyperTrackEngine::updateParticles(float dt) {
  static uint32_t rng = 918273u;
  const auto& st = physics_.state();

  Vec3 carFwd{std::sin(st.heading), 0.0f, std::cos(st.heading)};
  Vec3 carRight = carFwd.cross(Vec3{0.0f, 1.0f, 0.0f}).normalized();

  // Rain shower streaks around camera
  if (weather_ == WeatherMode::RAIN_SHOWER && graphics_.rainParticlesEnabled) {
    for (int i = 0; i < 6; ++i) {
      Vec3 offset{
          (rand01(rng) - 0.5f) * 32.0f,
          8.0f + rand01(rng) * 10.0f,
          (rand01(rng) - 0.5f) * 32.0f,
      };
      Vec3 rPos = camPos_ + camTarget_ * 0.0f + offset;
      // Bias slightly ahead of camera look direction
      Vec3 lookDir = (camTarget_ - camPos_).normalized();
      rPos += lookDir * 9.0f;
      spawnParticle(rPos, Vec3{-1.2f, -24.0f, -0.8f}, Vec4{0.75f, 0.85f, 0.96f, 0.45f},
                    0.035f, 14.0f, 0.65f);
    }
  }

  // Wet tire water spray / drift tire smoke
  if (st.speedMs > 10.0f && (weather_ != WeatherMode::DRY || std::abs(st.driftAngleDeg) > 9.0f)) {
    bool isSpray = (weather_ != WeatherMode::DRY);
    for (float side : {-0.86f, 0.86f}) {
      Vec3 rearWheel = st.position + carRight * side - carFwd * 1.42f + Vec3{0.0f, 0.22f, 0.0f};
      Vec3 jitter{(rand01(rng) - 0.5f) * 0.8f, 0.6f + rand01(rng) * 1.2f, (rand01(rng) - 0.5f) * 0.8f};
      Vec4 col = isSpray ? Vec4{0.78f, 0.84f, 0.90f, 0.30f} : Vec4{0.85f, 0.86f, 0.88f, 0.34f};
      spawnParticle(rearWheel, -carFwd * (st.speedMs * 0.25f) + jitter, col,
                    isSpray ? 0.38f : 0.55f, 1.0f, 0.55f);
    }
  }

  // Nitro / upshift exhaust flame
  if (st.nitroActive || st.backfireTimer > 0.0f) {
    for (float side : {-0.22f, 0.22f}) {
      Vec3 tip = st.position + carRight * side - carFwd * 2.28f + Vec3{0.0f, 0.34f, 0.0f};
      Vec4 flameCol = st.nitroActive ? Vec4{0.18f, 0.85f, 1.0f, 0.88f}
                                     : Vec4{1.0f, 0.48f, 0.10f, 0.88f};
      spawnParticle(tip, -carFwd * 6.0f + Vec3{(rand01(rng) - 0.5f) * 0.4f, 0.1f, 0.0f},
                    flameCol, 0.24f, 1.5f, 0.12f);
    }
  }

  for (auto& p : particles_) {
    if (!p.active) continue;
    p.life += dt;
    if (p.life >= p.maxLife) {
      p.active = false;
      continue;
    }
    p.pos += p.vel * dt;
    if (p.stretch < 2.0f) {
      p.size += dt * 0.65f;
    }
  }
}

void HyperTrackEngine::updateCamera(float dt) {
  const auto& st = physics_.state();
  Vec3 carFwd{std::sin(st.heading), 0.0f, std::cos(st.heading)};
  Vec3 carRight = carFwd.cross(Vec3{0.0f, 1.0f, 0.0f}).normalized();
  Vec3 carUp = st.surfaceNormal;

  Vec3 desiredPos = camPos_;
  Vec3 desiredTarget = st.position + Vec3{0.0f, 0.65f, 0.0f};
  float targetFov = 58.0f + clampf(st.speedKmh / 280.0f, 0.0f, 1.0f) * 14.0f +
                    (st.nitroActive ? 6.0f : 0.0f);

  switch (cameraMode_) {
    case CameraMode::CHASE: {
      desiredPos = st.position - carFwd * 5.8f + carUp * 2.15f;
      desiredTarget = st.position + carFwd * 4.5f + carUp * 0.85f;
      camPos_ = lerpVec3(camPos_, desiredPos, clampf(dt * 9.5f, 0.0f, 1.0f));
      camTarget_ = lerpVec3(camTarget_, desiredTarget, clampf(dt * 12.0f, 0.0f, 1.0f));
      break;
    }
    case CameraMode::HOOD: {
      camPos_ = st.position + carFwd * 0.35f + carUp * 1.18f;
      camTarget_ = st.position + carFwd * 16.0f + carUp * 0.85f;
      break;
    }
    case CameraMode::BUMPER: {
      camPos_ = st.position + carFwd * 2.25f + carUp * 0.42f;
      camTarget_ = st.position + carFwd * 18.0f + carUp * 0.55f;
      targetFov += 6.0f;
      break;
    }
    case CameraMode::SHOWCASE_ORBIT: {
      orbitIdleTime_ += dt;
      if (orbitIdleTime_ > 1.8f) {
        orbitYaw_ += dt * 0.24f;
      }
      float cy = std::cos(orbitPitch_);
      float sy = std::sin(orbitPitch_);
      Vec3 offset{
          std::sin(orbitYaw_) * cy * orbitDist_,
          sy * orbitDist_ + 0.55f,
          std::cos(orbitYaw_) * cy * orbitDist_,
      };
      desiredPos = st.position + offset;
      desiredTarget = st.position + Vec3{0.0f, 0.55f, 0.0f};
      camPos_ = lerpVec3(camPos_, desiredPos, clampf(dt * 12.0f, 0.0f, 1.0f));
      camTarget_ = lerpVec3(camTarget_, desiredTarget, clampf(dt * 14.0f, 0.0f, 1.0f));
      targetFov = 50.0f;
      break;
    }
    case CameraMode::SHOWCASE_DRONE: {
      float angle = totalTime_ * 0.32f;
      float radius = 11.5f + std::sin(totalTime_ * 0.45f) * 3.5f;
      float height = 4.5f + std::cos(totalTime_ * 0.52f) * 2.0f;
      desiredPos = st.position + Vec3{std::sin(angle) * radius, height, std::cos(angle) * radius};
      desiredTarget = st.position + carFwd * 2.5f + Vec3{0.0f, 0.6f, 0.0f};
      camPos_ = lerpVec3(camPos_, desiredPos, clampf(dt * 5.0f, 0.0f, 1.0f));
      camTarget_ = lerpVec3(camTarget_, desiredTarget, clampf(dt * 7.0f, 0.0f, 1.0f));
      targetFov = 54.0f;
      break;
    }
    case CameraMode::SHOWCASE_CURB: {
      const auto& samples = track_.samples();
      if (!samples.empty()) {
        int nearest = 0;
        track_.closestSample(st.position, nearest);
        int n = static_cast<int>(samples.size());
        int diff = (curbAnchorIdx_ - nearest + n) % n;
        if (diff > n / 2) diff -= n;
        if (diff < -18 || diff > 45) {
          curbAnchorIdx_ = (nearest + 26) % n;
        }
        const auto& cs = samples[curbAnchorIdx_];
        camPos_ = cs.pos + cs.right * (cs.halfWidth + 2.2f) + cs.up * 1.45f;
        camTarget_ = st.position + Vec3{0.0f, 0.55f, 0.0f};
        float dist = (camTarget_ - camPos_).length();
        targetFov = clampf(54.0f - dist * 0.42f, 24.0f, 52.0f);
      }
      break;
    }
  }
  camFovDeg_ = lerpf(camFovDeg_, targetFov, clampf(dt * 8.0f, 0.0f, 1.0f));
  (void)carRight;
}

void HyperTrackEngine::renderFrame() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!glReady_) return;

  if (pendingTrack_ != activeTrack_) {
    activeTrack_ = pendingTrack_;
    track_.build(activeTrack_);
    physics_.resetToTrack(track_);
    for (auto& p : particles_) p.active = false;
  }

  double now = nowSeconds();
  float frameDt = static_cast<float>(now - lastTimeSec_);
  lastTimeSec_ = now;
  frameDt = clampf(frameDt, 0.001f, 0.05f);
  totalTime_ += frameDt;

  fpsAccum_ += frameDt;
  fpsFrames_++;
  if (fpsAccum_ >= 0.5f) {
    fps_ = static_cast<float>(fpsFrames_) / fpsAccum_;
    fpsAccum_ = 0.0f;
    fpsFrames_ = 0;
  }

  bool isShowcase = (cameraMode_ == CameraMode::SHOWCASE_ORBIT ||
                     cameraMode_ == CameraMode::SHOWCASE_DRONE ||
                     cameraMode_ == CameraMode::SHOWCASE_CURB);

  // Fixed-timestep physics integration (120Hz substeps)
  const float subDt = 1.0f / 120.0f;
  int steps = std::max(1, static_cast<int>(std::ceil(frameDt / subDt)));
  float actualStep = frameDt / static_cast<float>(steps);
  for (int s = 0; s < steps; ++s) {
    physics_.step(actualStep, track_, driverInput_, weather_, isShowcase,
                  isShowcase && !showcaseCruising_);
  }

  updateCamera(frameDt);
  updateParticles(frameDt);

  const auto& st = physics_.state();
  LightingEnv env = getLightingEnv(timeOfDay_, weather_);

  // Build Car Chassis Model Matrix (nose = -Z in car_mesh, so negate forward!)
  Vec3 carFwd{std::sin(st.heading), 0.0f, std::cos(st.heading)};
  // Project onto road normal
  Vec3 alignedFwd = (carFwd - st.surfaceNormal * carFwd.dot(st.surfaceNormal)).normalized();
  Vec3 alignedRight = alignedFwd.cross(st.surfaceNormal).normalized();
  Vec3 alignedUp = alignedRight.cross(alignedFwd).normalized();

  Mat4 carBasis = Mat4::fromBasis(alignedRight, alignedUp, -alignedFwd, st.position);
  Mat4 suspRot = Mat4::rotateX(st.pitch) * Mat4::rotateZ(st.roll);
  Mat4 carModel = carBasis * suspRot;

  // 4 Wheel Transforms (FL, FR, RL, RR)
  // Note: car nose is -Z, tail is +Z, right is +X, left is -X
  const Vec3 wheelOffsets[4] = {
      {-0.86f, 0.36f, -1.42f},  // Front Left
      {0.86f, 0.36f, -1.42f},   // Front Right
      {-0.89f, 0.36f, 1.42f},   // Rear Left
      {0.89f, 0.36f, 1.42f},    // Rear Right
  };
  std::array<Mat4, 4> wheelModels;
  std::array<Mat4, 4> caliperModels;
  for (int w = 0; w < 4; ++w) {
    float steer = (w < 2) ? st.steerAngle : 0.0f;
    // Wheel spin around local +X: forward (-Z) rolling moves top toward -Z (negative X rotation)
    Mat4 hub = carBasis * Mat4::translate(wheelOffsets[w]) * Mat4::rotateY(steer);
    float flipY = (w % 2 == 0) ? kPi : 0.0f;
    caliperModels[w] = hub * Mat4::rotateY(flipY);
    wheelModels[w] = hub * Mat4::rotateX(-st.wheelSpin) * Mat4::rotateY(flipY);
  }

  // Light space matrix centered on the car for crisp dynamic car shadows
  Vec3 shadowTarget = st.position;
  Vec3 shadowEye = shadowTarget + env.sunDir * 45.0f;
  Mat4 lightView = Mat4::lookAt(shadowEye, shadowTarget, Vec3{0.0f, 1.0f, 0.0f});
  Mat4 lightProj = Mat4::ortho(-22.0f, 22.0f, -22.0f, 22.0f, 1.0f, 95.0f);
  Mat4 lightSpace = lightProj * lightView;

  float aspect = static_cast<float>(width_) / static_cast<float>(std::max(height_, 1));
  Mat4 proj = Mat4::perspective(camFovDeg_ * (kPi / 180.0f), aspect, 0.2f, 950.0f);
  Mat4 view = Mat4::lookAt(camPos_, camTarget_, Vec3{0.0f, 1.0f, 0.0f});

  if (graphics_.shadowsEnabled) {
    renderShadowPass(lightSpace, carModel, wheelModels);
  }
  renderScenePass(view, proj, lightSpace, carModel, wheelModels, caliperModels);
  renderPostProcess();
}

void HyperTrackEngine::renderShadowPass(const Mat4& lightSpace, const Mat4& carModel,
                                        const std::array<Mat4, 4>& wheelModels) {
  glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo_);
  glViewport(0, 0, 1024, 1024);
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glCullFace(GL_FRONT);

  glUseProgram(shadowProg_.id);
  glUniformMatrix4fv(shadowProg_.loc("uLightSpaceMatrix"), 1, GL_FALSE, lightSpace.m);

  GLint modelLoc = shadowProg_.loc("uModel");
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, carModel.m);
  carMesh_.bodyShell().draw();
  carMesh_.glassCanopy().draw();
  carMesh_.carbonAero().draw();

  for (int w = 0; w < 4; ++w) {
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, wheelModels[w].m);
    carMesh_.wheelTire().draw();
  }

  Mat4 ident = Mat4::identity();
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, ident.m);
  track_.structureMesh().draw();

  glCullFace(GL_BACK);
}

void HyperTrackEngine::drawPbrMesh(const GpuMesh& mesh, const Mat4& model,
                                   const MaterialParams& mat, GLuint albedoTex, GLuint normalTex,
                                   bool useTex) {
  float normalMat[9];
  computeNormalMatrix(model, normalMat);

  glUniformMatrix4fv(pbrProg_.loc("uModel"), 1, GL_FALSE, model.m);
  glUniformMatrix3fv(pbrProg_.loc("uNormalMatrix"), 1, GL_FALSE, normalMat);

  glUniform3f(pbrProg_.loc("uBaseColor"), mat.baseColor.x, mat.baseColor.y, mat.baseColor.z);
  glUniform1f(pbrProg_.loc("uMetallic"), mat.metallic);
  glUniform1f(pbrProg_.loc("uRoughness"), mat.roughness);
  glUniform1f(pbrProg_.loc("uClearcoat"), mat.clearcoat);
  glUniform1f(pbrProg_.loc("uClearcoatRoughness"), mat.clearcoatRoughness);
  glUniform3f(pbrProg_.loc("uEmissive"), mat.emissive.x, mat.emissive.y, mat.emissive.z);
  glUniform1i(pbrProg_.loc("uSurfaceType"), mat.surfaceType);
  glUniform1i(pbrProg_.loc("uUseTextures"), useTex ? 1 : 0);

  if (useTex) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, albedoTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTex);
  }

  mesh.draw();
}

void HyperTrackEngine::renderScenePass(const Mat4& view, const Mat4& proj, const Mat4& lightSpace,
                                       const Mat4& carModel,
                                       const std::array<Mat4, 4>& wheelModels,
                                       const std::array<Mat4, 4>& caliperModels) {
  glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo_);
  glViewport(0, 0, width_, height_);
  glClearColor(0.02f, 0.03f, 0.05f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);

  LightingEnv env = getLightingEnv(timeOfDay_, weather_);
  const auto& st = physics_.state();
  Vec3 carFwd{std::sin(st.heading), 0.0f, std::cos(st.heading)};
  Vec3 carRight = carFwd.cross(Vec3{0.0f, 1.0f, 0.0f}).normalized();

  Mat4 viewProj = proj * view;

  // 1. Render PBR World & Car
  glUseProgram(pbrProg_.id);
  glUniformMatrix4fv(pbrProg_.loc("uViewProj"), 1, GL_FALSE, viewProj.m);
  glUniformMatrix4fv(pbrProg_.loc("uLightSpaceMatrix"), 1, GL_FALSE, lightSpace.m);

  glUniform1i(pbrProg_.loc("uAlbedoMap"), 0);
  glUniform1i(pbrProg_.loc("uNormalMap"), 1);
  glUniform1i(pbrProg_.loc("uShadowMap"), 2);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, shadowTex_);

  glUniform3f(pbrProg_.loc("uCamPos"), camPos_.x, camPos_.y, camPos_.z);
  glUniform3f(pbrProg_.loc("uSunDir"), env.sunDir.x, env.sunDir.y, env.sunDir.z);
  glUniform3f(pbrProg_.loc("uSunColor"), env.sunColor.x, env.sunColor.y, env.sunColor.z);
  glUniform3f(pbrProg_.loc("uSkyZenith"), env.skyZenith.x, env.skyZenith.y, env.skyZenith.z);
  glUniform3f(pbrProg_.loc("uSkyHorizon"), env.skyHorizon.x, env.skyHorizon.y, env.skyHorizon.z);
  glUniform1f(pbrProg_.loc("uNightFactor"), env.nightFactor);

  float wetness = (weather_ == WeatherMode::DRY) ? 0.0f
                  : (weather_ == WeatherMode::WET_PUDDLES ? 0.72f : 1.0f);
  float rainIntensity = (weather_ == WeatherMode::RAIN_SHOWER) ? 1.0f : 0.0f;
  glUniform1f(pbrProg_.loc("uWetness"), wetness);
  glUniform1f(pbrProg_.loc("uRainIntensity"), rainIntensity);
  glUniform1f(pbrProg_.loc("uTime"), totalTime_);
  glUniform1i(pbrProg_.loc("uReflectionsEnabled"), graphics_.reflectionsEnabled ? 1 : 0);
  glUniform1i(pbrProg_.loc("uShadowsEnabled"), graphics_.shadowsEnabled ? 1 : 0);

  glUniform3f(pbrProg_.loc("uCarPos"), st.position.x, st.position.y, st.position.z);
  glUniform3f(pbrProg_.loc("uCarForward"), carFwd.x, carFwd.y, carFwd.z);
  glUniform3f(pbrProg_.loc("uCarRight"), carRight.x, carRight.y, carRight.z);
  glUniform1f(pbrProg_.loc("uBrakeGlow"), st.brakeGlow);

  Mat4 ident = Mat4::identity();
  glDisable(GL_CULL_FACE);

  // Terrain
  MaterialParams terrainMat;
  terrainMat.baseColor = (activeTrack_ == TrackId::ALPINE_PASS) ? Vec3{0.42f, 0.50f, 0.36f}
                                                                : Vec3{0.34f, 0.44f, 0.28f};
  terrainMat.roughness = 0.88f;
  terrainMat.surfaceType = 4;
  drawPbrMesh(track_.terrainMesh(), ident, terrainMat, textures_.terrainAlbedo,
              textures_.terrainNormal, true);

  // Asphalt road
  MaterialParams roadMat;
  roadMat.baseColor = {0.72f, 0.72f, 0.74f};
  roadMat.roughness = 0.62f;
  roadMat.surfaceType = 1;
  drawPbrMesh(track_.asphaltMesh(), ident, roadMat, textures_.asphaltAlbedo,
              textures_.asphaltNormal, true);

  // Kerbs
  MaterialParams kerbMat;
  kerbMat.baseColor = {0.95f, 0.95f, 0.95f};
  kerbMat.roughness = 0.55f;
  kerbMat.surfaceType = 2;
  drawPbrMesh(track_.kerbMesh(), ident, kerbMat, textures_.kerbAlbedo, textures_.kerbNormal, true);

  // Guardrails & Structures
  MaterialParams barMat;
  barMat.baseColor = {0.58f, 0.60f, 0.64f};
  barMat.metallic = 0.75f;
  barMat.roughness = 0.38f;
  drawPbrMesh(track_.barrierMesh(), ident, barMat, 0, 0, false);

  MaterialParams structMat;
  structMat.baseColor = {0.26f, 0.28f, 0.32f};
  structMat.metallic = 0.45f;
  structMat.roughness = 0.45f;
  drawPbrMesh(track_.structureMesh(), ident, structMat, 0, 0, false);

  // Emissive barrier strips & floodlight bulbs
  MaterialParams emMat;
  emMat.baseColor = {0.9f, 0.9f, 0.95f};
  emMat.roughness = 0.2f;
  emMat.emissive = env.barrierEmissive;
  drawPbrMesh(track_.emissiveMesh(), ident, emMat, 0, 0, false);

  // Hypercar Body
  MaterialParams carPaint = getLiveryMaterial(livery_);
  drawPbrMesh(carMesh_.bodyShell(), carModel, carPaint, 0, 0, false);

  // Glass Canopy
  MaterialParams glassMat;
  glassMat.baseColor = {0.03f, 0.04f, 0.06f};
  glassMat.metallic = 0.2f;
  glassMat.roughness = 0.04f;
  glassMat.clearcoat = 1.0f;
  glassMat.clearcoatRoughness = 0.02f;
  glassMat.surfaceType = 6;
  drawPbrMesh(carMesh_.glassCanopy(), carModel, glassMat, 0, 0, false);

  // Carbon Aero Kit
  MaterialParams carbonMat;
  carbonMat.baseColor = {0.72f, 0.74f, 0.78f};
  carbonMat.metallic = 0.25f;
  carbonMat.roughness = 0.26f;
  carbonMat.clearcoat = 0.8f;
  carbonMat.surfaceType = 3;
  drawPbrMesh(carMesh_.carbonAero(), carModel, carbonMat, textures_.carbonAlbedo,
              textures_.carbonNormal, true);

  // Headlights
  MaterialParams hlMat;
  hlMat.baseColor = {0.95f, 0.98f, 1.0f};
  hlMat.emissive = Vec3{2.4f, 2.6f, 3.0f};
  drawPbrMesh(carMesh_.headlights(), carModel, hlMat, 0, 0, false);

  // Tail Lights (glow brighter when braking!)
  MaterialParams tlMat;
  tlMat.baseColor = {0.6f, 0.04f, 0.06f};
  float brakeMult = 1.1f + st.brakeInput * 2.4f + st.brakeGlow * 1.2f;
  tlMat.emissive = Vec3{2.2f * brakeMult, 0.12f * brakeMult, 0.16f * brakeMult};
  drawPbrMesh(carMesh_.taillights(), carModel, tlMat, 0, 0, false);

  // Exhaust tips
  MaterialParams exMat;
  exMat.baseColor = {0.55f, 0.56f, 0.62f};
  exMat.metallic = 0.92f;
  exMat.roughness = 0.22f;
  if (st.nitroActive) exMat.emissive = Vec3{0.2f, 1.4f, 2.2f};
  drawPbrMesh(carMesh_.exhaust(), carModel, exMat, 0, 0, false);

  // 4 Wheels, Forged Rims, Brake Rotors & Calipers
  MaterialParams tireMat;
  tireMat.baseColor = {0.06f, 0.06f, 0.07f};
  tireMat.roughness = 0.68f - wetness * 0.25f;

  MaterialParams rimMat;
  rimMat.baseColor = (livery_ == CarLivery::STEALTH_CARBON) ? Vec3{0.18f, 0.19f, 0.21f}
                                                            : Vec3{0.78f, 0.80f, 0.84f};
  rimMat.metallic = 0.95f;
  rimMat.roughness = 0.14f;

  MaterialParams discMat;
  discMat.baseColor = {0.52f, 0.53f, 0.56f};
  discMat.metallic = 0.88f;
  discMat.roughness = 0.26f;
  discMat.emissive = Vec3{2.1f, 0.48f, 0.08f} * st.brakeGlow;

  MaterialParams calMat;
  calMat.baseColor = (livery_ == CarLivery::APEX_CYAN) ? Vec3{0.05f, 0.78f, 1.0f}
                                                       : Vec3{0.92f, 0.14f, 0.12f};
  calMat.metallic = 0.45f;
  calMat.roughness = 0.22f;
  calMat.clearcoat = 0.8f;

  for (int w = 0; w < 4; ++w) {
    drawPbrMesh(carMesh_.wheelTire(), wheelModels[w], tireMat, 0, 0, false);
    drawPbrMesh(carMesh_.wheelRim(), wheelModels[w], rimMat, 0, 0, false);
    drawPbrMesh(carMesh_.brakeDisc(), wheelModels[w], discMat, 0, 0, false);
    drawPbrMesh(carMesh_.brakeCaliper(), caliperModels[w], calMat, 0, 0, false);
  }

  // 2. Render Atmospheric Sky Dome last (with GL_LEQUAL depth func)
  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_FALSE);
  glUseProgram(skyProg_.id);

  Mat4 viewNoTrans = view;
  viewNoTrans.m[12] = viewNoTrans.m[13] = viewNoTrans.m[14] = 0.0f;
  Mat4 vpNoTrans = proj * viewNoTrans;
  glUniformMatrix4fv(skyProg_.loc("uViewProjNoTranslate"), 1, GL_FALSE, vpNoTrans.m);
  glUniform3f(skyProg_.loc("uSunDir"), env.sunDir.x, env.sunDir.y, env.sunDir.z);
  glUniform3f(skyProg_.loc("uSunColor"), env.sunColor.x, env.sunColor.y, env.sunColor.z);
  glUniform3f(skyProg_.loc("uSkyZenith"), env.skyZenith.x, env.skyZenith.y, env.skyZenith.z);
  glUniform3f(skyProg_.loc("uSkyHorizon"), env.skyHorizon.x, env.skyHorizon.y, env.skyHorizon.z);
  glUniform1f(skyProg_.loc("uNightFactor"), env.nightFactor);
  glUniform1f(skyProg_.loc("uCloudDensity"), env.cloudDensity);
  glUniform1f(skyProg_.loc("uTime"), totalTime_);

  glBindVertexArray(skyVao_);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);

  glDepthFunc(GL_LESS);

  // 3. Render Rain, Water Spray, Tire Smoke & Nitro Flames
  renderParticlesPass(view, viewProj);

  glDepthMask(GL_TRUE);
}

void HyperTrackEngine::renderParticlesPass(const Mat4& view, const Mat4& viewProj) {
  Vec3 camRight{view.m[0], view.m[4], view.m[8]};
  Vec3 camUp{view.m[1], view.m[5], view.m[9]};

  int activeCount = 0;
  for (const auto& p : particles_) {
    if (!p.active) continue;
    float t = p.life / std::max(p.maxLife, 0.001f);
    float alpha = p.color.w * std::sin(t * kPi);
    if (alpha < 0.01f) continue;

    Vec3 rightVec = camRight * p.size;
    Vec3 upVec = (p.stretch > 2.0f) ? (p.vel.normalized() * (-p.size * p.stretch))
                                    : (camUp * (p.size * p.stretch));

    Vec3 v0 = p.pos - rightVec - upVec;
    Vec3 v1 = p.pos + rightVec - upVec;
    Vec3 v2 = p.pos + rightVec + upVec;
    Vec3 v3 = p.pos - rightVec + upVec;

    int base = activeCount * 4 * 9;
    float* dst = particleVertData_.data() + base;
    auto writeV = [&](int idx, const Vec3& pos, float u, float v) {
      float* ptr = dst + idx * 9;
      ptr[0] = pos.x; ptr[1] = pos.y; ptr[2] = pos.z;
      ptr[3] = u; ptr[4] = v;
      ptr[5] = p.color.x; ptr[6] = p.color.y; ptr[7] = p.color.z; ptr[8] = alpha;
    };
    writeV(0, v0, 0.0f, 0.0f);
    writeV(1, v1, 1.0f, 0.0f);
    writeV(2, v2, 1.0f, 1.0f);
    writeV(3, v3, 0.0f, 1.0f);

    activeCount++;
    if (activeCount >= 400) break;
  }

  if (activeCount == 0) return;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);

  glUseProgram(particleProg_.id);
  glUniformMatrix4fv(particleProg_.loc("uViewProj"), 1, GL_FALSE, viewProj.m);

  glBindVertexArray(particleVao_);
  glBindBuffer(GL_ARRAY_BUFFER, particleVbo_);
  glBufferSubData(GL_ARRAY_BUFFER, 0, activeCount * 4 * 9 * sizeof(float), particleVertData_.data());
  glDrawElements(GL_TRIANGLES, activeCount * 6, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);

  glDisable(GL_BLEND);
}

void HyperTrackEngine::renderPostProcess() {
  glDisable(GL_DEPTH_TEST);

  if (graphics_.bloomEnabled) {
    // 1. Bright-pass into bloomFbo_[0]
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo_[0]);
    glViewport(0, 0, bloomW_, bloomH_);
    glUseProgram(brightProg_.id);
    glUniform1i(brightProg_.loc("uScene"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex_);
    glBindVertexArray(quadVao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 2. Separable Gaussian Blur (2 iterations)
    glUseProgram(blurProg_.id);
    glUniform1i(blurProg_.loc("uTex"), 0);
    for (int i = 0; i < 2; ++i) {
      glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo_[1]);
      glUniform2f(blurProg_.loc("uDirection"), 1.35f / static_cast<float>(bloomW_), 0.0f);
      glBindTexture(GL_TEXTURE_2D, bloomTex_[0]);
      glDrawArrays(GL_TRIANGLES, 0, 6);

      glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo_[0]);
      glUniform2f(blurProg_.loc("uDirection"), 0.0f, 1.35f / static_cast<float>(bloomH_));
      glBindTexture(GL_TEXTURE_2D, bloomTex_[1]);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }
  }

  // 3. Final ACES Filmic Composite to Screen
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, width_, height_);
  glUseProgram(compositeProg_.id);
  glUniform1i(compositeProg_.loc("uScene"), 0);
  glUniform1i(compositeProg_.loc("uBloom"), 1);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sceneColorTex_);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, bloomTex_[0]);

  float speedFactor = clampf((physics_.state().speedKmh - 140.0f) / 170.0f, 0.0f, 1.0f);
  if (physics_.state().nitroActive) speedFactor = std::max(speedFactor, 0.75f);

  glUniform1f(compositeProg_.loc("uExposure"), graphics_.exposure);
  glUniform1f(compositeProg_.loc("uSpeedFactor"), speedFactor);
  glUniform1i(compositeProg_.loc("uBloomEnabled"), graphics_.bloomEnabled ? 1 : 0);

  glBindVertexArray(quadVao_);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}

void HyperTrackEngine::getTelemetry(float* out16) {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto& st = physics_.state();
  out16[0] = st.speedKmh;
  out16[1] = st.rpm;
  out16[2] = static_cast<float>(st.gear);
  out16[3] = st.currentLapTime;
  out16[4] = st.bestLapTime;
  out16[5] = static_cast<float>(st.lapCount);
  out16[6] = st.trackProgress;
  out16[7] = st.driftAngleDeg;
  out16[8] = st.lateralG;
  out16[9] = st.longitudinalG;
  out16[10] = st.nitroAmount;
  out16[11] = st.nitroActive ? 1.0f : 0.0f;
  out16[12] = st.onTrack ? 1.0f : 0.0f;
  out16[13] = fps_;
  out16[14] = track_.totalLength();
  out16[15] = st.brakeGlow;
}

int HyperTrackEngine::getMinimapPoints(float* outXY, int maxPoints) {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto& pts = track_.minimapPoints();
  int count = std::min(static_cast<int>(pts.size()), maxPoints);
  for (int i = 0; i < count; ++i) {
    outXY[i * 2 + 0] = pts[i].x;
    outXY[i * 2 + 1] = pts[i].y;
  }
  return count;
}

}  // namespace hypertrack
