#include "shaders.hpp"
#include <android/log.h>
#include <vector>

#define LOG_TAG "HyperTrackShaders"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace hypertrack {

GLint ShaderProgram::loc(const char* name) const {
  return glGetUniformLocation(id, name);
}

GLuint compileShader(GLenum type, const char* source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  GLint compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    std::vector<char> log(len > 1 ? len : 1);
    glGetShaderInfoLog(shader, len, nullptr, log.data());
    LOGE("Shader compile error: %s", log.data());
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

GLuint linkProgram(const char* vertSrc, const char* fragSrc) {
  GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc);
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
  if (!vs || !fs) {
    if (vs) glDeleteShader(vs);
    if (fs) glDeleteShader(fs);
    return 0;
  }
  GLuint prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);
  glDeleteShader(vs);
  glDeleteShader(fs);
  GLint linked = 0;
  glGetProgramiv(prog, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint len = 0;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
    std::vector<char> log(len > 1 ? len : 1);
    glGetProgramInfoLog(prog, len, nullptr, log.data());
    LOGE("Program link error: %s", log.data());
    glDeleteProgram(prog);
    return 0;
  }
  return prog;
}

const char* kSkyVert = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;

uniform mat4 uViewProjNoTranslate;

out vec3 vDir;

void main() {
  vDir = aPos;
  vec4 pos = uViewProjNoTranslate * vec4(aPos, 1.0);
  gl_Position = pos.xyww;
}
)";

const char* kSkyFrag = R"(#version 300 es
precision highp float;

in vec3 vDir;

uniform vec3 uSunDir;
uniform vec3 uSunColor;
uniform vec3 uSkyZenith;
uniform vec3 uSkyHorizon;
uniform float uNightFactor;
uniform float uCloudDensity;
uniform float uTime;

out vec4 fragColor;

float hash21(vec2 p) {
  p = fract(p * vec2(123.34, 456.21));
  p += dot(p, p + 45.32);
  return fract(p.x * p.y);
}

float noise2(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  return mix(
    mix(hash21(i), hash21(i + vec2(1.0, 0.0)), u.x),
    mix(hash21(i + vec2(0.0, 1.0)), hash21(i + vec2(1.0, 1.0)), u.x),
    u.y
  );
}

float fbm(vec2 p) {
  float v = 0.0;
  float a = 0.5;
  for (int i = 0; i < 4; ++i) {
    v += a * noise2(p);
    p *= 2.04;
    a *= 0.5;
  }
  return v;
}

void main() {
  vec3 dir = normalize(vDir);
  float elev = clamp(dir.y, 0.0, 1.0);
  float horizonGlow = pow(1.0 - elev, 4.2);

  vec3 col = mix(uSkyHorizon, uSkyZenith, pow(elev, 0.55));

  // Mie solar glow & crisp solar disc
  float sunCos = max(dot(dir, normalize(uSunDir)), 0.0);
  float mieHalo = pow(sunCos, 10.0) * 0.45 + pow(sunCos, 96.0) * 1.1;
  float sunDisc = smoothstep(0.9992, 0.9997, sunCos) * (1.0 - uNightFactor * 0.65);
  col += uSunColor * (mieHalo * 0.75 + sunDisc * 3.8);

  // Horizon atmospheric band
  col += uSkyHorizon * horizonGlow * 0.35;

  // Procedural volumetric clouds above horizon
  if (dir.y > 0.015) {
    vec2 cloudUv = dir.xz / (dir.y + 0.14) * 1.25 + vec2(uTime * 0.012, uTime * 0.004);
    float c = fbm(cloudUv);
    float mask = smoothstep(0.42 - uCloudDensity * 0.18, 0.85, c) * smoothstep(0.015, 0.18, dir.y);
    vec3 cloudDark = mix(uSkyZenith * 0.55, vec3(0.18, 0.20, 0.26), 0.5);
    vec3 cloudLit = mix(uSkyHorizon * 1.15, uSunColor * 1.3, pow(sunCos, 3.0) * 0.7);
    vec3 cloudCol = mix(cloudDark, cloudLit, smoothstep(0.35, 0.85, c));
    col = mix(col, cloudCol, mask * (0.55 + uCloudDensity * 0.35));
  }

  // Stars on clear/midnight sky
  if (uNightFactor > 0.25 && dir.y > 0.05) {
    vec2 starUv = floor(dir.xz / (dir.y + 0.3) * 180.0);
    float s = hash21(starUv);
    if (s > 0.9965) {
      float twinkle = 0.65 + 0.35 * sin(uTime * 3.0 + s * 100.0);
      col += vec3(0.85, 0.92, 1.0) * twinkle * uNightFactor * smoothstep(0.05, 0.3, dir.y);
    }
  }

  // City / stadium horizon ambient bounce at night
  if (uNightFactor > 0.3) {
    float cityAngle = sin(atan(dir.z, dir.x) * 6.0) * 0.5 + 0.5;
    col += mix(vec3(0.0, 0.35, 0.55), vec3(0.55, 0.12, 0.28), cityAngle) *
           horizonGlow * 0.28 * uNightFactor;
  }

  fragColor = vec4(col, 1.0);
}
)";

