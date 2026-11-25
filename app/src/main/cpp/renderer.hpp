#pragma once

#include "car_mesh.hpp"
#include "engine_types.hpp"
#include "physics.hpp"
#include "shaders.hpp"
#include "textures.hpp"
#include "track.hpp"
#include <GLES3/gl3.h>
#include <array>
#include <mutex>
#include <vector>

namespace hypertrack {

struct Particle {
  Vec3 pos;
  Vec3 vel;
  Vec4 color;
  float size = 0.2f;
  float stretch = 1.0f;
  float life = 0.0f;
  float maxLife = 1.0f;
  bool active = false;
};

class HyperTrackEngine {
 public:
  void initGL();
  void resize(int width, int height);
  void renderFrame();
  void destroyGL();

  void setDriverInput(const DriverInput& input);
  void addOrbitTouch(float dx, float dy, float pinchScale);
  void setTrack(TrackId id);
  void setLivery(CarLivery livery);
  void setWeather(WeatherMode weather);
  void setTimeOfDay(TimeOfDay tod);
  void setCameraMode(CameraMode cam);
  void setShowcaseCruise(bool cruising);
  void setGraphicsSettings(const GraphicsSettings& settings);
  void resetCar();

  // Telemetry snapshot for Kotlin HUD (16 floats)
  void getTelemetry(float* out16);
  // Returns number of 2D points written (x, y pairs)
  int getMinimapPoints(float* outXY, int maxPoints);

 private:
  std::mutex mutex_;

  int width_ = 1280;
  int height_ = 720;
  bool glReady_ = false;
  double lastTimeSec_ = 0.0;
  float totalTime_ = 0.0f;
  float fps_ = 60.0f;
  float fpsAccum_ = 0.0f;
  int fpsFrames_ = 0;

  // World & Simulation
  TrackId pendingTrack_ = TrackId::APEX_GP;
  TrackId activeTrack_ = TrackId::APEX_GP;
  CarLivery livery_ = CarLivery::ROSSO_CORSA;
  WeatherMode weather_ = WeatherMode::WET_PUDDLES;
  TimeOfDay timeOfDay_ = TimeOfDay::GOLDEN_SUNSET;
  CameraMode cameraMode_ = CameraMode::SHOWCASE_ORBIT;
  bool showcaseCruising_ = true;

  DriverInput driverInput_;
  GraphicsSettings graphics_;

  Track track_;
  CarMesh carMesh_;
  CarPhysics physics_;
  TextureSet textures_;

  // Orbit & Smooth Camera State
  float orbitYaw_ = 0.65f;
  float orbitPitch_ = 0.24f;
  float orbitDist_ = 5.4f;
  float orbitIdleTime_ = 0.0f;
  Vec3 camPos_{0.0f, 3.0f, 8.0f};
  Vec3 camTarget_{0.0f, 0.5f, 0.0f};
  float camFovDeg_ = 60.0f;
  int curbAnchorIdx_ = 30;

  // Shaders
  ShaderProgram skyProg_;
  ShaderProgram shadowProg_;
  ShaderProgram pbrProg_;
  ShaderProgram particleProg_;
  ShaderProgram brightProg_;
  ShaderProgram blurProg_;
  ShaderProgram compositeProg_;

  // Skybox & Quad VAOs
  GLuint skyVao_ = 0, skyVbo_ = 0;
  GLuint quadVao_ = 0, quadVbo_ = 0;

  // FBOs
  GLuint shadowFbo_ = 0, shadowTex_ = 0;
  GLuint sceneFbo_ = 0, sceneColorTex_ = 0, sceneDepthRbo_ = 0;
  GLuint bloomFbo_[2] = {0, 0};
  GLuint bloomTex_[2] = {0, 0};
  int bloomW_ = 256, bloomH_ = 144;

  // Dynamic Particles
  std::vector<Particle> particles_;
  GLuint particleVao_ = 0, particleVbo_ = 0, particleEbo_ = 0;
  std::vector<float> particleVertData_;

  void setupSkyAndQuad();
  void setupFramebuffers();
  void destroyFramebuffers();
  void updateCamera(float dt);
  void updateParticles(float dt);
  void spawnParticle(const Vec3& pos, const Vec3& vel, const Vec4& color, float size,
                     float stretch, float maxLife);

  void renderShadowPass(const Mat4& lightSpace, const Mat4& carModel,
                        const std::array<Mat4, 4>& wheelModels);
  void renderScenePass(const Mat4& view, const Mat4& proj, const Mat4& lightSpace,
                       const Mat4& carModel, const std::array<Mat4, 4>& wheelModels,
                       const std::array<Mat4, 4>& caliperModels);
  void renderParticlesPass(const Mat4& view, const Mat4& viewProj);
  void renderPostProcess();

  void drawPbrMesh(const GpuMesh& mesh, const Mat4& model, const MaterialParams& mat,
                   GLuint albedoTex, GLuint normalTex, bool useTex);
};

}  // namespace hypertrack
