package com.example

import android.opengl.GLSurfaceView
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.BackHandler
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectTransformGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawing
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Cameraswitch
import androidx.compose.material.icons.filled.DirectionsCar
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material.icons.filled.SportsEsports
import androidx.compose.material.icons.filled.Tune
import androidx.compose.material.icons.filled.Videocam
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.engine.AppMode
import com.example.engine.EngineBridge
import com.example.engine.GameUiState
import com.example.engine.GameViewModel
import com.example.engine.ShowcaseCamera
import com.example.ui.components.DigitalTelemetryCluster
import com.example.ui.components.DriftIndicatorBadge
import com.example.ui.components.DrivingControlsOverlay
import com.example.ui.components.GraphicsPipelineSheet
import com.example.ui.components.LapTimingBadge
import com.example.ui.components.ShowcaseBottomDock
import com.example.ui.components.TrackMinimapRadar
import com.example.ui.theme.ApexCrimson
import com.example.ui.theme.ApexCyan
import com.example.ui.theme.ApexGtTheme
import com.example.ui.theme.GlassBorder
import com.example.ui.theme.OrbitronFamily
import com.example.ui.theme.RajdhaniFamily
import com.example.ui.theme.SmokedGlass
import com.example.ui.theme.TextPrimary
import com.example.ui.theme.TextSecondary
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MainActivity : ComponentActivity() {
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    enableEdgeToEdge()
    setContent {
      ApexGtTheme {
        HyperTrackRootScreen()
      }
    }
  }
}

@Composable
fun HyperTrackRootScreen(vm: GameViewModel = viewModel()) {
  val uiState by vm.uiState.collectAsStateWithLifecycle()

  // Handle back button to exit Graphics Sheet or return from Drive Mode to Showcase Mode
  BackHandler(enabled = uiState.showGraphicsSheet || uiState.mode == AppMode.DRIVE) {
    if (uiState.showGraphicsSheet) {
      vm.setGraphicsSheetOpen(false)
    } else if (uiState.mode == AppMode.DRIVE) {
      vm.setMode(AppMode.SHOWCASE)
    }
  }

  Box(modifier = Modifier.fillMaxSize()) {
    // Fullscreen Native C++ OpenGL ES 3.0 Viewport
    NativeGlViewport(
      onSurfaceReady = { vm.syncEngineOnSurfaceReady() },
      modifier = Modifier
        .fillMaxSize()
        .then(
          if (uiState.mode == AppMode.SHOWCASE && uiState.showcaseCamera == ShowcaseCamera.ORBIT) {
            Modifier.pointerInput(Unit) {
              detectTransformGestures { _, pan, zoom, _ ->
                vm.onOrbitTouch(pan.x, pan.y, zoom)
              }
            }
          } else {
            Modifier
          }
        ),
    )

    // Safe-area HUD Overlay
    Box(
      modifier = Modifier
        .fillMaxSize()
        .windowInsetsPadding(WindowInsets.safeDrawing),
    ) {
      if (uiState.mode == AppMode.SHOWCASE) {
        ShowcaseHudOverlay(state = uiState, vm = vm)
      } else {
        DriveHudOverlay(state = uiState, vm = vm)
      }
    }

    if (uiState.showGraphicsSheet) {
      GraphicsPipelineSheet(
        graphics = uiState.graphics,
        fps = uiState.telemetry.fps,
        onUpdate = { transform -> vm.updateGraphics(transform) },
        onDismiss = { vm.setGraphicsSheetOpen(false) },
      )
    }
  }
}

@Composable
private fun NativeGlViewport(
  onSurfaceReady: () -> Unit,
  modifier: Modifier = Modifier,
) {
  val context = LocalContext.current
  val lifecycleOwner = LocalLifecycleOwner.current

  val glView = remember(context) {
    GLSurfaceView(context).apply {
      setEGLContextClientVersion(3)
      preserveEGLContextOnPause = true
      setRenderer(object : GLSurfaceView.Renderer {
        override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
          EngineBridge.nativeInit()
          onSurfaceReady()
        }

        override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
          EngineBridge.nativeResize(width, height)
        }

        override fun onDrawFrame(gl: GL10?) {
          EngineBridge.nativeRender()
        }
      })
      renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY
    }
  }

  DisposableEffect(lifecycleOwner, glView) {
    val observer = LifecycleEventObserver { _, event ->
      when (event) {
        Lifecycle.Event.ON_RESUME -> glView.onResume()
        Lifecycle.Event.ON_PAUSE -> glView.onPause()
        else -> {}
      }
    }
    lifecycleOwner.lifecycle.addObserver(observer)
    onDispose {
      lifecycleOwner.lifecycle.removeObserver(observer)
      glView.queueEvent { EngineBridge.nativeDestroy() }
    }
  }

  AndroidView(
    factory = { glView },
    modifier = modifier,
  )
}

