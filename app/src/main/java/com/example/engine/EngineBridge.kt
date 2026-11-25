package com.example.engine

object EngineBridge {
  init {
    System.loadLibrary("hypertrack")
  }

  external fun nativeInit()
  external fun nativeResize(width: Int, height: Int)
  external fun nativeRender()
  external fun nativeDestroy()

  external fun nativeSetInput(
    throttle: Float,
    brake: Float,
    steer: Float,
    handbrake: Boolean,
    nitro: Boolean,
  )

  external fun nativeOrbitTouch(dx: Float, dy: Float, pinchScale: Float)

  external fun nativeSetTrack(trackId: Int)
  external fun nativeSetLivery(liveryId: Int)
  external fun nativeSetWeather(weatherId: Int)
  external fun nativeSetTimeOfDay(todId: Int)
  external fun nativeSetCameraMode(camId: Int)
  external fun nativeSetShowcaseCruise(cruising: Boolean)
  external fun nativeSetGraphics(
    bloom: Boolean,
    reflections: Boolean,
    shadows: Boolean,
    rainParticles: Boolean,
    exposure: Float,
  )

  external fun nativeResetCar()
  external fun nativeGetTelemetry(outArray: FloatArray)
  external fun nativeGetMinimapPoints(outXY: FloatArray): Int
}
