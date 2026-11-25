package com.example.ui.components

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.expandVertically
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.shrinkVertically
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Tune
import androidx.compose.material.icons.filled.Visibility
import androidx.compose.material.icons.filled.VisibilityOff
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ModalBottomSheet
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Surface
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.engine.GameUiState
import com.example.engine.GraphicsConfig
import com.example.engine.LiveryOption
import com.example.engine.ShowcaseCamera
import com.example.engine.TimeOfDayOption
import com.example.engine.TrackOption
import com.example.engine.WeatherOption
import com.example.ui.theme.ApexCrimson
import com.example.ui.theme.ApexCyan
import com.example.ui.theme.CarbonSurface
import com.example.ui.theme.GlassBorder
import com.example.ui.theme.JetBrainsMonoFamily
import com.example.ui.theme.OrbitronFamily
import com.example.ui.theme.RajdhaniFamily
import com.example.ui.theme.SmokedGlass
import com.example.ui.theme.TelemetryEmerald
import com.example.ui.theme.TextPrimary
import com.example.ui.theme.TextSecondary
import kotlin.math.roundToInt

@Composable
fun ShowcaseBottomDock(
  state: GameUiState,
  onSelectTrack: (TrackOption) -> Unit,
  onSelectLivery: (LiveryOption) -> Unit,
  onSelectWeather: (WeatherOption) -> Unit,
  onSelectTimeOfDay: (TimeOfDayOption) -> Unit,
  onSelectCamera: (ShowcaseCamera) -> Unit,
  onToggleCruise: () -> Unit,
  onTogglePanel: () -> Unit,
  onOpenGraphics: () -> Unit,
  modifier: Modifier = Modifier,
) {
  Column(
    modifier = modifier
      .fillMaxWidth()
      .padding(horizontal = 14.dp, vertical = 10.dp),
    horizontalAlignment = Alignment.CenterHorizontally,
  ) {
    // Quick action toolbar row above dock
    Row(
      modifier = Modifier.fillMaxWidth(),
      horizontalArrangement = Arrangement.SpaceBetween,
      verticalAlignment = Alignment.CenterVertically,
    ) {
      Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
        // Cruise / Park car toggle
        Surface(
          modifier = Modifier
            .testTag("showcase_cruise_btn")
            .height(48.dp)
            .clickable(onClick = onToggleCruise),
          shape = RoundedCornerShape(999.dp),
          color = SmokedGlass,
          border = BorderStroke(1.dp, if (state.showcaseCruising) ApexCyan else GlassBorder),
        ) {
          Row(
            modifier = Modifier.padding(horizontal = 14.dp),
            verticalAlignment = Alignment.CenterVertically,
          ) {
            Icon(
              imageVector = if (state.showcaseCruising) Icons.Default.Pause else Icons.Default.PlayArrow,
              contentDescription = if (state.showcaseCruising) "Park Car" else "Cruise Track",
              tint = ApexCyan,
              modifier = Modifier.size(18.dp),
            )
            Spacer(modifier = Modifier.width(6.dp))
            Text(
              text = if (state.showcaseCruising) "Cruising" else "Parked",
              style = MaterialTheme.typography.titleMedium,
              color = TextPrimary,
            )
          }
        }

        // Graphics Pipeline Sheet trigger
        Surface(
          modifier = Modifier
            .testTag("graphics_settings_btn")
            .height(48.dp)
            .clickable(onClick = onOpenGraphics),
          shape = RoundedCornerShape(999.dp),
          color = SmokedGlass,
          border = BorderStroke(1.dp, GlassBorder),
        ) {
          Row(
            modifier = Modifier.padding(horizontal = 14.dp),
            verticalAlignment = Alignment.CenterVertically,
          ) {
            Icon(
              imageVector = Icons.Default.Tune,
              contentDescription = "Graphics Pipeline Settings",
              tint = TextPrimary,
              modifier = Modifier.size(18.dp),
            )
            Spacer(modifier = Modifier.width(6.dp))
            Text(
              text = "PBR & VFX",
              style = MaterialTheme.typography.titleMedium,
              color = TextPrimary,
            )
          }
        }
      }

      // Collapse / Expand dock button for pure 100% 3D viewport inspection
      Surface(
        shape = CircleShape,
        color = SmokedGlass,
        border = BorderStroke(1.dp, GlassBorder),
      ) {
        IconButton(
          onClick = onTogglePanel,
          modifier = Modifier
            .size(48.dp)
            .testTag("toggle_showcase_dock_btn"),
        ) {
          Icon(
            imageVector = if (state.showShowcasePanel) Icons.Default.VisibilityOff else Icons.Default.Visibility,
            contentDescription = if (state.showShowcasePanel) "Hide Controls" else "Show Controls",
            tint = TextPrimary,
          )
        }
      }
    }

    AnimatedVisibility(
      visible = state.showShowcasePanel,
      enter = fadeIn() + expandVertically(),
      exit = fadeOut() + shrinkVertically(),
    ) {
      Surface(
        modifier = Modifier
          .padding(top = 10.dp)
          .fillMaxWidth(),
        shape = RoundedCornerShape(20.dp),
        color = SmokedGlass,
        border = BorderStroke(1.dp, GlassBorder),
      ) {
        Column(
          modifier = Modifier.padding(14.dp),
          verticalArrangement = Arrangement.spacedBy(10.dp),
        ) {
          // Row 1: Camera Presets + Livery Swatches
          Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically,
          ) {
            Row(
              modifier = Modifier.horizontalScroll(rememberScrollState()),
              horizontalArrangement = Arrangement.spacedBy(6.dp),
            ) {
              ShowcaseCamera.entries.forEach { cam ->
                val selected = state.showcaseCamera == cam
                PillChip(
                  label = cam.label,
                  selected = selected,
                  accent = ApexCyan,
                  tag = "cam_${cam.name.lowercase()}",
                  onClick = { onSelectCamera(cam) },
                )
              }
            }

            Spacer(modifier = Modifier.width(8.dp))

            // Car Paint Liveries
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
              LiveryOption.entries.forEach { liv ->
                val active = state.livery == liv
                Box(
                  modifier = Modifier
                    .size(26.dp)
                    .testTag("livery_${liv.name.lowercase()}")
                    .clip(CircleShape)
                    .background(liv.swatch)
                    .border(
                      width = if (active) 2.5.dp else 1.dp,
                      color = if (active) Color.White else GlassBorder,
                      shape = CircleShape,
                    )
                    .clickable { onSelectLivery(liv) },
                )
              }
            }
          }

          // Row 2: Circuit Track Selector
          Row(
            modifier = Modifier
              .fillMaxWidth()
              .horizontalScroll(rememberScrollState()),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
          ) {
            TrackOption.entries.forEach { trk ->
              val selected = state.track == trk
              PillChip(
                label = trk.title,
                selected = selected,
                accent = ApexCrimson,
                tag = "track_${trk.name.lowercase()}",
                onClick = { onSelectTrack(trk) },
              )
            }
          }

          // Row 3: Weather (Dry / Wet / Rain Shower) & Time of Day
          Row(
            modifier = Modifier
              .fillMaxWidth()
              .horizontalScroll(rememberScrollState()),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
          ) {
            WeatherOption.entries.forEach { w ->
              PillChip(
                label = w.label,
                selected = state.weather == w,
                accent = ApexCyan,
                tag = "weather_${w.name.lowercase()}",
                onClick = { onSelectWeather(w) },
              )
            }
            Box(
              modifier = Modifier
                .width(1.dp)
                .height(24.dp)
                .background(GlassBorder)
                .align(Alignment.CenterVertically),
            )
            TimeOfDayOption.entries.forEach { tod ->
              PillChip(
                label = tod.label,
                selected = state.timeOfDay == tod,
                accent = Color(0xFFFFB300),
                tag = "tod_${tod.name.lowercase()}",
                onClick = { onSelectTimeOfDay(tod) },
              )
            }
          }
        }
      }
    }
  }
}