@Composable
private fun ShowcaseHudOverlay(
  state: GameUiState,
  vm: GameViewModel,
) {
  Box(modifier = Modifier.fillMaxSize()) {
    // Top Header Bar
    Row(
      modifier = Modifier
        .fillMaxWidth()
        .padding(horizontal = 14.dp, vertical = 10.dp)
        .align(Alignment.TopCenter),
      horizontalArrangement = Arrangement.SpaceBetween,
      verticalAlignment = Alignment.Top,
    ) {
      // Brand + Track Info Pill
      Surface(
        shape = RoundedCornerShape(16.dp),
        color = SmokedGlass,
        border = BorderStroke(1.dp, GlassBorder),
      ) {
        Row(
          modifier = Modifier.padding(horizontal = 14.dp, vertical = 10.dp),
          verticalAlignment = Alignment.CenterVertically,
        ) {
          Icon(
            imageVector = Icons.Default.DirectionsCar,
            contentDescription = "ApexGT 3D",
            tint = ApexCrimson,
            modifier = Modifier.size(22.dp),
          )
          Spacer(modifier = Modifier.width(10.dp))
          Column {
            Text(
              text = "APEXGT 3D",
              fontFamily = OrbitronFamily,
              fontSize = 14.sp,
              color = TextPrimary,
            )
            Text(
              text = "${state.track.title}  •  ${state.weather.label}",
              style = MaterialTheme.typography.labelSmall,
              color = ApexCyan,
            )
          }
        }
      }

      // Live Speed/FPS + DRIVE CTA
      Row(
        horizontalArrangement = Arrangement.spacedBy(10.dp),
        verticalAlignment = Alignment.CenterVertically,
      ) {
        Surface(
          shape = RoundedCornerShape(14.dp),
          color = SmokedGlass,
          border = BorderStroke(1.dp, GlassBorder),
        ) {
          Column(
            modifier = Modifier.padding(horizontal = 12.dp, vertical = 6.dp),
            horizontalAlignment = Alignment.End,
          ) {
            Text(
              text = "${state.telemetry.speedKmh.toInt()} KM/H",
              fontFamily = OrbitronFamily,
              fontSize = 13.sp,
              color = TextPrimary,
            )
            Text(
              text = "C++ GLES3 • ${state.telemetry.fps.toInt()} FPS",
              style = MaterialTheme.typography.labelSmall,
              color = TextSecondary,
            )
          }
        }

        // Primary Drive CTA Button
        Surface(
          modifier = Modifier
            .testTag("enter_drive_mode_btn")
            .height(48.dp)
            .clickable { vm.setMode(AppMode.DRIVE) },
          shape = RoundedCornerShape(999.dp),
          color = ApexCrimson,
          shadowElevation = 6.dp,
        ) {
          Row(
            modifier = Modifier.padding(horizontal = 18.dp),
            verticalAlignment = Alignment.CenterVertically,
          ) {
            Icon(
              imageVector = Icons.Default.SportsEsports,
              contentDescription = "Drive Car",
              tint = Color.White,
              modifier = Modifier.size(20.dp),
            )
            Spacer(modifier = Modifier.width(8.dp))
            Text(
              text = "DRIVE",
              fontFamily = OrbitronFamily,
              fontSize = 13.sp,
              color = Color.White,
            )
          }
        }
      }
    }

    // Orbit Drag Hint when in Orbit mode and panel visible
    if (state.showcaseCamera == ShowcaseCamera.ORBIT && state.showShowcasePanel) {
      Surface(
        modifier = Modifier
          .align(Alignment.TopCenter)
          .padding(top = 74.dp),
        shape = RoundedCornerShape(999.dp),
        color = SmokedGlass.copy(alpha = 0.65f),
      ) {
        Text(
          text = "Drag to orbit 360° • Pinch to zoom",
          style = MaterialTheme.typography.labelSmall,
          color = TextSecondary,
          modifier = Modifier.padding(horizontal = 12.dp, vertical = 4.dp),
        )
      }
    }

    // Bottom Track Showcase Dock
    ShowcaseBottomDock(
      state = state,
      onSelectTrack = vm::selectTrack,
      onSelectLivery = vm::selectLivery,
      onSelectWeather = vm::selectWeather,
      onSelectTimeOfDay = vm::selectTimeOfDay,
      onSelectCamera = vm::selectShowcaseCamera,
      onToggleCruise = vm::toggleShowcaseCruise,
      onTogglePanel = vm::toggleShowcasePanel,
      onOpenGraphics = { vm.setGraphicsSheetOpen(true) },
      modifier = Modifier.align(Alignment.BottomCenter),
    )
  }
}

