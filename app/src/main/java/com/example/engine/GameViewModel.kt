package com.example.engine

import androidx.compose.ui.graphics.Color
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

enum class AppMode {
  SHOWCASE,
  DRIVE,
}

enum class SteerControlStyle(val label: String) {
  TOUCH_PADS("Arrows"),
  STEERING_WHEEL("Wheel"),
}

enum class TrackOption(val id: Int, val title: String, val subtitle: String) {
  APEX_GP(0, "Apex GP", "High-Speed Circuit"),
  MONACO_NIGHT(1, "Harbor Street", "Technical Chicane"),
  ALPINE_PASS(2, "Alpine Touge", "Mountain Elevation"),
}

enum class LiveryOption(val id: Int, val label: String, val swatch: Color) {
  ROSSO_CORSA(0, "Rosso Corsa", Color(0xFFD6141F)),
  STEALTH_CARBON(1, "Stealth Carbon", Color(0xFF1F242D)),
  LIQUID_SILVER(2, "Liquid Silver", Color(0xFFC8CFD8)),
  APEX_CYAN(3, "Apex Cyan", Color(0xFF00B4F0)),
  MIDNIGHT_EMERALD(4, "Racing Emerald", Color(0xFF0E683B)),
}

enum class WeatherOption(val id: Int, val label: String) {
  DRY(0, "Dry"),
  WET_PUDDLES(1, "Wet Puddles"),
  RAIN_SHOWER(2, "Rain Shower"),
}

enum class TimeOfDayOption(val id: Int, val label: String) {
  GOLDEN_SUNSET(0, "Sunset"),
  MIDNIGHT_NEON(1, "Midnight"),
  ALPINE_OVERCAST(2, "Overcast"),
  HIGH_NOON(3, "Noon"),
}

enum class DriveCamera(val id: Int, val label: String) {
  CHASE(0, "Chase"),
  HOOD(1, "Hood"),
  BUMPER(2, "Bumper"),
}

enum class ShowcaseCamera(val id: Int, val label: String) {
  ORBIT(3, "Orbit 360"),
  DRONE(4, "Drone Flyby"),
  CURB(5, "Trackside"),
}

data class TelemetryState(
  val speedKmh: Float = 0f,
  val rpm: Float = 1200f,
  val gear: Int = 1,
  val currentLapTime: Float = 0f,
  val bestLapTime: Float = 0f,
  val lapCount: Int = 1,
  val trackProgress: Float = 0f,
  val driftAngleDeg: Float = 0f,
  val lateralG: Float = 0f,
  val longitudinalG: Float = 0f,
  val nitroAmount: Float = 1f,
  val nitroActive: Boolean = false,
  val onTrack: Boolean = true,
  val fps: Float = 60f,
  val trackLengthM: Float = 1600f,
  val brakeGlow: Float = 0f,
)

data class GraphicsConfig(
  val bloomEnabled: Boolean = true,
  val reflectionsEnabled: Boolean = true,
  val shadowsEnabled: Boolean = true,
  val rainParticlesEnabled: Boolean = true,
  val exposure: Float = 1.18f,
)

data class GameUiState(
  val mode: AppMode = AppMode.SHOWCASE,
  val track: TrackOption = TrackOption.APEX_GP,
  val livery: LiveryOption = LiveryOption.ROSSO_CORSA,
  val weather: WeatherOption = WeatherOption.WET_PUDDLES,
  val timeOfDay: TimeOfDayOption = TimeOfDayOption.GOLDEN_SUNSET,
  val driveCamera: DriveCamera = DriveCamera.CHASE,
  val showcaseCamera: ShowcaseCamera = ShowcaseCamera.ORBIT,
  val showcaseCruising: Boolean = true,
  val steerStyle: SteerControlStyle = SteerControlStyle.TOUCH_PADS,
  val showGraphicsSheet: Boolean = false,
  val showShowcasePanel: Boolean = true,
  val graphics: GraphicsConfig = GraphicsConfig(),
  val telemetry: TelemetryState = TelemetryState(),
  val minimapPoints: List<Pair<Float, Float>> = emptyList(),
)

