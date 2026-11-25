#include "renderer.hpp"
#include <jni.h>
#include <memory>

namespace {
hypertrack::HyperTrackEngine gEngine;
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeInit(JNIEnv*, jobject) {
  gEngine.initGL();
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeResize(JNIEnv*, jobject, jint width, jint height) {
  gEngine.resize(width, height);
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeRender(JNIEnv*, jobject) {
  gEngine.renderFrame();
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeDestroy(JNIEnv*, jobject) {
  gEngine.destroyGL();
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeSetInput(JNIEnv*, jobject, jfloat throttle,
                                                    jfloat brake, jfloat steer, jboolean handbrake,
                                                    jboolean nitro) {
  hypertrack::DriverInput in;
  in.throttle = throttle;
  in.brake = brake;
  in.steer = steer;
  in.handbrake = handbrake == JNI_TRUE;
  in.nitro = nitro == JNI_TRUE;
  gEngine.setDriverInput(in);
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeOrbitTouch(JNIEnv*, jobject, jfloat dx, jfloat dy,
                                                      jfloat pinchScale) {
  gEngine.addOrbitTouch(dx, dy, pinchScale);
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeSetTrack(JNIEnv*, jobject, jint trackId) {
  gEngine.setTrack(static_cast<hypertrack::TrackId>(trackId));
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeSetLivery(JNIEnv*, jobject, jint liveryId) {
  gEngine.setLivery(static_cast<hypertrack::CarLivery>(liveryId));
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeSetWeather(JNIEnv*, jobject, jint weatherId) {
  gEngine.setWeather(static_cast<hypertrack::WeatherMode>(weatherId));
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeSetTimeOfDay(JNIEnv*, jobject, jint todId) {
  gEngine.setTimeOfDay(static_cast<hypertrack::TimeOfDay>(todId));
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeSetCameraMode(JNIEnv*, jobject, jint camId) {
  gEngine.setCameraMode(static_cast<hypertrack::CameraMode>(camId));
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeSetShowcaseCruise(JNIEnv*, jobject, jboolean cruising) {
  gEngine.setShowcaseCruise(cruising == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeSetGraphics(JNIEnv*, jobject, jboolean bloom,
                                                       jboolean reflections, jboolean shadows,
                                                       jboolean rainParticles, jfloat exposure) {
  hypertrack::GraphicsSettings gs;
  gs.bloomEnabled = bloom == JNI_TRUE;
  gs.reflectionsEnabled = reflections == JNI_TRUE;
  gs.shadowsEnabled = shadows == JNI_TRUE;
  gs.rainParticlesEnabled = rainParticles == JNI_TRUE;
  gs.exposure = exposure;
  gEngine.setGraphicsSettings(gs);
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeResetCar(JNIEnv*, jobject) {
  gEngine.resetCar();
}

JNIEXPORT void JNICALL
Java_com_example_engine_EngineBridge_nativeGetTelemetry(JNIEnv* env, jobject, jfloatArray outArray) {
  if (!outArray || env->GetArrayLength(outArray) < 16) return;
  float buf[16] = {0};
  gEngine.getTelemetry(buf);
  env->SetFloatArrayRegion(outArray, 0, 16, buf);
}

JNIEXPORT jint JNICALL
Java_com_example_engine_EngineBridge_nativeGetMinimapPoints(JNIEnv* env, jobject,
                                                            jfloatArray outXY) {
  if (!outXY) return 0;
  int maxPts = env->GetArrayLength(outXY) / 2;
  if (maxPts <= 0) return 0;
  std::vector<float> buf(static_cast<size_t>(maxPts) * 2);
  int count = gEngine.getMinimapPoints(buf.data(), maxPts);
  env->SetFloatArrayRegion(outXY, 0, count * 2, buf.data());
  return count;
}

}  // extern "C"
