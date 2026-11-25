#include "track.hpp"
#include <cmath>

namespace hypertrack {

void GpuMesh::upload(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices) {
  destroy();
  if (verts.empty() || indices.empty()) return;

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(),
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void*>(offsetof(Vertex, px)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void*>(offsetof(Vertex, nx)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void*>(offsetof(Vertex, u)));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void*>(offsetof(Vertex, tx)));

  glBindVertexArray(0);
  indexCount = static_cast<GLsizei>(indices.size());
}

void GpuMesh::draw() const {
  if (vao == 0 || indexCount == 0) return;
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}

void GpuMesh::destroy() {
  if (ebo) glDeleteBuffers(1, &ebo);
  if (vbo) glDeleteBuffers(1, &vbo);
  if (vao) glDeleteVertexArrays(1, &vao);
  vao = vbo = ebo = 0;
  indexCount = 0;
}

namespace {

void appendBox(std::vector<Vertex>& verts, std::vector<uint32_t>& indices, const Vec3& center,
               const Vec3& right, const Vec3& up, const Vec3& forward, const Vec3& halfExtents) {
  Vec3 r = right * halfExtents.x;
  Vec3 u = up * halfExtents.y;
  Vec3 f = forward * halfExtents.z;

  struct Face {
    Vec3 n;
    Vec3 p0, p1, p2, p3;
  };
  Face faces[6] = {
      {forward, center - r - u + f, center + r - u + f, center + r + u + f, center - r + u + f},
      {-forward, center + r - u - f, center - r - u - f, center - r + u - f, center + r + u - f},
      {right, center + r - u + f, center + r - u - f, center + r + u - f, center + r + u + f},
      {-right, center - r - u - f, center - r - u + f, center - r + u + f, center - r + u - f},
      {up, center - r + u + f, center + r + u + f, center + r + u - f, center - r + u - f},
      {-up, center - r - u - f, center + r - u - f, center + r - u + f, center - r - u + f},
  };

  for (const auto& face : faces) {
    uint32_t base = static_cast<uint32_t>(verts.size());
    Vec3 tang = (face.p1 - face.p0).normalized();
    verts.push_back({face.p0.x, face.p0.y, face.p0.z, face.n.x, face.n.y, face.n.z, 0.0f, 0.0f,
                     tang.x, tang.y, tang.z});
    verts.push_back({face.p1.x, face.p1.y, face.p1.z, face.n.x, face.n.y, face.n.z, 1.0f, 0.0f,
                     tang.x, tang.y, tang.z});
    verts.push_back({face.p2.x, face.p2.y, face.p2.z, face.n.x, face.n.y, face.n.z, 1.0f, 1.0f,
                     tang.x, tang.y, tang.z});
    verts.push_back({face.p3.x, face.p3.y, face.p3.z, face.n.x, face.n.y, face.n.z, 0.0f, 1.0f,
                     tang.x, tang.y, tang.z});
    indices.push_back(base + 0);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
    indices.push_back(base + 0);
    indices.push_back(base + 2);
    indices.push_back(base + 3);
  }
}

Vec3 rotateAroundAxis(const Vec3& v, const Vec3& axis, float angleRad) {
  float c = std::cos(angleRad);
  float s = std::sin(angleRad);
  return v * c + axis.cross(v) * s + axis * (axis.dot(v) * (1.0f - c));
}

}  // namespace

void Track::build(TrackId id) {
  destroy();
  id_ = id;

  std::vector<ControlPoint> cps;
  if (id == TrackId::APEX_GP) {
    cps = {
        {{0.0f, 0.0f, 0.0f}, 9.5f, 0.0f},
        {{0.0f, 0.0f, -150.0f}, 9.5f, 0.0f},
        {{28.0f, 3.0f, -270.0f}, 9.5f, -5.0f},
        {{115.0f, 8.0f, -315.0f}, 9.0f, -8.0f},
        {{205.0f, 11.0f, -260.0f}, 9.0f, -4.0f},
        {{215.0f, 7.0f, -145.0f}, 9.0f, 6.0f},
        {{155.0f, 4.0f, -75.0f}, 9.0f, 4.0f},
        {{240.0f, 5.5f, 15.0f}, 9.5f, -5.0f},
        {{265.0f, 3.0f, 130.0f}, 9.5f, -7.0f},
        {{175.0f, 1.0f, 210.0f}, 9.5f, -4.0f},
        {{75.0f, 0.5f, 165.0f}, 9.0f, 3.0f},
        {{15.0f, 0.0f, 115.0f}, 9.5f, -3.0f},
    };
  } else if (id == TrackId::MONACO_NIGHT) {
    cps = {
        {{0.0f, 0.0f, 0.0f}, 8.2f, 0.0f},
        {{0.0f, 0.5f, -145.0f}, 8.2f, 0.0f},
        {{55.0f, 4.5f, -225.0f}, 8.0f, -4.0f},
        {{145.0f, 9.5f, -210.0f}, 8.0f, -5.0f},
        {{195.0f, 12.0f, -135.0f}, 7.8f, -6.0f},
        {{145.0f, 7.0f, -65.0f}, 7.8f, 4.0f},
        {{220.0f, 3.5f, -5.0f}, 8.2f, -4.0f},
        {{235.0f, 1.0f, 95.0f}, 8.5f, -5.0f},
        {{155.0f, 0.5f, 165.0f}, 8.5f, -3.0f},
        {{75.0f, 0.2f, 125.0f}, 8.2f, 3.0f},
        {{20.0f, 0.0f, 130.0f}, 8.2f, -3.0f},
    };
  } else {
    // ALPINE_PASS
    cps = {
        {{0.0f, 0.0f, 0.0f}, 8.5f, 0.0f},
        {{0.0f, 4.0f, -140.0f}, 8.5f, 0.0f},
        {{-65.0f, 14.0f, -245.0f}, 8.2f, 6.0f},
        {{15.0f, 24.0f, -340.0f}, 8.2f, -7.0f},
        {{145.0f, 28.0f, -310.0f}, 8.5f, -6.0f},
        {{225.0f, 20.0f, -210.0f}, 8.2f, -5.0f},
        {{165.0f, 13.0f, -110.0f}, 8.0f, 5.0f},
        {{250.0f, 8.0f, -5.0f}, 8.5f, -6.0f},
        {{220.0f, 2.0f, 125.0f}, 8.8f, -5.0f},
        {{105.0f, -1.0f, 195.0f}, 8.8f, -4.0f},
        {{15.0f, -0.5f, 135.0f}, 8.5f, -3.0f},
    };
  }

  generateSpline(cps, 360);
  buildRoadAndProps();
  buildTerrain();
}

void Track::destroy() {
  asphalt_.destroy();
  kerb_.destroy();
  barrier_.destroy();
  emissive_.destroy();
  structure_.destroy();
  terrain_.destroy();
  samples_.clear();
  minimapPoints_.clear();
}

void Track::generateSpline(const std::vector<ControlPoint>& cps, int totalSamples) {
  samples_.clear();
  samples_.resize(totalSamples);
  int n = static_cast<int>(cps.size());

  for (int i = 0; i < totalSamples; ++i) {
    float globalT = static_cast<float>(i) / static_cast<float>(totalSamples) * static_cast<float>(n);
    int seg = static_cast<int>(std::floor(globalT)) % n;
    float localT = globalT - std::floor(globalT);

    const auto& p0 = cps[(seg - 1 + n) % n];
    const auto& p1 = cps[seg];
    const auto& p2 = cps[(seg + 1) % n];
    const auto& p3 = cps[(seg + 2) % n];

    samples_[i].pos = catmullRom(p0.pos, p1.pos, p2.pos, p3.pos, localT);
    samples_[i].halfWidth = lerpf(p1.halfWidth, p2.halfWidth, localT);
    samples_[i].bankAngle = lerpf(p1.bankDeg, p2.bankDeg, localT) * (kPi / 180.0f);
  }

  float accumDist = 0.0f;
  float minX = 1e9f, maxX = -1e9f, minZ = 1e9f, maxZ = -1e9f;

  for (int i = 0; i < totalSamples; ++i) {
    int prev = (i - 1 + totalSamples) % totalSamples;
    int next = (i + 1) % totalSamples;

    Vec3 fwd = (samples_[next].pos - samples_[prev].pos).normalized();
    Vec3 rightFlat = fwd.cross(Vec3{0.0f, 1.0f, 0.0f}).normalized();
    Vec3 right = rotateAroundAxis(rightFlat, fwd, samples_[i].bankAngle).normalized();
    Vec3 up = right.cross(fwd).normalized();

    samples_[i].forward = fwd;
    samples_[i].right = right;
    samples_[i].up = up;

    if (i > 0) {
      accumDist += (samples_[i].pos - samples_[i - 1].pos).length();
    }
    samples_[i].distAlong = accumDist;

    Vec3 fPrev = (samples_[i].pos - samples_[prev].pos).normalized();
    Vec3 fNext = (samples_[next].pos - samples_[i].pos).normalized();
    samples_[i].curvature = (fNext - fPrev).length();

    minX = std::min(minX, samples_[i].pos.x);
    maxX = std::max(maxX, samples_[i].pos.x);
    minZ = std::min(minZ, samples_[i].pos.z);
    maxZ = std::max(maxZ, samples_[i].pos.z);
  }

  totalLength_ = accumDist + (samples_[0].pos - samples_.back().pos).length();

  // Compute normalized 2D minimap points preserving aspect ratio
  float spanX = std::max(maxX - minX, 1.0f);
  float spanZ = std::max(maxZ - minZ, 1.0f);
  float maxSpan = std::max(spanX, spanZ);
  float padX = (maxSpan - spanX) * 0.5f;
  float padZ = (maxSpan - spanZ) * 0.5f;

  minimapPoints_.clear();
  for (int i = 0; i < totalSamples; i += 3) {
    float nx = (samples_[i].pos.x - minX + padX) / maxSpan;
    float nz = (samples_[i].pos.z - minZ + padZ) / maxSpan;
    minimapPoints_.push_back({nx, nz});
  }
}

void Track::buildRoadAndProps() {
  int n = static_cast<int>(samples_.size());

  std::vector<Vertex> roadV, kerbV, barV, emV, structV;
  std::vector<uint32_t> roadI, kerbI, barI, emI, structI;

  const float kerbWidth = 1.15f;
  const float barrierOffset = 2.6f;
  const float barrierHeight = 1.05f;

  for (int i = 0; i <= n; ++i) {
    const auto& s = samples_[i % n];
    float dist = (i == n) ? totalLength_ : s.distAlong;
    float vCoord = dist * 0.14f;

    // 1. Road asphalt vertices (left & right edge)
    Vec3 lPos = s.pos - s.right * s.halfWidth;
    Vec3 rPos = s.pos + s.right * s.halfWidth;
    roadV.push_back({lPos.x, lPos.y, lPos.z, s.up.x, s.up.y, s.up.z, 0.0f, vCoord, s.right.x,
                     s.right.y, s.right.z});
    roadV.push_back({rPos.x, rPos.y, rPos.z, s.up.x, s.up.y, s.up.z, 1.0f, vCoord, s.right.x,
                     s.right.y, s.right.z});

    // 2. Left and Right FIA Kerbs
    Vec3 lkIn = lPos + s.up * 0.03f;
    Vec3 lkOut = lPos - s.right * kerbWidth + s.up * 0.06f;
    Vec3 rkIn = rPos + s.up * 0.03f;
    Vec3 rkOut = rPos + s.right * kerbWidth + s.up * 0.06f;
    float kV = dist * 0.35f;

    kerbV.push_back({lkOut.x, lkOut.y, lkOut.z, s.up.x, s.up.y, s.up.z, 0.0f, kV, s.right.x,
                     s.right.y, s.right.z});
    kerbV.push_back({lkIn.x, lkIn.y, lkIn.z, s.up.x, s.up.y, s.up.z, 1.0f, kV, s.right.x,
                     s.right.y, s.right.z});
    kerbV.push_back({rkIn.x, rkIn.y, rkIn.z, s.up.x, s.up.y, s.up.z, 0.0f, kV, s.right.x,
                     s.right.y, s.right.z});
    kerbV.push_back({rkOut.x, rkOut.y, rkOut.z, s.up.x, s.up.y, s.up.z, 1.0f, kV, s.right.x,
                     s.right.y, s.right.z});

    // 3. Left & Right Guardrail Barriers
    Vec3 lbBot = s.pos - s.right * (s.halfWidth + barrierOffset) - s.up * 0.1f;
    Vec3 lbTop = lbBot + s.up * barrierHeight;
    Vec3 rbBot = s.pos + s.right * (s.halfWidth + barrierOffset) - s.up * 0.1f;
    Vec3 rbTop = rbBot + s.up * barrierHeight;

    barV.push_back({lbBot.x, lbBot.y, lbBot.z, s.right.x, s.right.y, s.right.z, 0.0f, vCoord,
                    s.forward.x, s.forward.y, s.forward.z});
    barV.push_back({lbTop.x, lbTop.y, lbTop.z, s.right.x, s.right.y, s.right.z, 1.0f, vCoord,
                    s.forward.x, s.forward.y, s.forward.z});
    barV.push_back({rbBot.x, rbBot.y, rbBot.z, -s.right.x, -s.right.y, -s.right.z, 0.0f, vCoord,
                    s.forward.x, s.forward.y, s.forward.z});
    barV.push_back({rbTop.x, rbTop.y, rbTop.z, -s.right.x, -s.right.y, -s.right.z, 1.0f, vCoord,
                    s.forward.x, s.forward.y, s.forward.z});

    // 4. Emissive Safety Strip along barriers (high contrast on night/wet tracks)
    Vec3 lStripBot = lbBot + s.up * (barrierHeight * 0.65f) + s.right * 0.05f;
    Vec3 lStripTop = lStripBot + s.up * 0.12f;
    Vec3 rStripBot = rbBot + s.up * (barrierHeight * 0.65f) - s.right * 0.05f;
    Vec3 rStripTop = rStripBot + s.up * 0.12f;

    emV.push_back({lStripBot.x, lStripBot.y, lStripBot.z, s.right.x, s.right.y, s.right.z, 0.0f,
                   vCoord, s.forward.x, s.forward.y, s.forward.z});
    emV.push_back({lStripTop.x, lStripTop.y, lStripTop.z, s.right.x, s.right.y, s.right.z, 1.0f,
                   vCoord, s.forward.x, s.forward.y, s.forward.z});
    emV.push_back({rStripBot.x, rStripBot.y, rStripBot.z, -s.right.x, -s.right.y, -s.right.z, 0.0f,
                   vCoord, s.forward.x, s.forward.y, s.forward.z});
    emV.push_back({rStripTop.x, rStripTop.y, rStripTop.z, -s.right.x, -s.right.y, -s.right.z, 1.0f,
                   vCoord, s.forward.x, s.forward.y, s.forward.z});
  }

  for (int i = 0; i < n; ++i) {
    uint32_t r0 = static_cast<uint32_t>(i * 2);
    roadI.push_back(r0 + 0);
    roadI.push_back(r0 + 1);
    roadI.push_back(r0 + 3);
    roadI.push_back(r0 + 0);
    roadI.push_back(r0 + 3);
    roadI.push_back(r0 + 2);

    uint32_t k0 = static_cast<uint32_t>(i * 4);
    kerbI.push_back(k0 + 0);
    kerbI.push_back(k0 + 1);
    kerbI.push_back(k0 + 5);
    kerbI.push_back(k0 + 0);
    kerbI.push_back(k0 + 5);
    kerbI.push_back(k0 + 4);

    kerbI.push_back(k0 + 2);
    kerbI.push_back(k0 + 3);
    kerbI.push_back(k0 + 7);
    kerbI.push_back(k0 + 2);
    kerbI.push_back(k0 + 7);
    kerbI.push_back(k0 + 6);

    uint32_t b0 = static_cast<uint32_t>(i * 4);
    barI.push_back(b0 + 0);
    barI.push_back(b0 + 4);
    barI.push_back(b0 + 5);
    barI.push_back(b0 + 0);
    barI.push_back(b0 + 5);
    barI.push_back(b0 + 1);

    barI.push_back(b0 + 2);
    barI.push_back(b0 + 3);
    barI.push_back(b0 + 7);
    barI.push_back(b0 + 2);
    barI.push_back(b0 + 7);
    barI.push_back(b0 + 6);

    emI.push_back(b0 + 0);
    emI.push_back(b0 + 4);
    emI.push_back(b0 + 5);
    emI.push_back(b0 + 0);
    emI.push_back(b0 + 5);
    emI.push_back(b0 + 1);

    emI.push_back(b0 + 2);
    emI.push_back(b0 + 3);
    emI.push_back(b0 + 7);
    emI.push_back(b0 + 2);
    emI.push_back(b0 + 7);
    emI.push_back(b0 + 6);
  }

  // 5. Start/Finish Gantry Bridge at sample 0
  {
    const auto& s0 = samples_[0];
    float span = s0.halfWidth + barrierOffset + 0.8f;
    appendBox(structV, structI, s0.pos - s0.right * span + s0.up * 3.8f, s0.right, s0.up,
              s0.forward, {0.45f, 3.8f, 0.65f});
    appendBox(structV, structI, s0.pos + s0.right * span + s0.up * 3.8f, s0.right, s0.up,
              s0.forward, {0.45f, 3.8f, 0.65f});
    appendBox(structV, structI, s0.pos + s0.up * 7.2f, s0.right, s0.up, s0.forward,
              {span + 0.6f, 0.75f, 1.1f});
    // Glowing start gantry light bar
    appendBox(emV, emI, s0.pos + s0.up * 6.42f, s0.right, s0.up, s0.forward,
              {s0.halfWidth * 0.9f, 0.16f, 0.4f});
  }

  // 6. Trackside Floodlight Towers & Grandstand / City / Alpine Structures
  for (int i = 0; i < n; i += 15) {
    const auto& s = samples_[i];
    float side = ((i / 15) % 2 == 0) ? 1.0f : -1.0f;
    Vec3 poleBase = s.pos + s.right * (side * (s.halfWidth + barrierOffset + 1.8f));
    appendBox(structV, structI, poleBase + s.up * 4.5f, s.right, s.up, s.forward,
              {0.18f, 4.5f, 0.18f});
    Vec3 armCenter = poleBase + s.up * 8.9f - s.right * (side * 1.4f);
    appendBox(structV, structI, armCenter, s.right, s.up, s.forward, {1.5f, 0.14f, 0.18f});
    appendBox(emV, emI, armCenter - s.up * 0.18f - s.right * (side * 0.9f), s.right, s.up,
              s.forward, {0.55f, 0.12f, 0.35f});
  }

  // Grandstands near main straight
  for (int g = 0; g < 3; ++g) {
    int idx = (12 + g * 12) % n;
    const auto& s = samples_[idx];
    Vec3 standBase = s.pos + s.right * (s.halfWidth + 11.0f);
    for (int tier = 0; tier < 3; ++tier) {
      appendBox(structV, structI,
                standBase + s.right * (tier * 2.2f) + s.up * (1.2f + tier * 1.4f), s.right, s.up,
                s.forward, {1.4f, 0.9f, 10.0f});
    }
  }

  asphalt_.upload(roadV, roadI);
  kerb_.upload(kerbV, kerbI);
  barrier_.upload(barV, barI);
  emissive_.upload(emV, emI);
  structure_.upload(structV, structI);
}

void Track::buildTerrain() {
  const int grid = 64;
  const float minCoord = -420.0f;
  const float maxCoord = 520.0f;
  const float step = (maxCoord - minCoord) / static_cast<float>(grid);

  std::vector<Vertex> verts((grid + 1) * (grid + 1));
  std::vector<uint32_t> indices;
  indices.reserve(grid * grid * 6);

  int n = static_cast<int>(samples_.size());

  for (int z = 0; z <= grid; ++z) {
    for (int x = 0; x <= grid; ++x) {
      float wx = minCoord + x * step;
      float wz = minCoord + z * step;

      // Find nearest track sample to blend terrain height seamlessly below track shoulders
      float bestDistSq = 1e12f;
      float trackY = 0.0f;
      float trackHW = 9.0f;
      for (int i = 0; i < n; i += 3) {
        float dx = wx - samples_[i].pos.x;
        float dz = wz - samples_[i].pos.z;
        float d2 = dx * dx + dz * dz;
        if (d2 < bestDistSq) {
          bestDistSq = d2;
          trackY = samples_[i].pos.y;
          trackHW = samples_[i].halfWidth;
        }
      }
      float dist = std::sqrt(bestDistSq);
      float hillAmp = (id_ == TrackId::ALPINE_PASS) ? 38.0f : (id_ == TrackId::MONACO_NIGHT ? 14.0f : 12.0f);
      float hills = (fbm2D(wx * 0.006f + 4.2f, wz * 0.006f + 1.9f, 4) - 0.35f) * hillAmp;
      float blend = smoothstepf(trackHW + 5.0f, trackHW + 65.0f, dist);
      float wy = (trackY - 0.25f) + hills * blend;

      int idx = z * (grid + 1) + x;
      verts[idx] = {wx, wy, wz, 0.0f, 1.0f, 0.0f, wx * 0.03f, wz * 0.03f, 1.0f, 0.0f, 0.0f};
    }
  }

  // Recompute smooth terrain normals
  for (int z = 1; z < grid; ++z) {
    for (int x = 1; x < grid; ++x) {
      int idx = z * (grid + 1) + x;
      float hL = verts[idx - 1].py;
      float hR = verts[idx + 1].py;
      float hD = verts[idx - (grid + 1)].py;
      float hU = verts[idx + (grid + 1)].py;
      Vec3 nrm = Vec3{hL - hR, 2.0f * step, hD - hU}.normalized();
      verts[idx].nx = nrm.x;
      verts[idx].ny = nrm.y;
      verts[idx].nz = nrm.z;
    }
  }

  for (int z = 0; z < grid; ++z) {
    for (int x = 0; x < grid; ++x) {
      uint32_t i0 = static_cast<uint32_t>(z * (grid + 1) + x);
      uint32_t i1 = i0 + 1;
      uint32_t i2 = i0 + (grid + 1);
      uint32_t i3 = i2 + 1;
      indices.push_back(i0);
      indices.push_back(i2);
      indices.push_back(i1);
      indices.push_back(i1);
      indices.push_back(i2);
      indices.push_back(i3);
    }
  }

  terrain_.upload(verts, indices);
}

TrackSample Track::sampleAtProgress(float t01) const {
  if (samples_.empty()) return {};
  float wrapped = t01 - std::floor(t01);
  float fIdx = wrapped * static_cast<float>(samples_.size());
  int i0 = static_cast<int>(std::floor(fIdx)) % static_cast<int>(samples_.size());
  int i1 = (i0 + 1) % static_cast<int>(samples_.size());
  float alpha = fIdx - std::floor(fIdx);

  TrackSample s;
  s.pos = lerpVec3(samples_[i0].pos, samples_[i1].pos, alpha);
  s.forward = lerpVec3(samples_[i0].forward, samples_[i1].forward, alpha).normalized();
  s.right = lerpVec3(samples_[i0].right, samples_[i1].right, alpha).normalized();
  s.up = s.right.cross(s.forward).normalized();
  s.halfWidth = lerpf(samples_[i0].halfWidth, samples_[i1].halfWidth, alpha);
  s.bankAngle = lerpf(samples_[i0].bankAngle, samples_[i1].bankAngle, alpha);
  s.distAlong = lerpf(samples_[i0].distAlong, samples_[i1].distAlong, alpha);
  s.curvature = lerpf(samples_[i0].curvature, samples_[i1].curvature, alpha);
  return s;
}

TrackSample Track::closestSample(const Vec3& worldPos, int& outNearestIdx) const {
  int n = static_cast<int>(samples_.size());
  if (n == 0) {
    outNearestIdx = 0;
    return {};
  }
  float bestD2 = 1e14f;
  int bestIdx = 0;
  for (int i = 0; i < n; ++i) {
    Vec3 d = worldPos - samples_[i].pos;
    float d2 = d.x * d.x + d.z * d.z + d.y * d.y * 0.25f;
    if (d2 < bestD2) {
      bestD2 = d2;
      bestIdx = i;
    }
  }
  outNearestIdx = bestIdx;

  // Interpolate between nearest neighbor segment
  int next = (bestIdx + 1) % n;
  Vec3 seg = samples_[next].pos - samples_[bestIdx].pos;
  float segLen2 = std::max(seg.lengthSq(), 1e-4f);
  float proj = clampf((worldPos - samples_[bestIdx].pos).dot(seg) / segLen2, 0.0f, 1.0f);

  TrackSample s;
  s.pos = lerpVec3(samples_[bestIdx].pos, samples_[next].pos, proj);
  s.forward = lerpVec3(samples_[bestIdx].forward, samples_[next].forward, proj).normalized();
  s.right = lerpVec3(samples_[bestIdx].right, samples_[next].right, proj).normalized();
  s.up = s.right.cross(s.forward).normalized();
  s.halfWidth = lerpf(samples_[bestIdx].halfWidth, samples_[next].halfWidth, proj);
  s.bankAngle = lerpf(samples_[bestIdx].bankAngle, samples_[next].bankAngle, proj);
  s.distAlong = lerpf(samples_[bestIdx].distAlong, samples_[next].distAlong, proj);
  s.curvature = lerpf(samples_[bestIdx].curvature, samples_[next].curvature, proj);
  return s;
}

}  // namespace hypertrack