class GameViewModel : ViewModel() {
  private val _uiState = MutableStateFlow(GameUiState())
  val uiState: StateFlow<GameUiState> = _uiState.asStateFlow()

  private val telemetryBuf = FloatArray(16)
  private val minimapBuf = FloatArray(512)

  // Real-time driving control inputs updated directly from multi-touch overlays
  @Volatile private var steerLeft = false
  @Volatile private var steerRight = false
  @Volatile private var analogSteer = 0f
  @Volatile private var throttleDown = false
  @Volatile private var brakeDown = false
  @Volatile private var handbrakeDown = false
  @Volatile private var nitroDown = false

  init {
    viewModelScope.launch {
      while (true) {
        delay(33) // ~30Hz telemetry poll
        try {
          EngineBridge.nativeGetTelemetry(telemetryBuf)
          val count = if (_uiState.value.minimapPoints.isEmpty()) {
            EngineBridge.nativeGetMinimapPoints(minimapBuf)
          } else {
            0
          }
          _uiState.update { state ->
            val pts = if (count > 0) {
              List(count) { i -> minimapBuf[i * 2] to minimapBuf[i * 2 + 1] }
            } else {
              state.minimapPoints
            }
            state.copy(
              telemetry = TelemetryState(
                speedKmh = telemetryBuf[0],
                rpm = telemetryBuf[1],
                gear = telemetryBuf[2].toInt(),
                currentLapTime = telemetryBuf[3],
                bestLapTime = telemetryBuf[4],
                lapCount = telemetryBuf[5].toInt(),
                trackProgress = telemetryBuf[6],
                driftAngleDeg = telemetryBuf[7],
                lateralG = telemetryBuf[8],
                longitudinalG = telemetryBuf[9],
                nitroAmount = telemetryBuf[10],
                nitroActive = telemetryBuf[11] > 0.5f,
                onTrack = telemetryBuf[12] > 0.5f,
                fps = telemetryBuf[13],
                trackLengthM = telemetryBuf[14],
                brakeGlow = telemetryBuf[15],
              ),
              minimapPoints = pts,
            )
          }
        } catch (_: UnsatisfiedLinkError) {
          // Safe guard during Compose previews or Robolectric unit tests
        }
      }
    }
  }

  fun syncEngineOnSurfaceReady() {
    val s = _uiState.value
    EngineBridge.nativeSetTrack(s.track.id)
    EngineBridge.nativeSetLivery(s.livery.id)
    EngineBridge.nativeSetWeather(s.weather.id)
    EngineBridge.nativeSetTimeOfDay(s.timeOfDay.id)
    EngineBridge.nativeSetShowcaseCruise(s.showcaseCruising)
    EngineBridge.nativeSetCameraMode(
      if (s.mode == AppMode.DRIVE) s.driveCamera.id else s.showcaseCamera.id
    )
    pushGraphics(s.graphics)
    refreshMinimapSoon()
  }

  private fun refreshMinimapSoon() {
    viewModelScope.launch {
      delay(80)
      val count = EngineBridge.nativeGetMinimapPoints(minimapBuf)
      if (count > 0) {
        val pts = List(count) { i -> minimapBuf[i * 2] to minimapBuf[i * 2 + 1] }
        _uiState.update { it.copy(minimapPoints = pts) }
      }
    }
  }

  fun setMode(mode: AppMode) {
    _uiState.update { it.copy(mode = mode, showGraphicsSheet = false) }
    val s = _uiState.value
    EngineBridge.nativeSetCameraMode(
      if (mode == AppMode.DRIVE) s.driveCamera.id else s.showcaseCamera.id
    )
    if (mode == AppMode.SHOWCASE) {
      clearInputs()
    }
  }

  fun selectTrack(track: TrackOption) {
    _uiState.update { it.copy(track = track, minimapPoints = emptyList()) }
    EngineBridge.nativeSetTrack(track.id)
    refreshMinimapSoon()
  }

  fun selectLivery(livery: LiveryOption) {
    _uiState.update { it.copy(livery = livery) }
    EngineBridge.nativeSetLivery(livery.id)
  }

