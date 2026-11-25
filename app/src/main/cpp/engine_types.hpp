#pragma once

#include "math3d.hpp"
#include <cstdint>

namespace hypertrack {

enum class TrackId : int {
  APEX_GP = 0,
  MONACO_NIGHT = 1,
  ALPINE_PASS = 2,
};

enum class CarLivery : int {
  ROSSO_CORSA = 0,
  STEALTH_CARBON = 1,
  LIQUID_SILVER = 2,
  APEX_CYAN = 3,
  MIDNIGHT_EMERALD = 4,
};

enum class WeatherMode : int {
  DRY = 0,
  WET_PUDDLES = 1,
  RAIN_SHOWER = 2,
};

enum class TimeOfDay : int {
  GOLDEN_SUNSET = 0,
  MIDNIGHT_NEON = 1,
  ALPINE_OVERCAST = 2,
  HIGH_NOON = 3,
};

enum class CameraMode : int {
  CHASE = 0,
  HOOD = 1,
  BUMPER = 2,
  SHOWCASE_ORBIT = 3,
  SHOWCASE_DRONE = 4,
  SHOWCASE_CURB = 5,
};

struct Vertex {
  float px, py, pz;
  float nx, ny, nz;
  float u, v;
  float tx, ty, tz;
};

struct MaterialParams {
  Vec3 baseColor{0.8f, 0.1f, 0.1f};
  float metallic = 0.0f;
  float roughness = 0.4f;
  float clearcoat = 0.0f;
  float clearcoatRoughness = 0.08f;
  Vec3 emissive{0.0f, 0.0f, 0.0f};
  int surfaceType = 0;  // 0=generic, 1=asphalt, 2=kerb, 3=carbon, 4=terrain, 5=carpaint, 6=glass, 7=brake
};

struct DriverInput {
  float throttle = 0.0f;
  float brake = 0.0f;
  float steer = 0.0f;      // -1 left, +1 right
  bool handbrake = false;
  bool nitro = false;
};

struct GraphicsSettings {
  bool bloomEnabled = true;
  bool reflectionsEnabled = true;
  bool shadowsEnabled = true;
  bool rainParticlesEnabled = true;
  float exposure = 1.18f;
};

struct CarPhysicsState {
  Vec3 position{0.0f, 0.0f, 0.0f};
  Vec3 velocity{0.0f, 0.0f, 0.0f};
  Vec3 surfaceNormal{0.0f, 1.0f, 0.0f};
  float heading = 0.0f;       // yaw in radians
  float pitch = 0.0f;         // suspension + road slope pitch
  float roll = 0.0f;          // suspension body roll
  float roadPitch = 0.0f;
  float roadBank = 0.0f;
  float steerAngle = 0.0f;    // front wheel angle radians
  float wheelSpin = 0.0f;     // radians
  float speedMs = 0.0f;
  float speedKmh = 0.0f;
  float rpm = 1100.0f;
  int gear = 1;
  float driftAngleDeg = 0.0f;
  float lateralG = 0.0f;
  float longitudinalG = 0.0f;
  float nitroAmount = 1.0f;   // 0..1
  bool nitroActive = false;
  float brakeGlow = 0.0f;     // 0..1
  float brakeInput = 0.0f;
  float throttleInput = 0.0f;
  float trackProgress = 0.0f; // 0..1
  int lapCount = 1;
  float currentLapTime = 0.0f;
  float bestLapTime = 0.0f;
  bool onTrack = true;
  float backfireTimer = 0.0f;
};

}  // namespace hypertrack
