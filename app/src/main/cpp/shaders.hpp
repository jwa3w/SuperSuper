#pragma once

#include <GLES3/gl3.h>
#include <string>

namespace hypertrack {

struct ShaderProgram {
  GLuint id = 0;
  GLint loc(const char* name) const;
};

GLuint compileShader(GLenum type, const char* source);
GLuint linkProgram(const char* vertSrc, const char* fragSrc);

extern const char* kSkyVert;
extern const char* kSkyFrag;

extern const char* kShadowVert;
extern const char* kShadowFrag;

extern const char* kPbrVert;
extern const char* kPbrFrag;

extern const char* kParticleVert;
extern const char* kParticleFrag;

extern const char* kQuadVert;
extern const char* kBrightPassFrag;
extern const char* kBlurFrag;
extern const char* kCompositeFrag;

}  // namespace hypertrack
