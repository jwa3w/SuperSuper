package com.example.ui.components

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
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
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.engine.TelemetryState
import com.example.ui.theme.ApexCrimson
import com.example.ui.theme.ApexCyan
import com.example.ui.theme.GlassBorder
import com.example.ui.theme.JetBrainsMonoFamily
import com.example.ui.theme.OrbitronFamily
import com.example.ui.theme.SmokedGlass
import com.example.ui.theme.TelemetryAmber
import com.example.ui.theme.TelemetryEmerald
import com.example.ui.theme.TextPrimary
import com.example.ui.theme.TextSecondary
import kotlin.math.abs
import kotlin.math.roundToInt

fun formatLapTime(seconds: Float): String {
  if (seconds <= 0.01f) return "--:--.--"
  val mins = (seconds / 60f).toInt()
  val secs = (seconds % 60f).toInt()
  val hundredths = ((seconds * 100f).toInt()) % 100
  return "%02d:%02d.%02d".format(mins, secs, hundredths)
}

@Composable
fun TrackMinimapRadar(
  points: List<Pair<Float, Float>>,
  trackProgress: Float,
  modifier: Modifier = Modifier,
) {
  Surface(
    modifier = modifier.testTag("track_minimap"),
    shape = RoundedCornerShape(14.dp),
    color = SmokedGlass,
    border = BorderStroke(1.dp, GlassBorder),
  ) {
    Box(
      modifier = Modifier
        .size(92.dp)
        .padding(10.dp),
      contentAlignment = Alignment.Center,
    ) {
      Canvas(modifier = Modifier.matchParentSize()) {
        if (points.size < 3) return@Canvas
        val w = size.width
        val h = size.height
        val path = Path()
        points.forEachIndexed { idx, (nx, ny) ->
          val px = nx * w
          val py = ny * h
          if (idx == 0) path.moveTo(px, py) else path.lineTo(px, py)
        }
        path.close()

        // Outer track shadow
        drawPath(
          path = path,
          color = Color(0x66000000),
          style = Stroke(width = 6f, cap = StrokeCap.Round, join = StrokeJoin.Round),
        )
        // Track ribbon
        drawPath(
          path = path,
          color = Color(0xCCFFFFFF),
          style = Stroke(width = 3.2f, cap = StrokeCap.Round, join = StrokeJoin.Round),
        )

        // Start/Finish marker
        val startPt = Offset(points[0].first * w, points[0].second * h)
        drawCircle(color = ApexCrimson, radius = 4.5f, center = startPt)

        // Live car position on spline
        val wrapped = ((trackProgress % 1f) + 1f) % 1f
        val fIdx = wrapped * points.size
        val i0 = fIdx.toInt() % points.size
        val i1 = (i0 + 1) % points.size
        val frac = fIdx - fIdx.toInt()
        val cx = (points[i0].first + (points[i1].first - points[i0].first) * frac) * w
        val cy = (points[i0].second + (points[i1].second - points[i0].second) * frac) * h
        val carPt = Offset(cx, cy)

        drawCircle(color = ApexCyan.copy(alpha = 0.38f), radius = 10f, center = carPt)
        drawCircle(color = ApexCyan, radius = 5.2f, center = carPt)
      }
    }
  }
}