@Composable
private fun DriveHudOverlay(
  state: GameUiState,
  vm: GameViewModel,
) {
  BoxWithConstraints(modifier = Modifier.fillMaxSize()) {
    val isWide = maxWidth >= 600.dp

    Column(
      modifier = Modifier
        .fillMaxWidth()
        .padding(horizontal = 14.dp, vertical = 10.dp)
        .align(Alignment.TopCenter),
      verticalArrangement = Arrangement.spacedBy(8.dp),
    ) {
      Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.Top,
      ) {
        // Left: Minimap + Lap Timing
        Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
          Row(
            horizontalArrangement = Arrangement.spacedBy(8.dp),
            verticalAlignment = Alignment.Top,
          ) {
            if (isWide) {
              TrackMinimapRadar(
                points = state.minimapPoints,
                trackProgress = state.telemetry.trackProgress,
              )
            }
            LapTimingBadge(telemetry = state.telemetry)
          }
          DriftIndicatorBadge(driftAngleDeg = state.telemetry.driftAngleDeg)
        }

        if (isWide) {
          DigitalTelemetryCluster(
            telemetry = state.telemetry,
            modifier = Modifier.padding(horizontal = 6.dp),
          )
        }

        // Right: Camera, Steering Style, Reset, Graphics, Showcase Back
        Column(
          horizontalAlignment = Alignment.End,
          verticalArrangement = Arrangement.spacedBy(8.dp),
        ) {
          Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
            HudIconButton(
              tag = "drive_camera_btn",
              onClick = vm::cycleDriveCamera,
            ) {
              Icon(
                imageVector = Icons.Default.Cameraswitch,
                contentDescription = "Switch Drive Camera (${state.driveCamera.label})",
                tint = ApexCyan,
              )
            }

            HudIconButton(
              tag = "steer_style_btn",
              onClick = vm::toggleSteerStyle,
            ) {
              Text(
                text = state.steerStyle.label.take(3).uppercase(),
                fontFamily = RajdhaniFamily,
                fontSize = 12.sp,
                color = TextPrimary,
              )
            }

            HudIconButton(
              tag = "reset_car_btn",
              onClick = vm::resetCar,
            ) {
              Icon(
                imageVector = Icons.Default.Refresh,
                contentDescription = "Reset Car to Start Line",
                tint = TextPrimary,
              )
            }

            HudIconButton(
              tag = "drive_graphics_btn",
              onClick = { vm.setGraphicsSheetOpen(true) },
            ) {
              Icon(
                imageVector = Icons.Default.Tune,
                contentDescription = "Graphics Settings",
                tint = TextPrimary,
              )
            }
          }

          // Return to Track Showcase ("Track Shower")
          Surface(
            modifier = Modifier
              .testTag("enter_showcase_mode_btn")
              .height(44.dp)
              .clickable { vm.setMode(AppMode.SHOWCASE) },
            shape = RoundedCornerShape(999.dp),
            color = SmokedGlass,
            border = BorderStroke(1.dp, ApexCyan.copy(alpha = 0.7f)),
          ) {
            Row(
              modifier = Modifier.padding(horizontal = 14.dp),
              verticalAlignment = Alignment.CenterVertically,
            ) {
              Icon(
                imageVector = Icons.Default.Videocam,
                contentDescription = "Track Showcase",
                tint = ApexCyan,
                modifier = Modifier.size(18.dp),
              )
              Spacer(modifier = Modifier.width(6.dp))
              Text(
                text = "SHOWCASE",
                fontFamily = OrbitronFamily,
                fontSize = 11.sp,
                color = TextPrimary,
              )
            }
          }
        }
      }

      if (!isWide) {
        Row(
          modifier = Modifier.fillMaxWidth(),
          horizontalArrangement = Arrangement.SpaceBetween,
          verticalAlignment = Alignment.CenterVertically,
        ) {
          TrackMinimapRadar(
            points = state.minimapPoints,
            trackProgress = state.telemetry.trackProgress,
          )
          DigitalTelemetryCluster(telemetry = state.telemetry)
        }
      }
    }

    // Bottom Multi-Touch Driving Pedals & Steering
    DrivingControlsOverlay(
      steerStyle = state.steerStyle,
      nitroAmount = state.telemetry.nitroAmount,
      onSteerLeft = vm::setSteerLeft,
      onSteerRight = vm::setSteerRight,
      onAnalogSteer = vm::setAnalogSteer,
      onThrottle = vm::setThrottle,
      onBrake = vm::setBrake,
      onHandbrake = vm::setHandbrake,
      onNitro = vm::setNitro,
      modifier = Modifier.align(Alignment.BottomCenter),
    )
  }
}

@Composable
private fun HudIconButton(
  tag: String,
  onClick: () -> Unit,
  content: @Composable () -> Unit,
) {
  Surface(
    shape = RoundedCornerShape(12.dp),
    color = SmokedGlass,
    border = BorderStroke(1.dp, GlassBorder),
  ) {
    IconButton(
      onClick = onClick,
      modifier = Modifier
        .size(48.dp)
        .testTag(tag),
    ) {
      content()
    }
  }
}
