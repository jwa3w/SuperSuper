#pragma once

#include "engine_types.hpp"
#include "math3d.hpp"
#include <GLES3/gl3.h>
#include <vector>

namespace hypertrack {

struct TrackSample {
  Vec3 pos;
  Vec3 forward;
  Vec3 right;
  Vec3 up;
  float halfWidth = 8.5f;
  float bankAngle = 0.0f;
  float distAlong = 0.0f;
  float curvature = 0.0f;
};

struct GpuMesh {
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint ebo = 0;
  GLsizei indexCount = 0;

  void upload(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices);
  void draw() const;
  void destroy();
};

class Track {
 public:
  void build(TrackId id);
  void destroy();

  TrackId currentId() const { return id_; }
  float totalLength() const { return totalLength_; }
  const std::vector<TrackSample>& samples() const { return samples_; }

  TrackSample sampleAtProgress(float t01) const;
  TrackSample closestSample(const Vec3& worldPos, int& outNearestIdx) const;

  const GpuMesh& asphaltMesh() const { return asphalt_; }
  const GpuMesh& kerbMesh() const { return kerb_; }
  const GpuMesh& barrierMesh() const { return barrier_; }
  const GpuMesh& emissiveMesh() const { return emissive_; }
  const GpuMesh& structureMesh() const { return structure_; }
  const GpuMesh& terrainMesh() const { return terrain_; }

  // Normalized 2D minimap points in [0,1]
  const std::vector<Vec2>& minimapPoints() const { return minimapPoints_; }

 private:
  struct ControlPoint {
    Vec3 pos;
    float halfWidth;
    float bankDeg;
  };

  TrackId id_ = TrackId::APEX_GP;
  float totalLength_ = 1.0f;
  std::vector<TrackSample> samples_;
  std::vector<Vec2> minimapPoints_;

  GpuMesh asphalt_;
  GpuMesh kerb_;
  GpuMesh barrier_;
  GpuMesh emissive_;
  GpuMesh structure_;
  GpuMesh terrain_;

  void generateSpline(const std::vector<ControlPoint>& cps, int totalSamples);
  void buildRoadAndProps();
  void buildTerrain();
};

}  // namespace hypertrack