@Composable
fun DigitalTelemetryCluster(
  telemetry: TelemetryState,
  modifier: Modifier = Modifier,
) {
  Surface(
    modifier = modifier.testTag("telemetry_cluster"),
    shape = RoundedCornerShape(16.dp),
    color = SmokedGlass,
    border = BorderStroke(1.dp, GlassBorder),
  ) {
    Column(
      modifier = Modifier.padding(horizontal = 14.dp, vertical = 8.dp),
      horizontalAlignment = Alignment.CenterHorizontally,
    ) {
      // LED RPM Shift Bar (12 segments)
      val rpmNorm = ((telemetry.rpm - 1200f) / 7700f).coerceIn(0f, 1f)
      val litCount = (rpmNorm * 12f).roundToInt()
      Row(
        horizontalArrangement = Arrangement.spacedBy(3.dp),
        verticalAlignment = Alignment.CenterVertically,
      ) {
        for (i in 0 until 12) {
          val lit = i < litCount
          val segColor = when {
            !lit -> Color(0x28FFFFFF)
            i < 6 -> TelemetryEmerald
            i < 9 -> TelemetryAmber
            else -> ApexCrimson
          }
          Box(
            modifier = Modifier
              .width(12.dp)
              .height(5.dp)
              .clip(RoundedCornerShape(2.dp))
              .background(segColor),
          )
        }
      }

      Spacer(modifier = Modifier.height(6.dp))

      Row(
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(14.dp),
      ) {
        // Gear Pill
        Box(
          modifier = Modifier
            .size(38.dp)
            .clip(RoundedCornerShape(10.dp))
            .background(
              Brush.verticalGradient(
                listOf(ApexCrimson.copy(alpha = 0.35f), ApexCrimson.copy(alpha = 0.14f))
              )
            )
            .border(1.dp, ApexCrimson.copy(alpha = 0.7f), RoundedCornerShape(10.dp)),
          contentAlignment = Alignment.Center,
        ) {
          val gearText = if (telemetry.gear == 0) "R" else telemetry.gear.toString()
          Text(
            text = gearText,
            fontFamily = OrbitronFamily,
            fontSize = 18.sp,
            color = TextPrimary,
          )
        }

        // Digital Speedometer
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
          Text(
            text = "%03d".format(telemetry.speedKmh.roundToInt().coerceIn(0, 999)),
            fontFamily = OrbitronFamily,
            fontSize = 26.sp,
            color = if (telemetry.nitroActive) ApexCyan else TextPrimary,
          )
          Text(
            text = "KM/H  •  ${telemetry.rpm.roundToInt()} RPM",
            fontFamily = JetBrainsMonoFamily,
            fontSize = 9.sp,
            color = TextSecondary,
          )
        }

        // Nitro Gauge
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
          Text(
            text = "N2O",
            fontFamily = JetBrainsMonoFamily,
            fontSize = 9.sp,
            color = if (telemetry.nitroActive) ApexCyan else TextSecondary,
          )
          Spacer(modifier = Modifier.height(3.dp))
          Box(
            modifier = Modifier
              .width(42.dp)
              .height(6.dp)
              .clip(RoundedCornerShape(3.dp))
              .background(Color(0x33FFFFFF)),
          ) {
            Box(
              modifier = Modifier
                .fillMaxWidth(telemetry.nitroAmount.coerceIn(0f, 1f))
                .height(6.dp)
                .clip(RoundedCornerShape(3.dp))
                .background(if (telemetry.nitroActive) Color.White else ApexCyan),
            )
          }
          Spacer(modifier = Modifier.height(2.dp))
          Text(
            text = "${"%.1f".format(abs(telemetry.lateralG))}G",
            fontFamily = JetBrainsMonoFamily,
            fontSize = 9.sp,
            color = TelemetryAmber,
          )
        }
      }
    }
  }
}

@Composable
fun LapTimingBadge(
  telemetry: TelemetryState,
  modifier: Modifier = Modifier,
) {
  Surface(
    modifier = modifier.testTag("lap_timing_badge"),
    shape = RoundedCornerShape(14.dp),
    color = SmokedGlass,
    border = BorderStroke(1.dp, GlassBorder),
  ) {
    Column(modifier = Modifier.padding(horizontal = 12.dp, vertical = 8.dp)) {
      Row(verticalAlignment = Alignment.CenterVertically) {
        Text(
          text = "LAP ${telemetry.lapCount}",
          style = MaterialTheme.typography.labelSmall,
          color = ApexCyan,
        )
        Spacer(modifier = Modifier.width(8.dp))
        Text(
          text = "${telemetry.fps.roundToInt()} FPS",
          style = MaterialTheme.typography.labelSmall,
          color = TelemetryEmerald,
        )
      }
      Text(
        text = formatLapTime(telemetry.currentLapTime),
        fontFamily = OrbitronFamily,
        fontSize = 15.sp,
        color = TextPrimary,
      )
      Text(
        text = "BEST ${formatLapTime(telemetry.bestLapTime)}",
        style = MaterialTheme.typography.labelSmall,
        color = TextSecondary,
      )
    }
  }
}

@Composable
fun DriftIndicatorBadge(
  driftAngleDeg: Float,
  modifier: Modifier = Modifier,
) {
  val absDrift = abs(driftAngleDeg)
  AnimatedVisibility(
    visible = absDrift >= 8f,
    enter = fadeIn(),
    exit = fadeOut(),
    modifier = modifier,
  ) {
    Surface(
      shape = RoundedCornerShape(999.dp),
      color = TelemetryAmber.copy(alpha = 0.22f),
      border = BorderStroke(1.dp, TelemetryAmber),
    ) {
      Text(
        text = "DRIFT  ${absDrift.roundToInt()}°",
        fontFamily = OrbitronFamily,
        fontSize = 12.sp,
        color = TelemetryAmber,
        modifier = Modifier.padding(horizontal = 12.dp, vertical = 5.dp),
      )
    }
  }
}
