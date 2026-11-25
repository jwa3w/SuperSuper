#pragma once

#include "track.hpp"

namespace hypertrack {

class CarMesh {
 public:
  void build();
  void destroy();

  const GpuMesh& bodyShell() const { return bodyShell_; }
  const GpuMesh& glassCanopy() const { return glassCanopy_; }
  const GpuMesh& carbonAero() const { return carbonAero_; }
  const GpuMesh& headlights() const { return headlights_; }
  const GpuMesh& taillights() const { return taillights_; }
  const GpuMesh& exhaust() const { return exhaust_; }

  const GpuMesh& wheelTire() const { return wheelTire_; }
  const GpuMesh& wheelRim() const { return wheelRim_; }
  const GpuMesh& brakeDisc() const { return brakeDisc_; }
  const GpuMesh& brakeCaliper() const { return brakeCaliper_; }

 private:
  GpuMesh bodyShell_;
  GpuMesh glassCanopy_;
  GpuMesh carbonAero_;
  GpuMesh headlights_;
  GpuMesh taillights_;
  GpuMesh exhaust_;

  GpuMesh wheelTire_;
  GpuMesh wheelRim_;
  GpuMesh brakeDisc_;
  GpuMesh brakeCaliper_;
};

}  // namespace hypertrack