const char* kShadowVert = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;

uniform mat4 uLightSpaceMatrix;
uniform mat4 uModel;

void main() {
  gl_Position = uLightSpaceMatrix * uModel * vec4(aPos, 1.0);
}
)";

const char* kShadowFrag = R"(#version 300 es
precision highp float;

void main() {
  // Depth written automatically
}
)";

const char* kPbrVert = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;
layout(location = 3) in vec3 aTangent;

uniform mat4 uModel;
uniform mat4 uViewProj;
uniform mat4 uLightSpaceMatrix;
uniform mat3 uNormalMatrix;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUv;
out vec3 vTangent;
out vec4 vLightSpacePos;

void main() {
  vec4 worldPos = uModel * vec4(aPos, 1.0);
  vWorldPos = worldPos.xyz;
  vNormal = normalize(uNormalMatrix * aNormal);
  vTangent = normalize(uNormalMatrix * aTangent);
  vUv = aUv;
  vLightSpacePos = uLightSpaceMatrix * worldPos;
  gl_Position = uViewProj * worldPos;
}
)";

const char* kPbrFrag = R"(#version 300 es
precision highp float;
precision highp sampler2D;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUv;
in vec3 vTangent;
in vec4 vLightSpacePos;

uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uShadowMap;
uniform int uUseTextures;

uniform vec3 uCamPos;
uniform vec3 uSunDir;
uniform vec3 uSunColor;
uniform vec3 uSkyZenith;
uniform vec3 uSkyHorizon;
uniform float uNightFactor;

uniform vec3 uBaseColor;
uniform float uMetallic;
uniform float uRoughness;
uniform float uClearcoat;
uniform float uClearcoatRoughness;
uniform vec3 uEmissive;
uniform int uSurfaceType; // 0=generic, 1=asphalt, 2=kerb, 3=carbon, 4=terrain, 5=carpaint, 6=glass, 7=brake

uniform float uWetness;
uniform float uRainIntensity;
uniform float uTime;
uniform int uReflectionsEnabled;
uniform int uShadowsEnabled;

// Dynamic car lighting
uniform vec3 uCarPos;
uniform vec3 uCarForward;
uniform vec3 uCarRight;
uniform float uBrakeGlow;

out vec4 fragColor;

const float PI = 3.14159265359;

float hash21(vec2 p) {
  p = fract(p * vec2(127.1, 311.7));
  p += dot(p, p + 34.5);
  return fract(p.x * p.y);
}

float noise2(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  return mix(
    mix(hash21(i), hash21(i + vec2(1.0, 0.0)), u.x),
    mix(hash21(i + vec2(0.0, 1.0)), hash21(i + vec2(1.0, 1.0)), u.x),
    u.y
  );
}

float fbm2(vec2 p) {
  float v = 0.0;
  float a = 0.5;
  for (int i = 0; i < 3; ++i) {
    v += a * noise2(p);
    p *= 2.1;
    a *= 0.5;
  }
  return v;
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;
  float denom = NdotH2 * (a2 - 1.0) + 1.0;
  return a2 / max(PI * denom * denom, 0.0001);
}