  fun selectWeather(weather: WeatherOption) {
    _uiState.update { it.copy(weather = weather) }
    EngineBridge.nativeSetWeather(weather.id)
  }

  fun selectTimeOfDay(tod: TimeOfDayOption) {
    _uiState.update { it.copy(timeOfDay = tod) }
    EngineBridge.nativeSetTimeOfDay(tod.id)
  }

  fun cycleDriveCamera() {
    val entries = DriveCamera.entries
    val next = entries[(_uiState.value.driveCamera.ordinal + 1) % entries.size]
    _uiState.update { it.copy(driveCamera = next) }
    EngineBridge.nativeSetCameraMode(next.id)
  }

  fun selectShowcaseCamera(cam: ShowcaseCamera) {
    _uiState.update { it.copy(showcaseCamera = cam) }
    EngineBridge.nativeSetCameraMode(cam.id)
  }

  fun toggleShowcaseCruise() {
    val next = !_uiState.value.showcaseCruising
    _uiState.update { it.copy(showcaseCruising = next) }
    EngineBridge.nativeSetShowcaseCruise(next)
  }

  fun toggleShowcasePanel() {
    _uiState.update { it.copy(showShowcasePanel = !_uiState.value.showShowcasePanel) }
  }

  fun setGraphicsSheetOpen(open: Boolean) {
    _uiState.update { it.copy(showGraphicsSheet = open) }
  }

  fun toggleSteerStyle() {
    val next = if (_uiState.value.steerStyle == SteerControlStyle.TOUCH_PADS) {
      SteerControlStyle.STEERING_WHEEL
    } else {
      SteerControlStyle.TOUCH_PADS
    }
    analogSteer = 0f
    steerLeft = false
    steerRight = false
    pushInputs()
    _uiState.update { it.copy(steerStyle = next) }
  }

  fun updateGraphics(transform: (GraphicsConfig) -> GraphicsConfig) {
    val updated = transform(_uiState.value.graphics)
    _uiState.update { it.copy(graphics = updated) }
    pushGraphics(updated)
  }

  private fun pushGraphics(g: GraphicsConfig) {
    EngineBridge.nativeSetGraphics(
      bloom = g.bloomEnabled,
      reflections = g.reflectionsEnabled,
      shadows = g.shadowsEnabled,
      rainParticles = g.rainParticlesEnabled,
      exposure = g.exposure,
    )
  }

  fun resetCar() {
    EngineBridge.nativeResetCar()
  }

  fun onOrbitTouch(dx: Float, dy: Float, pinchScale: Float) {
    EngineBridge.nativeOrbitTouch(dx, dy, pinchScale)
  }

  fun setSteerLeft(pressed: Boolean) {
    steerLeft = pressed
    pushInputs()
  }

  fun setSteerRight(pressed: Boolean) {
    steerRight = pressed
    pushInputs()
  }

  fun setAnalogSteer(value: Float) {
    analogSteer = value.coerceIn(-1f, 1f)
    pushInputs()
  }

  fun setThrottle(pressed: Boolean) {
    throttleDown = pressed
    pushInputs()
  }

  fun setBrake(pressed: Boolean) {
    brakeDown = pressed
    pushInputs()
  }

  fun setHandbrake(pressed: Boolean) {
    handbrakeDown = pressed
    pushInputs()
  }

  fun setNitro(pressed: Boolean) {
    nitroDown = pressed
    pushInputs()
  }

  private fun clearInputs() {
    steerLeft = false
    steerRight = false
    analogSteer = 0f
    throttleDown = false
    brakeDown = false
    handbrakeDown = false
    nitroDown = false
    pushInputs()
  }

  private fun pushInputs() {
    val padSteer = (if (steerRight) 1f else 0f) - (if (steerLeft) 1f else 0f)
    val steer = if (kotlin.math.abs(analogSteer) > 0.01f) analogSteer else padSteer
    EngineBridge.nativeSetInput(
      throttle = if (throttleDown) 1f else 0f,
      brake = if (brakeDown) 1f else 0f,
      steer = steer,
      handbrake = handbrakeDown,
      nitro = nitroDown,
    )
  }
}
