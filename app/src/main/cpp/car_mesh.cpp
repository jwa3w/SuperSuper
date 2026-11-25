#include "car_mesh.hpp"
#include <cmath>
#include <vector>

namespace hypertrack {

namespace {

void recomputeSmoothNormals(std::vector<Vertex>& verts, const std::vector<uint32_t>& indices) {
  for (auto& v : verts) {
    v.nx = v.ny = v.nz = 0.0f;
  }
  for (size_t i = 0; i + 2 < indices.size(); i += 3) {
    Vertex& a = verts[indices[i + 0]];
    Vertex& b = verts[indices[i + 1]];
    Vertex& c = verts[indices[i + 2]];
    Vec3 ab{b.px - a.px, b.py - a.py, b.pz - a.pz};
    Vec3 ac{c.px - a.px, c.py - a.py, c.pz - a.pz};
    Vec3 fn = ab.cross(ac);
    a.nx += fn.x; a.ny += fn.y; a.nz += fn.z;
    b.nx += fn.x; b.ny += fn.y; b.nz += fn.z;
    c.nx += fn.x; c.ny += fn.y; c.nz += fn.z;
  }
  for (auto& v : verts) {
    Vec3 n = Vec3{v.nx, v.ny, v.nz}.normalized();
    v.nx = n.x; v.ny = n.y; v.nz = n.z;
    Vec3 t = Vec3{1.0f, 0.0f, 0.0f};
    if (std::abs(n.x) > 0.9f) t = Vec3{0.0f, 0.0f, 1.0f};
    t = (t - n * n.dot(t)).normalized();
    v.tx = t.x; v.ty = t.y; v.tz = t.z;
  }
}

void addBox(std::vector<Vertex>& verts, std::vector<uint32_t>& indices, const Vec3& center,
            const Vec3& halfExtents, float pitchRad = 0.0f) {
  Mat4 rot = Mat4::rotateX(pitchRad);
  auto xform = [&](Vec3 p) {
    float y = p.y * rot.m[5] + p.z * rot.m[9];
    float z = p.y * rot.m[6] + p.z * rot.m[10];
    return Vec3{p.x + center.x, y + center.y, z + center.z};
  };
  auto xformN = [&](Vec3 n) {
    float y = n.y * rot.m[5] + n.z * rot.m[9];
    float z = n.y * rot.m[6] + n.z * rot.m[10];
    return Vec3{n.x, y, z}.normalized();
  };

  float hx = halfExtents.x, hy = halfExtents.y, hz = halfExtents.z;
  struct Face {
    Vec3 n;
    Vec3 p[4];
  };
  Face faces[6] = {
      {{0, 0, 1}, {{-hx, -hy, hz}, {hx, -hy, hz}, {hx, hy, hz}, {-hx, hy, hz}}},
      {{0, 0, -1}, {{hx, -hy, -hz}, {-hx, -hy, -hz}, {-hx, hy, -hz}, {hx, hy, -hz}}},
      {{1, 0, 0}, {{hx, -hy, hz}, {hx, -hy, -hz}, {hx, hy, -hz}, {hx, hy, hz}}},
      {{-1, 0, 0}, {{-hx, -hy, -hz}, {-hx, -hy, hz}, {-hx, hy, hz}, {-hx, hy, -hz}}},
      {{0, 1, 0}, {{-hx, hy, hz}, {hx, hy, hz}, {hx, hy, -hz}, {-hx, hy, -hz}}},
      {{0, -1, 0}, {{-hx, -hy, -hz}, {hx, -hy, -hz}, {hx, -hy, hz}, {-hx, -hy, hz}}},
  };
  for (const auto& f : faces) {
    uint32_t base = static_cast<uint32_t>(verts.size());
    Vec3 n = xformN(f.n);
    Vec3 tang = (xform(f.p[1]) - xform(f.p[0])).normalized();
    for (int i = 0; i < 4; ++i) {
      Vec3 wp = xform(f.p[i]);
      float u = (i == 1 || i == 2) ? 1.0f : 0.0f;
      float v = (i >= 2) ? 1.0f : 0.0f;
      verts.push_back({wp.x, wp.y, wp.z, n.x, n.y, n.z, u, v, tang.x, tang.y, tang.z});
    }
    indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
    indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
  }
}

// Sculpted lofted mesh helper: each station along Z has a half-profile from bottom-center -> outer -> top-center
struct BodyStation {
  float z;         // -2.25 (nose) to +2.25 (tail)
  float halfW;     // half width at beltline
  float lowY;      // bottom sill height
  float beltY;     // fender/beltline height
  float deckY;     // hood/roof/rear deck center height
  float shoulderW; // width at upper crease
};

void buildLoftedShell(const std::vector<BodyStation>& stations, std::vector<Vertex>& verts,
                      std::vector<uint32_t>& indices) {
  // 11 profile points around cross section (CCW when looking from +Z toward -Z):
  // 0: bottom center, 1: bottom right, 2: lower fender right, 3: belt right, 4: shoulder right,
  // 5: top center, 6: shoulder left, 7: belt left, 8: lower fender left, 9: bottom left, 10: bottom center
  const int ringPts = 11;
  int nS = static_cast<int>(stations.size());

  for (int i = 0; i < nS; ++i) {
    const auto& st = stations[i];
    float vCoord = static_cast<float>(i) / static_cast<float>(nS - 1);
    Vec3 pts[ringPts] = {
        {0.0f, st.lowY, st.z},
        {st.halfW * 0.88f, st.lowY + 0.02f, st.z},
        {st.halfW * 1.02f, lerpf(st.lowY, st.beltY, 0.45f), st.z},
        {st.halfW, st.beltY, st.z},
        {st.shoulderW, lerpf(st.beltY, st.deckY, 0.65f), st.z},
        {0.0f, st.deckY, st.z},
        {-st.shoulderW, lerpf(st.beltY, st.deckY, 0.65f), st.z},
        {-st.halfW, st.beltY, st.z},
        {-st.halfW * 1.02f, lerpf(st.lowY, st.beltY, 0.45f), st.z},
        {-st.halfW * 0.88f, st.lowY + 0.02f, st.z},
        {0.0f, st.lowY, st.z},
    };
    for (int j = 0; j < ringPts; ++j) {
      float uCoord = static_cast<float>(j) / static_cast<float>(ringPts - 1);
      verts.push_back({pts[j].x, pts[j].y, pts[j].z, 0, 1, 0, uCoord, vCoord, 1, 0, 0});
    }
  }

  for (int i = 0; i < nS - 1; ++i) {
    for (int j = 0; j < ringPts - 1; ++j) {
      uint32_t a = static_cast<uint32_t>(i * ringPts + j);
      uint32_t b = a + 1;
      uint32_t c = a + ringPts;
      uint32_t d = c + 1;
      indices.push_back(a); indices.push_back(b); indices.push_back(d);
      indices.push_back(a); indices.push_back(d); indices.push_back(c);
    }
  }

  // Cap front & rear ends
  uint32_t frontCenter = static_cast<uint32_t>(verts.size());
  const auto& f0 = stations.front();
  verts.push_back({0.0f, (f0.lowY + f0.beltY) * 0.5f, f0.z, 0, 0, -1, 0.5f, 0.5f, 1, 0, 0});
  for (int j = 0; j < ringPts - 1; ++j) {
    indices.push_back(frontCenter);
    indices.push_back(static_cast<uint32_t>(j + 1));
    indices.push_back(static_cast<uint32_t>(j));
  }

  uint32_t backCenter = static_cast<uint32_t>(verts.size());
  const auto& b0 = stations.back();
  verts.push_back({0.0f, (b0.lowY + b0.beltY) * 0.5f, b0.z, 0, 0, 1, 0.5f, 0.5f, 1, 0, 0});
  uint32_t lastRing = static_cast<uint32_t>((nS - 1) * ringPts);
  for (int j = 0; j < ringPts - 1; ++j) {
    indices.push_back(backCenter);
    indices.push_back(lastRing + j);
    indices.push_back(lastRing + j + 1);
  }

  recomputeSmoothNormals(verts, indices);
}

void buildCylinderX(std::vector<Vertex>& verts, std::vector<uint32_t>& indices, float rOuter,
                    float rInner, float halfWidth, int segs, bool capEnds) {
  uint32_t start = static_cast<uint32_t>(verts.size());
  for (int i = 0; i <= segs; ++i) {
    float a = static_cast<float>(i) / static_cast<float>(segs) * kTwoPi;
    float cy = std::cos(a);
    float sz = std::sin(a);
    // Outer tread
    verts.push_back({-halfWidth, cy * rOuter, sz * rOuter, 0, cy, sz, 0, static_cast<float>(i) / segs, 1, 0, 0});
    verts.push_back({halfWidth, cy * rOuter, sz * rOuter, 0, cy, sz, 1, static_cast<float>(i) / segs, 1, 0, 0});
    // Inner barrel
    verts.push_back({-halfWidth, cy * rInner, sz * rInner, 0, -cy, -sz, 0, static_cast<float>(i) / segs, 1, 0, 0});
    verts.push_back({halfWidth, cy * rInner, sz * rInner, 0, -cy, -sz, 1, static_cast<float>(i) / segs, 1, 0, 0});
  }
  for (int i = 0; i < segs; ++i) {
    uint32_t b = start + static_cast<uint32_t>(i * 4);
    // Outer tread
    indices.push_back(b + 0); indices.push_back(b + 4); indices.push_back(b + 5);
    indices.push_back(b + 0); indices.push_back(b + 5); indices.push_back(b + 1);
    // Inner barrel
    indices.push_back(b + 2); indices.push_back(b + 3); indices.push_back(b + 7);
    indices.push_back(b + 2); indices.push_back(b + 7); indices.push_back(b + 6);
    if (capEnds) {
      // Left sidewall (-X)
      indices.push_back(b + 0); indices.push_back(b + 2); indices.push_back(b + 6);
      indices.push_back(b + 0); indices.push_back(b + 6); indices.push_back(b + 4);
      // Right sidewall (+X)
      indices.push_back(b + 1); indices.push_back(b + 5); indices.push_back(b + 7);
      indices.push_back(b + 1); indices.push_back(b + 7); indices.push_back(b + 3);
    }
  }
  recomputeSmoothNormals(verts, indices);
}

}  // namespace

void CarMesh::build() {
  destroy();

  // 1. Sculpted Hypercar Body Shell (nose at -Z, tail at +Z)
  {
    std::vector<BodyStation> body = {
        {-2.26f, 0.78f, 0.14f, 0.34f, 0.36f, 0.56f},  // Front nose lip
        {-2.00f, 0.94f, 0.13f, 0.56f, 0.50f, 0.68f},  // Headlight brow
        {-1.55f, 1.00f, 0.13f, 0.72f, 0.64f, 0.72f},  // Front wheel arch peak
        {-1.02f, 0.96f, 0.13f, 0.70f, 0.73f, 0.68f},  // A-pillar cowl
        {-0.45f, 0.93f, 0.14f, 0.68f, 0.76f, 0.66f},  // Door waist
        {0.25f, 0.95f, 0.14f, 0.72f, 0.79f, 0.68f},   // Side intake scoop
        {0.92f, 1.03f, 0.15f, 0.84f, 0.85f, 0.76f},   // Muscular rear haunch
        {1.55f, 1.02f, 0.16f, 0.86f, 0.86f, 0.76f},   // Rear wheel arch peak
        {2.05f, 0.94f, 0.21f, 0.80f, 0.82f, 0.72f},   // Tail taper
        {2.22f, 0.82f, 0.28f, 0.74f, 0.76f, 0.66f},   // Rear fascia
    };
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    buildLoftedShell(body, v, idx);
    bodyShell_.upload(v, idx);
  }

  // 2. Teardrop Glass Canopy & Roofline
  {
    std::vector<BodyStation> canopy = {
        {-1.08f, 0.68f, 0.68f, 0.72f, 0.74f, 0.54f},  // Windshield base
        {-0.52f, 0.64f, 0.72f, 0.98f, 1.12f, 0.52f},  // Windshield top
        {0.08f, 0.62f, 0.74f, 1.04f, 1.19f, 0.50f},   // Roof apex
        {0.72f, 0.60f, 0.76f, 0.98f, 1.10f, 0.46f},   // Fastback slope
        {1.45f, 0.52f, 0.78f, 0.85f, 0.88f, 0.36f},   // Engine louver glass end
    };
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    buildLoftedShell(canopy, v, idx);
    glassCanopy_.upload(v, idx);
  }

  // 3. Carbon Fiber Aero Kit (Splitter, Side Skirts, Diffuser, Swan-Neck GT Wing)
  {
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    // Front aero splitter
    addBox(v, idx, {0.0f, 0.11f, -2.14f}, {0.96f, 0.025f, 0.28f});
    // Side aero skirts
    addBox(v, idx, {-0.96f, 0.12f, 0.05f}, {0.06f, 0.03f, 1.05f});
    addBox(v, idx, {0.96f, 0.12f, 0.05f}, {0.06f, 0.03f, 1.05f});
    // Rear diffuser + vertical aero strakes
    addBox(v, idx, {0.0f, 0.19f, 2.08f}, {0.78f, 0.06f, 0.24f}, -0.18f);
    for (float sx : {-0.52f, -0.26f, 0.0f, 0.26f, 0.52f}) {
      addBox(v, idx, {sx, 0.22f, 2.16f}, {0.015f, 0.09f, 0.18f});
    }
    // Swan-neck rear wing uprights
    addBox(v, idx, {-0.44f, 1.02f, 2.04f}, {0.025f, 0.22f, 0.09f}, 0.22f);
    addBox(v, idx, {0.44f, 1.02f, 2.04f}, {0.025f, 0.22f, 0.09f}, 0.22f);
    // Main airfoil wing blade + endplates
    addBox(v, idx, {0.0f, 1.22f, 2.10f}, {0.96f, 0.025f, 0.19f}, -0.14f);
    addBox(v, idx, {-0.97f, 1.20f, 2.10f}, {0.015f, 0.11f, 0.22f});
    addBox(v, idx, {0.97f, 1.20f, 2.10f}, {0.015f, 0.11f, 0.22f});
    // Side mirrors
    addBox(v, idx, {-0.88f, 0.80f, -0.68f}, {0.10f, 0.045f, 0.07f});
    addBox(v, idx, {0.88f, 0.80f, -0.68f}, {0.10f, 0.045f, 0.07f});
    carbonAero_.upload(v, idx);
  }

  // 4. LED DRL Headlights
  {
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    addBox(v, idx, {-0.68f, 0.49f, -2.07f}, {0.19f, 0.032f, 0.07f}, 0.22f);
    addBox(v, idx, {0.68f, 0.49f, -2.07f}, {0.19f, 0.032f, 0.07f}, 0.22f);
    headlights_.upload(v, idx);
  }

  // 5. Rear Full-Width LED Tail Light Bar
  {
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    addBox(v, idx, {0.0f, 0.67f, 2.22f}, {0.74f, 0.028f, 0.03f});
    addBox(v, idx, {-0.64f, 0.67f, 2.21f}, {0.14f, 0.055f, 0.035f});
    addBox(v, idx, {0.64f, 0.67f, 2.21f}, {0.14f, 0.055f, 0.035f});
    taillights_.upload(v, idx);
  }

  // 6. Twin Titanium Exhaust Tips
  {
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    addBox(v, idx, {-0.22f, 0.33f, 2.24f}, {0.06f, 0.045f, 0.08f});
    addBox(v, idx, {0.22f, 0.33f, 2.24f}, {0.06f, 0.045f, 0.08f});
    exhaust_.upload(v, idx);
  }

  // 7. Wheel Tire (radius 0.36m, width 0.28m)
  {
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    buildCylinderX(v, idx, 0.36f, 0.245f, 0.14f, 28, true);
    wheelTire_.upload(v, idx);
  }

  // 8. Forged 10-Spoke Alloy Rim
  {
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    buildCylinderX(v, idx, 0.245f, 0.22f, 0.125f, 24, true);
    // 10 spokes (5 diametric bars)
    for (int s = 0; s < 5; ++s) {
      float a = static_cast<float>(s) * (kPi / 5.0f);
      float cy = std::cos(a);
      float sz = std::sin(a);
      Vec3 dir{0.0f, cy, sz};
      Vec3 perp{0.0f, -sz, cy};
      Vec3 c{0.0f, 0.0f, 0.0f};
      // Build thin spoke bar at outer rim face
      float xOuter = 0.11f;
      float xInner = 0.06f;
      Vec3 p0 = dir * (-0.23f) - perp * 0.016f;
      Vec3 p1 = dir * (0.23f) - perp * 0.016f;
      Vec3 p2 = dir * (0.23f) + perp * 0.016f;
      Vec3 p3 = dir * (-0.23f) + perp * 0.016f;
      uint32_t base = static_cast<uint32_t>(v.size());
      v.push_back({xOuter, p0.y, p0.z, 1, 0, 0, 0, 0, 0, 1, 0});
      v.push_back({xOuter, p1.y, p1.z, 1, 0, 0, 1, 0, 0, 1, 0});
      v.push_back({xOuter, p2.y, p2.z, 1, 0, 0, 1, 1, 0, 1, 0});
      v.push_back({xOuter, p3.y, p3.z, 1, 0, 0, 0, 1, 0, 1, 0});
      v.push_back({-xOuter, p0.y, p0.z, -1, 0, 0, 0, 0, 0, 1, 0});
      v.push_back({-xOuter, p1.y, p1.z, -1, 0, 0, 1, 0, 0, 1, 0});
      v.push_back({-xOuter, p2.y, p2.z, -1, 0, 0, 1, 1, 0, 1, 0});
      v.push_back({-xOuter, p3.y, p3.z, -1, 0, 0, 0, 1, 0, 1, 0});
      (void)xInner;
      (void)c;
      idx.push_back(base + 0); idx.push_back(base + 1); idx.push_back(base + 2);
      idx.push_back(base + 0); idx.push_back(base + 2); idx.push_back(base + 3);
      idx.push_back(base + 4); idx.push_back(base + 6); idx.push_back(base + 5);
      idx.push_back(base + 4); idx.push_back(base + 7); idx.push_back(base + 6);
    }
    // Center lock hub
    addBox(v, idx, {0.0f, 0.0f, 0.0f}, {0.125f, 0.055f, 0.055f});
    wheelRim_.upload(v, idx);
  }

  // 9. Ventilated Brake Rotor Disc
  {
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    buildCylinderX(v, idx, 0.19f, 0.09f, 0.018f, 20, true);
    brakeDisc_.upload(v, idx);
  }

  // 10. Performance Brake Caliper
  {
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    addBox(v, idx, {0.0f, 0.05f, 0.155f}, {0.036f, 0.075f, 0.05f});
    brakeCaliper_.upload(v, idx);
  }
}

void CarMesh::destroy() {
  bodyShell_.destroy();
  glassCanopy_.destroy();
  carbonAero_.destroy();
  headlights_.destroy();
  taillights_.destroy();
  exhaust_.destroy();
  wheelTire_.destroy();
  wheelRim_.destroy();
  brakeDisc_.destroy();
  brakeCaliper_.destroy();
}

}  // namespace hypertrack