float geometrySchlickGGX(float NdotV, float roughness) {
  float r = roughness + 1.0;
  float k = (r * r) / 8.0;
  return NdotV / max(NdotV * (1.0 - k) + k, 0.0001);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
  return geometrySchlickGGX(max(dot(N, V), 0.0), roughness) *
         geometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 sampleEnvironment(vec3 R, float roughness) {
  vec3 dir = normalize(R);
  float elev = clamp(dir.y, -0.1, 1.0);
  vec3 sky = mix(uSkyHorizon, uSkyZenith, pow(max(elev, 0.0), 0.55));
  if (dir.y < 0.0) {
    vec3 groundBounce = mix(uSkyHorizon * 0.45, vec3(0.07, 0.08, 0.10), clamp(-dir.y * 4.0, 0.0, 1.0));
    sky = groundBounce;
  }
  float sunCos = max(dot(dir, normalize(uSunDir)), 0.0);
  float sharp = mix(48.0, 4.0, roughness);
  sky += uSunColor * pow(sunCos, sharp) * mix(1.4, 0.2, roughness);

  // Subtle stadium gantry / horizon specular highlights for automotive paint pop
  float streak = pow(max(0.0, sin(dir.x * 9.0 + dir.z * 7.0)), 8.0) *
                 smoothstep(0.0, 0.25, dir.y) * smoothstep(0.6, 0.15, dir.y);
  vec3 accent = mix(vec3(0.45, 0.55, 0.70), vec3(0.25, 0.75, 1.0), uNightFactor);
  sky += accent * streak * (1.0 - roughness) * (0.35 + 0.7 * uNightFactor);
  return sky;
}

float computeShadow(vec4 lightSpacePos, vec3 N, vec3 L) {
  if (uShadowsEnabled == 0) return 1.0;
  vec3 proj = lightSpacePos.xyz / lightSpacePos.w;
  proj = proj * 0.5 + 0.5;
  if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0) {
    return 1.0;
  }
  float bias = max(0.0022 * (1.0 - dot(N, L)), 0.0006);
  float shadow = 0.0;
  vec2 texelSize = vec2(1.0 / 1024.0);
  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      float pcfDepth = texture(uShadowMap, proj.xy + vec2(float(x), float(y)) * texelSize).r;
      shadow += (proj.z - bias) > pcfDepth ? 0.0 : 1.0;
    }
  }
  return shadow / 9.0;
}

