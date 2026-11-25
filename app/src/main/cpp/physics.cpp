#include "physics.hpp"
#include <cmath>

namespace hypertrack {

namespace {

float wrapAngle(float a) {
  while (a > kPi) a -= kTwoPi;
  while (a < -kPi) a += kTwoPi;
  return a;
}

}  // namespace

void CarPhysics::resetToTrack(const Track& track) {
  const auto& samples = track.samples();
  if (samples.empty()) return;
  const auto& s0 = samples[0];

  state_ = CarPhysicsState{};
  state_.position = s0.pos;
  state_.surfaceNormal = s0.up;
  state_.heading = std::atan2(s0.forward.x, s0.forward.z);
  state_.gear = 1;
  state_.rpm = 1200.0f;
  state_.nitroAmount = 1.0f;
  state_.lapCount = 1;
  lastNearestIdx_ = 0;
  prevProgress_ = 0.0f;
  prevForwardSpeed_ = 0.0f;
  prevLateralSpeed_ = 0.0f;
}

DriverInput CarPhysics::computeShowcasePilot(const Track& track, bool parked) const {
  DriverInput cmd;
  if (parked) {
    cmd.brake = state_.speedMs > 0.5f ? 0.85f : 0.0f;
    cmd.handbrake = state_.speedMs <= 0.5f;
    return cmd;
  }

  const auto& samples = track.samples();
  if (samples.empty()) return cmd;
  int n = static_cast<int>(samples.size());

  int lookAhead = clampf(static_cast<int>(state_.speedMs * 0.35f) + 6, 6, 22);
  int targetIdx = (lastNearestIdx_ + lookAhead) % n;
  const auto& targetSample = samples[targetIdx];

  Vec3 toTarget = targetSample.pos - state_.position;
  float targetHeading = std::atan2(toTarget.x, toTarget.z);
  float err = wrapAngle(targetHeading - state_.heading);

  // err > 0 means target is to our left (since right turns decrease heading)
  cmd.steer = clampf(-err * 2.1f, -1.0f, 1.0f);

  // Modulate speed for upcoming corner curvature
  float maxCurve = 0.0f;
  for (int k = 2; k <= 16; k += 2) {
    maxCurve = std::max(maxCurve, samples[(lastNearestIdx_ + k) % n].curvature);
  }
  float targetSpeed = clampf(62.0f - maxCurve * 310.0f, 22.0f, 64.0f);

  if (state_.speedMs < targetSpeed) {
    cmd.throttle = clampf((targetSpeed - state_.speedMs) * 0.14f, 0.25f, 0.95f);
    cmd.nitro = (maxCurve < 0.03f && state_.speedMs > 32.0f && state_.nitroAmount > 0.45f);
  } else if (state_.speedMs > targetSpeed + 4.5f) {
    cmd.brake = clampf((state_.speedMs - targetSpeed) * 0.15f, 0.15f, 0.85f);
  } else {
    cmd.throttle = 0.2f;
  }

  return cmd;
}

void CarPhysics::step(float dt, const Track& track, const DriverInput& userInput,
                      WeatherMode weather, bool showcaseAutoDrive, bool showcaseParked) {
  if (track.samples().empty()) return;

  DriverInput input = showcaseAutoDrive ? computeShowcasePilot(track, showcaseParked) : userInput;

  // Weather grip multiplier
  float weatherGrip = 1.0f;
  if (weather == WeatherMode::WET_PUDDLES) weatherGrip = 0.86f;
  if (weather == WeatherMode::RAIN_SHOWER) weatherGrip = 0.74f;

  Vec3 fwdFlat{std::sin(state_.heading), 0.0f, std::cos(state_.heading)};
  float vForward = state_.velocity.dot(fwdFlat);

  // Nitro boost management
  bool usingNitro = input.nitro && state_.nitroAmount > 0.03f && input.throttle > 0.1f;
  state_.nitroActive = usingNitro;
  if (usingNitro) {
    state_.nitroAmount = clampf(state_.nitroAmount - dt * 0.24f, 0.0f, 1.0f);
  } else {
    state_.nitroAmount = clampf(state_.nitroAmount + dt * 0.08f, 0.0f, 1.0f);
  }

  // Smooth steering response with speed-sensitive high-speed authority
  float targetSteerAngle = input.steer * (0.42f / (1.0f + std::abs(vForward) * 0.022f));
  state_.steerAngle = lerpf(state_.steerAngle, targetSteerAngle, clampf(dt * 11.0f, 0.0f, 1.0f));

  // Engine force & braking / reverse
  float maxEngineAccel = usingNitro ? 27.5f : 18.5f;
  float accel = 0.0f;

  if (input.throttle > 0.01f) {
    float speedFactor = 1.0f - clampf(vForward / (usingNitro ? 92.0f : 76.0f), 0.0f, 0.92f);
    accel += input.throttle * maxEngineAccel * speedFactor * (0.85f + 0.15f * weatherGrip);
  }

  if (input.brake > 0.01f) {
    if (vForward > 1.2f) {
      accel -= input.brake * 32.0f * weatherGrip;
    } else {
      // Smooth reverse gear
      accel -= input.brake * 10.5f;
      if (vForward < -18.0f) accel = 0.0f;
    }
  }

  if (input.handbrake) {
    accel -= (vForward > 0.0f ? 1.0f : -1.0f) * 10.0f;
  }

  // Aerodynamic drag + rolling resistance
  float drag = 0.0032f * vForward * std::abs(vForward) + 0.18f * vForward;
  float offTrackDrag = state_.onTrack ? 0.0f : 1.85f * vForward;
  vForward += (accel - drag - offTrackDrag) * dt;

  // Yaw / steering angular integration
  float speedSign = (vForward >= -0.2f) ? 1.0f : -1.0f;
  float turnEfficiency = clampf(std::abs(vForward) / 4.5f, 0.0f, 1.0f);
  float driftTurnBoost = input.handbrake ? 1.38f : 1.0f;
  float yawRate = -state_.steerAngle * 3.15f * turnEfficiency * speedSign * driftTurnBoost;
  state_.heading = wrapAngle(state_.heading + yawRate * dt);

  // Recompute basis after yaw update & apply lateral tire grip (allows controllable drift!)
  Vec3 newFwd{std::sin(state_.heading), 0.0f, std::cos(state_.heading)};
  Vec3 newRight = newFwd.cross(Vec3{0.0f, 1.0f, 0.0f}).normalized();

  // Centrifugal velocity transfer + lateral tire friction
  float latGripRate = (input.handbrake ? 1.7f : 6.8f) * weatherGrip * (state_.onTrack ? 1.0f : 0.65f);
  // Blend previous world velocity into new basis
  Vec3 worldVelFlat{state_.velocity.x, 0.0f, state_.velocity.z};
  float projFwd = worldVelFlat.dot(newFwd);
  float projLat = worldVelFlat.dot(newRight);

  // Engine longitudinal delta applied on top of projected forward speed
  projFwd += (vForward - worldVelFlat.dot(fwdFlat));
  projLat *= std::exp(-latGripRate * dt);

  state_.velocity = newFwd * projFwd + newRight * projLat;
  state_.position += state_.velocity * dt;

  // Snap / follow track surface height, banking & barrier collision
  TrackSample nearest = track.closestSample(state_.position, lastNearestIdx_);
  Vec3 rel = state_.position - nearest.pos;
  float lateralOffset = rel.dot(nearest.right);
  float roadLimit = nearest.halfWidth + 0.35f;
  float wallLimit = nearest.halfWidth + 1.55f;

  state_.onTrack = std::abs(lateralOffset) <= roadLimit;

  if (std::abs(lateralOffset) > wallLimit) {
    float sign = (lateralOffset > 0.0f) ? 1.0f : -1.0f;
    state_.position = nearest.pos + nearest.right * (sign * wallLimit);
    // Reflect velocity softly off guardrail
    float vn = state_.velocity.dot(nearest.right) * sign;
    if (vn > 0.0f) {
      state_.velocity -= nearest.right * (sign * vn * 1.25f);
      state_.velocity *= 0.86f;
    }
    lateralOffset = sign * wallLimit;
  }

  // Smoothly align car height & road surface normal
  float targetY = nearest.pos.y + std::tan(nearest.bankAngle) * lateralOffset;
  // Re-evaluate exact point on banked road plane
  Vec3 roadPoint = nearest.pos + nearest.right * lateralOffset;
  targetY = roadPoint.y;
  state_.position.y = lerpf(state_.position.y, targetY, clampf(dt * 18.0f, 0.0f, 1.0f));
  state_.surfaceNormal = lerpVec3(state_.surfaceNormal, nearest.up, clampf(dt * 14.0f, 0.0f, 1.0f)).normalized();

  // Compute longitudinal & lateral G-forces for suspension squat/dive/roll
  float lonAcc = (projFwd - prevForwardSpeed_) / std::max(dt, 0.001f);
  float latAcc = (projLat - prevLateralSpeed_) / std::max(dt, 0.001f) + yawRate * projFwd;
  prevForwardSpeed_ = projFwd;
  prevLateralSpeed_ = projLat;

  state_.longitudinalG = lerpf(state_.longitudinalG, lonAcc / 9.81f, clampf(dt * 10.0f, 0.0f, 1.0f));
  state_.lateralG = lerpf(state_.lateralG, latAcc / 9.81f, clampf(dt * 10.0f, 0.0f, 1.0f));

  float targetPitch = clampf(-state_.longitudinalG * 0.028f, -0.045f, 0.045f);
  float targetRoll = clampf(state_.lateralG * 0.042f, -0.065f, 0.065f);
  state_.pitch = lerpf(state_.pitch, targetPitch, clampf(dt * 10.0f, 0.0f, 1.0f));
  state_.roll = lerpf(state_.roll, targetRoll, clampf(dt * 10.0f, 0.0f, 1.0f));

  state_.speedMs = state_.velocity.length();
  state_.speedKmh = state_.speedMs * 3.6f;
  state_.wheelSpin += (projFwd / 0.36f) * dt;
  state_.brakeInput = input.brake;
  state_.throttleInput = input.throttle;

  // Brake disc thermal glow
  float heatTarget = (input.brake > 0.15f && state_.speedMs > 8.0f) ? input.brake : 0.0f;
  if (heatTarget > state_.brakeGlow) {
    state_.brakeGlow = lerpf(state_.brakeGlow, heatTarget, clampf(dt * 4.5f, 0.0f, 1.0f));
  } else {
    state_.brakeGlow = lerpf(state_.brakeGlow, 0.0f, clampf(dt * 1.4f, 0.0f, 1.0f));
  }

  // Drift angle in degrees
  if (state_.speedMs > 3.0f) {
    state_.driftAngleDeg = std::atan2(projLat, std::abs(projFwd)) * (180.0f / kPi);
  } else {
    state_.driftAngleDeg = 0.0f;
  }

  // 6-speed sequential gearbox & RPM simulation
  static constexpr float kGearTopSpeeds[7] = {0.0f, 18.0f, 30.0f, 44.0f, 58.0f, 72.0f, 95.0f};
  float absFwd = std::abs(projFwd);
  int newGear = 1;
  if (projFwd < -1.0f) {
    newGear = 0;  // Reverse
  } else {
    for (int g = 1; g <= 6; ++g) {
      if (absFwd <= kGearTopSpeeds[g] || g == 6) {
        newGear = g;
        break;
      }
    }
  }
  if (newGear != state_.gear && newGear > state_.gear) {
    state_.backfireTimer = 0.14f;  // Upshift exhaust flame pop!
  }
  state_.gear = newGear;
  state_.backfireTimer = std::max(0.0f, state_.backfireTimer - dt);

  float gLow = (state_.gear <= 1) ? 0.0f : kGearTopSpeeds[state_.gear - 1] * 0.72f;
  float gHigh = (state_.gear == 0) ? 18.0f : kGearTopSpeeds[state_.gear];
  float gearFrac = clampf((absFwd - gLow) / std::max(gHigh - gLow, 1.0f), 0.0f, 1.0f);
  float targetRpm = 1400.0f + gearFrac * 7100.0f + (input.throttle * 450.0f);
  state_.rpm = lerpf(state_.rpm, clampf(targetRpm, 1100.0f, 8900.0f), clampf(dt * 12.0f, 0.0f, 1.0f));

  // Lap & progress tracking
  float prog = static_cast<float>(lastNearestIdx_) / static_cast<float>(track.samples().size());
  if (!showcaseParked) {
    state_.currentLapTime += dt;
  }
  if (prevProgress_ > 0.85f && prog < 0.15f && state_.currentLapTime > 12.0f) {
    if (state_.bestLapTime <= 0.0f || state_.currentLapTime < state_.bestLapTime) {
      state_.bestLapTime = state_.currentLapTime;
    }
    state_.lapCount++;
    state_.currentLapTime = 0.0f;
  }
  prevProgress_ = prog;
  state_.trackProgress = prog;
}

}  // namespace hypertrack