@Composable
private fun PillChip(
  label: String,
  selected: Boolean,
  accent: Color,
  tag: String,
  onClick: () -> Unit,
) {
  Surface(
    modifier = Modifier
      .testTag(tag)
      .height(34.dp)
      .clickable(onClick = onClick),
    shape = RoundedCornerShape(999.dp),
    color = if (selected) accent.copy(alpha = 0.24f) else Color(0x18FFFFFF),
    border = BorderStroke(1.dp, if (selected) accent else GlassBorder),
  ) {
    Box(
      modifier = Modifier.padding(horizontal = 12.dp),
      contentAlignment = Alignment.Center,
    ) {
      Text(
        text = label,
        fontFamily = RajdhaniFamily,
        fontSize = 13.sp,
        color = if (selected) TextPrimary else TextSecondary,
      )
    }
  }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun GraphicsPipelineSheet(
  graphics: GraphicsConfig,
  fps: Float,
  onUpdate: ((GraphicsConfig) -> GraphicsConfig) -> Unit,
  onDismiss: () -> Unit,
) {
  ModalBottomSheet(
    onDismissRequest = onDismiss,
    containerColor = CarbonSurface,
    contentColor = TextPrimary,
  ) {
    Column(
      modifier = Modifier
        .fillMaxWidth()
        .padding(horizontal = 20.dp, vertical = 8.dp),
      verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
      Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically,
      ) {
        Column {
          Text(
            text = "C++ GLSL ES 3.0 PIPELINE",
            fontFamily = OrbitronFamily,
            fontSize = 15.sp,
            color = ApexCyan,
          )
          Text(
            text = "Cook-Torrance PBR • Clearcoat • ACES Filmic",
            fontFamily = JetBrainsMonoFamily,
            fontSize = 11.sp,
            color = TextSecondary,
          )
        }
        Text(
          text = "${fps.roundToInt()} FPS",
          fontFamily = OrbitronFamily,
          fontSize = 15.sp,
          color = TelemetryEmerald,
        )
      }

      PipelineToggleRow(
        title = "Wet Asphalt & Clearcoat Reflections",
        subtitle = "Analytical sky + specular clearcoat & puddle mirror",
        checked = graphics.reflectionsEnabled,
        onCheckedChange = { v -> onUpdate { it.copy(reflectionsEnabled = v) } },
      )
      PipelineToggleRow(
        title = "PCF Directional Shadow Mapping",
        subtitle = "1024x1024 depth shadow pass + contact AO",
        checked = graphics.shadowsEnabled,
        onCheckedChange = { v -> onUpdate { it.copy(shadowsEnabled = v) } },
      )
      PipelineToggleRow(
        title = "HDR Gaussian Bloom",
        subtitle = "Quarter-res bright-pass + dual-pass blur",
        checked = graphics.bloomEnabled,
        onCheckedChange = { v -> onUpdate { it.copy(bloomEnabled = v) } },
      )
      PipelineToggleRow(
        title = "Volumetric Rain & Water Spray",
        subtitle = "Dynamic particle streaks & tire spray",
        checked = graphics.rainParticlesEnabled,
        onCheckedChange = { v -> onUpdate { it.copy(rainParticlesEnabled = v) } },
      )

      Column {
        Row(
          modifier = Modifier.fillMaxWidth(),
          horizontalArrangement = Arrangement.SpaceBetween,
        ) {
          Text(
            text = "Filmic Exposure",
            style = MaterialTheme.typography.titleMedium,
            color = TextPrimary,
          )
          Text(
            text = "%.2f EV".format(graphics.exposure),
            fontFamily = JetBrainsMonoFamily,
            fontSize = 12.sp,
            color = ApexCyan,
          )
        }
        Slider(
          value = graphics.exposure,
          onValueChange = { v -> onUpdate { it.copy(exposure = v) } },
          valueRange = 0.65f..1.90f,
          colors = SliderDefaults.colors(
            thumbColor = ApexCyan,
            activeTrackColor = ApexCyan,
          ),
        )
      }

      Spacer(modifier = Modifier.height(14.dp))
    }
  }
}

@Composable
private fun PipelineToggleRow(
  title: String,
  subtitle: String,
  checked: Boolean,
  onCheckedChange: (Boolean) -> Unit,
) {
  Row(
    modifier = Modifier.fillMaxWidth(),
    horizontalArrangement = Arrangement.SpaceBetween,
    verticalAlignment = Alignment.CenterVertically,
  ) {
    Column(modifier = Modifier.weight(1f).padding(end = 12.dp)) {
      Text(
        text = title,
        style = MaterialTheme.typography.titleMedium,
        color = TextPrimary,
      )
      Text(
        text = subtitle,
        style = MaterialTheme.typography.labelSmall,
        color = TextSecondary,
      )
    }
    Switch(
      checked = checked,
      onCheckedChange = onCheckedChange,
      colors = SwitchDefaults.colors(
        checkedThumbColor = Color.White,
        checkedTrackColor = ApexCyan,
      ),
    )
  }
}
