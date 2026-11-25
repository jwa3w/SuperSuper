#pragma once

#include <GLES3/gl3.h>

namespace hypertrack {

struct TextureSet {
  GLuint asphaltAlbedo = 0;
  GLuint asphaltNormal = 0;
  GLuint kerbAlbedo = 0;
  GLuint kerbNormal = 0;
  GLuint carbonAlbedo = 0;
  GLuint carbonNormal = 0;
  GLuint terrainAlbedo = 0;
  GLuint terrainNormal = 0;

  void init();
  void destroy();
};

}  // namespace hypertrack