void main() {
  vec3 N = normalize(vNormal);
  vec3 V = normalize(uCamPos - vWorldPos);
  vec3 L = normalize(uSunDir);

  vec3 albedo = uBaseColor;
  float metallic = uMetallic;
  float roughness = clamp(uRoughness, 0.04, 0.98);
  float clearcoat = uClearcoat;

  if (uUseTextures == 1) {
    vec3 texCol = texture(uAlbedoMap, vUv).rgb;
    albedo *= texCol;
    vec3 nSample = texture(uNormalMap, vUv).xyz * 2.0 - 1.0;
    vec3 T = normalize(vTangent - N * dot(N, vTangent));
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);
    float nStrength = (uSurfaceType == 1) ? 0.55 : (uSurfaceType == 3 ? 0.45 : 0.35);
    N = normalize(mix(N, normalize(TBN * nSample), nStrength));
  }

  // Asphalt wet puddles, racing line & animated raindrop ripples
  if (uSurfaceType == 1) {
    // Center rubbered-in racing line darkening
    float laneDist = abs(fract(vUv.x) - 0.5) * 2.0;
    float racingLine = exp(-laneDist * laneDist * 4.5);
    albedo *= mix(1.0, 0.78, racingLine);

    // Crisp dashed lane edge markings
    float edgeLine = smoothstep(0.89, 0.91, laneDist) * smoothstep(0.95, 0.93, laneDist);
    albedo = mix(albedo, vec3(0.86, 0.86, 0.88), edgeLine * 0.65);

    if (uWetness > 0.01) {
      float puddleNoise = fbm2(vWorldPos.xz * 0.11);
      float puddleMask = smoothstep(0.46 - uWetness * 0.22, 0.68 - uWetness * 0.12, puddleNoise) * uWetness;
      albedo *= mix(1.0, 0.48, uWetness * 0.65 + puddleMask * 0.35);
      roughness = mix(roughness, 0.045, puddleMask * 0.92 + uWetness * 0.35);
      clearcoat = max(clearcoat, puddleMask * 0.9);

      // Rain ripples on water puddles
      if (uRainIntensity > 0.05 && puddleMask > 0.25) {
        vec2 cell = floor(vWorldPos.xz * 2.8);
        vec2 local = fract(vWorldPos.xz * 2.8) - 0.5;
        float rnd = hash21(cell);
        float phase = fract(uTime * 1.6 + rnd);
        float dist = length(local - vec2(rnd - 0.5, fract(rnd * 13.7) - 0.5) * 0.4);
        float ring = sin((dist - phase * 0.48) * 45.0) * exp(-dist * 8.0) * (1.0 - phase);
        N = normalize(N + vec3(local.x, 0.0, local.y) * ring * 0.28 * puddleMask * uRainIntensity);
      }
    }
  }

  // Terrain organic color variation
  if (uSurfaceType == 4) {
    float macro = fbm2(vWorldPos.xz * 0.03);
    albedo = mix(albedo * 0.75, albedo * 1.25, macro);
    if (uWetness > 0.01) {
      albedo *= mix(1.0, 0.75, uWetness);
      roughness = mix(roughness, 0.45, uWetness * 0.5);
    }
  }

  // Contact AO shadow directly underneath the car chassis
  float contactAO = 1.0;
  if (uSurfaceType == 1 || uSurfaceType == 2 || uSurfaceType == 4) {
    vec3 rel = vWorldPos - uCarPos;
    float along = dot(rel, uCarForward);
    float across = dot(rel, uCarRight);
    float boxDist = length(vec2(max(abs(across) - 0.85, 0.0) * 1.3, max(abs(along) - 2.05, 0.0) * 0.85));
    float carShadow = exp(-boxDist * boxDist * 3.8) * smoothstep(1.2, 0.0, abs(rel.y));
    contactAO = 1.0 - carShadow * 0.82;
  }

  // Cook-Torrance Direct Sun PBR
  vec3 F0 = mix(vec3(0.04), albedo, metallic);
  vec3 H = normalize(V + L);
  float NDF = distributionGGX(N, H, roughness);
  float G = geometrySmith(N, V, L, roughness);
  vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

  vec3 numerator = NDF * G * F;
  float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
  vec3 specular = numerator / denom;

  vec3 kS = F;
  vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
  float NdotL = max(dot(N, L), 0.0);

  float shadow = computeShadow(vLightSpacePos, N, L);
  vec3 directLight = (kD * albedo / PI + specular) * uSunColor * NdotL * shadow;

  // Dual-lobe Automotive Clearcoat specular
  if (clearcoat > 0.01) {
    float ccNDF = distributionGGX(N, H, uClearcoatRoughness);
    float ccG = geometrySmith(N, V, L, uClearcoatRoughness);
    vec3 ccF = fresnelSchlick(max(dot(H, V), 0.0), vec3(0.04));
    vec3 ccSpec = (ccNDF * ccG * ccF) / denom;
    directLight += ccSpec * uSunColor * NdotL * shadow * clearcoat;
  }

  // Dynamic Car Headlights & Tail Lights projected on surrounding track
  vec3 dynamicLights = vec3(0.0);
  if (uSurfaceType != 5 && uSurfaceType != 6) {
    for (int i = 0; i < 2; ++i) {
      float side = (i == 0) ? -0.68 : 0.68;
      vec3 headPos = uCarPos + uCarForward * 2.15 + uCarRight * side + vec3(0.0, 0.55, 0.0);
      vec3 toFrag = vWorldPos - headPos;
      float dist = length(toFrag);
      if (dist < 42.0) {
        vec3 lDir = -toFrag / max(dist, 0.001);
        float cone = smoothstep(0.62, 0.94, dot(-lDir, normalize(uCarForward - vec3(0.0, 0.06, 0.0))));
        float atten = 1.0 / (1.0 + 0.055 * dist + 0.012 * dist * dist);
        float beamIntensity = mix(0.8, 3.6, uNightFactor) * cone * atten;
        vec3 beamCol = vec3(0.95, 0.97, 1.0) * beamIntensity;
        float hlNdotL = max(dot(N, lDir), 0.0);
        vec3 hlH = normalize(V + lDir);
        float hlSpec = distributionGGX(N, hlH, max(roughness, 0.08)) * 0.45;
        dynamicLights += (albedo / PI + vec3(hlSpec)) * beamCol * hlNdotL;
      }
    }
    // Rear tail/brake glow on wet tarmac
    vec3 tailPos = uCarPos - uCarForward * 2.15 + vec3(0.0, 0.65, 0.0);
    vec3 toTail = vWorldPos - tailPos;
    float tDist = length(toTail);
    if (tDist < 9.0) {
      float tAtten = 1.0 / (1.0 + 0.45 * tDist * tDist);
      vec3 tDir = -toTail / max(tDist, 0.001);
      float brakeBoost = 1.0 + uBrakeGlow * 2.2;
      dynamicLights += vec3(1.0, 0.08, 0.12) * brakeBoost * tAtten * max(dot(N, tDir), 0.0) * (albedo + vec3(0.25));
    }
  }

  // Hemisphere Ambient + Procedural Environment Reflections
  float hemi = N.y * 0.5 + 0.5;
  vec3 ambientColor = mix(uSkyHorizon * 0.25, uSkyZenith * 0.45 + vec3(0.05), hemi);
  vec3 ambient = kD * albedo * ambientColor * contactAO;

  vec3 envReflection = vec3(0.0);
  if (uReflectionsEnabled == 1) {
    vec3 R = reflect(-V, N);
    vec3 envCol = sampleEnvironment(R, roughness);
    vec3 fres = fresnelSchlick(max(dot(N, V), 0.0), F0);
    envReflection += envCol * fres * (1.0 - roughness * 0.72) * contactAO;
    if (clearcoat > 0.01) {
      vec3 ccEnv = sampleEnvironment(R, uClearcoatRoughness);
      vec3 ccFres = fresnelSchlick(max(dot(N, V), 0.0), vec3(0.04));
      envReflection += ccEnv * ccFres * clearcoat * 1.15;
    }
  }

  vec3 color = ambient + directLight + dynamicLights + envReflection + uEmissive;

  // Atmospheric distance fog matching sky horizon
  float camDist = length(uCamPos - vWorldPos);
  float fogFactor = 1.0 - exp(-pow(camDist * (0.0024 + uRainIntensity * 0.0028), 1.4));
  color = mix(color, uSkyHorizon * 0.92, clamp(fogFactor, 0.0, 0.92));

  fragColor = vec4(color, 1.0);
}
)";

