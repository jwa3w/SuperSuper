#pragma once

#include "engine_types.hpp"
#include "track.hpp"

namespace hypertrack {

class CarPhysics {
 public:
  void resetToTrack(const Track& track);
  void step(float dt, const Track& track, const DriverInput& userInput, WeatherMode weather,
            bool showcaseAutoDrive, bool showcaseParked);

  const CarPhysicsState& state() const { return state_; }

 private:
  CarPhysicsState state_;
  int lastNearestIdx_ = 0;
  float prevProgress_ = 0.0f;
  float prevForwardSpeed_ = 0.0f;
  float prevLateralSpeed_ = 0.0f;

  DriverInput computeShowcasePilot(const Track& track, bool parked) const;
};

}  // namespace hypertrack