const char* kParticleVert = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUv;
layout(location = 2) in vec4 aColor;

uniform mat4 uViewProj;

out vec2 vUv;
out vec4 vColor;

void main() {
  vUv = aUv;
  vColor = aColor;
  gl_Position = uViewProj * vec4(aPos, 1.0);
}
)";

const char* kParticleFrag = R"(#version 300 es
precision highp float;

in vec2 vUv;
in vec4 vColor;

out vec4 fragColor;

void main() {
  vec2 d = vUv * 2.0 - 1.0;
  float alpha = exp(-dot(d, d) * 2.8) * vColor.a;
  if (alpha < 0.01) discard;
  fragColor = vec4(vColor.rgb, alpha);
}
)";

const char* kQuadVert = R"(#version 300 es
precision highp float;

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUv;

out vec2 vUv;

void main() {
  vUv = aUv;
  gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* kBrightPassFrag = R"(#version 300 es
precision highp float;

in vec2 vUv;
uniform sampler2D uScene;

out vec4 fragColor;

void main() {
  vec3 col = texture(uScene, vUv).rgb;
  float lum = dot(col, vec3(0.2126, 0.7152, 0.0722));
  float weight = smoothstep(0.85, 1.65, lum);
  fragColor = vec4(col * weight, 1.0);
}
)";

const char* kBlurFrag = R"(#version 300 es
precision highp float;

in vec2 vUv;
uniform sampler2D uTex;
uniform vec2 uDirection;

out vec4 fragColor;

void main() {
  float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
  vec3 res = texture(uTex, vUv).rgb * weights[0];
  for (int i = 1; i < 5; ++i) {
    vec2 off = uDirection * float(i);
    res += texture(uTex, vUv + off).rgb * weights[i];
    res += texture(uTex, vUv - off).rgb * weights[i];
  }
  fragColor = vec4(res, 1.0);
}
)";

const char* kCompositeFrag = R"(#version 300 es
precision highp float;

in vec2 vUv;
uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform float uExposure;
uniform float uSpeedFactor;
uniform int uBloomEnabled;

out vec4 fragColor;

vec3 acesFilmic(vec3 x) {
  float a = 2.51;
  float b = 0.03;
  float c = 2.43;
  float d = 0.59;
  float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
  vec2 centered = vUv - 0.5;
  float dist = length(centered);

  // Subtle chromatic aberration on peripheral edges at high speed / nitro
  float ca = uSpeedFactor * 0.0045 * dist;
  vec3 col;
  if (ca > 0.0002) {
    col.r = texture(uScene, vUv + centered * ca).r;
    col.g = texture(uScene, vUv).g;
    col.b = texture(uScene, vUv - centered * ca).b;
  } else {
    col = texture(uScene, vUv).rgb;
  }

  if (uBloomEnabled == 1) {
    col += texture(uBloom, vUv).rgb * 0.65;
  }

  // Subtle cinematic vignette
  float vig = smoothstep(1.05, 0.35, dist * (1.0 + uSpeedFactor * 0.25));
  col *= mix(0.72, 1.0, vig);

  col = acesFilmic(col * uExposure);
  col = pow(col, vec3(1.0 / 2.2));
  fragColor = vec4(col, 1.0);
}
)";

}  // namespace hypertrack
