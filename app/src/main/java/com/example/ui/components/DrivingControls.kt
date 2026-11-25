package com.example.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
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
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowLeft
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.Bolt
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.rotate
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.engine.SteerControlStyle
import com.example.ui.theme.ApexCrimson
import com.example.ui.theme.ApexCyan
import com.example.ui.theme.GlassBorder
import com.example.ui.theme.OrbitronFamily
import com.example.ui.theme.RajdhaniFamily
import com.example.ui.theme.SmokedGlass
import com.example.ui.theme.TelemetryAmber
import com.example.ui.theme.TelemetryEmerald
import com.example.ui.theme.TextPrimary
import kotlin.math.atan2

@Composable
fun DrivingControlsOverlay(
  steerStyle: SteerControlStyle,
  nitroAmount: Float,
  onSteerLeft: (Boolean) -> Unit,
  onSteerRight: (Boolean) -> Unit,
  onAnalogSteer: (Float) -> Unit,
  onThrottle: (Boolean) -> Unit,
  onBrake: (Boolean) -> Unit,
  onHandbrake: (Boolean) -> Unit,
  onNitro: (Boolean) -> Unit,
  modifier: Modifier = Modifier,
) {
  Row(
    modifier = modifier
      .fillMaxWidth()
      .padding(horizontal = 16.dp, vertical = 12.dp),
    horizontalArrangement = Arrangement.SpaceBetween,
    verticalAlignment = Alignment.Bottom,
  ) {
    // Left side: Steering Controls
    if (steerStyle == SteerControlStyle.TOUCH_PADS) {
      Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
        HoldButton(
          tag = "steer_left_btn",
          accent = ApexCyan,
          modifier = Modifier.size(72.dp),
          onPressChange = onSteerLeft,
        ) {
          Icon(
            imageVector = Icons.AutoMirrored.Filled.KeyboardArrowLeft,
            contentDescription = "Steer Left",
            tint = TextPrimary,
            modifier = Modifier.size(38.dp),
          )
        }
        HoldButton(
          tag = "steer_right_btn",
          accent = ApexCyan,
          modifier = Modifier.size(72.dp),
          onPressChange = onSteerRight,
        ) {
          Icon(
            imageVector = Icons.AutoMirrored.Filled.KeyboardArrowRight,
            contentDescription = "Steer Right",
            tint = TextPrimary,
            modifier = Modifier.size(38.dp),
          )
        }
      }
    } else {
      VirtualSteeringWheel(
        onSteerChange = onAnalogSteer,
        modifier = Modifier.size(144.dp),
      )
    }

    // Right side: Nitro + Handbrake + Brake/Rev + Throttle Pedals
    Column(
      horizontalAlignment = Alignment.End,
      verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
      Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
        HoldButton(
          tag = "handbrake_btn",
          accent = TelemetryAmber,
          modifier = Modifier
            .height(52.dp)
            .width(74.dp),
          onPressChange = onHandbrake,
        ) {
          Text(
            text = "DRIFT",
            fontFamily = RajdhaniFamily,
            fontSize = 13.sp,
            color = TelemetryAmber,
          )
        }

        HoldButton(
          tag = "nitro_btn",
          accent = ApexCyan,
          modifier = Modifier
            .height(52.dp)
            .width(84.dp),
          onPressChange = onNitro,
        ) {
          Row(verticalAlignment = Alignment.CenterVertically) {
            Icon(
              imageVector = Icons.Default.Bolt,
              contentDescription = "Nitro Boost",
              tint = if (nitroAmount > 0.08f) ApexCyan else Color.Gray,
              modifier = Modifier.size(18.dp),
            )
            Spacer(modifier = Modifier.width(2.dp))
            Text(
              text = "N2O",
              fontFamily = OrbitronFamily,
              fontSize = 12.sp,
              color = TextPrimary,
            )
          }
        }
      }

      Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
        HoldButton(
          tag = "brake_pedal_btn",
          accent = ApexCrimson,
          modifier = Modifier
            .width(68.dp)
            .height(86.dp),
          onPressChange = onBrake,
        ) {
          Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Text(
              text = "BRAKE",
              fontFamily = RajdhaniFamily,
              fontSize = 13.sp,
              color = ApexCrimson,
            )
            Text(
              text = "REV",
              fontFamily = RajdhaniFamily,
              fontSize = 10.sp,
              color = TextPrimary.copy(alpha = 0.7f),
            )
          }
        }

        HoldButton(
          tag = "throttle_pedal_btn",
          accent = TelemetryEmerald,
          modifier = Modifier
            .width(78.dp)
            .height(106.dp),
          onPressChange = onThrottle,
        ) {
          Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Text(
              text = "GAS",
              fontFamily = OrbitronFamily,
              fontSize = 14.sp,
              color = TelemetryEmerald,
            )
            Spacer(modifier = Modifier.height(4.dp))
            Box(
              modifier = Modifier
                .width(28.dp)
                .height(3.dp)
                .background(TelemetryEmerald.copy(alpha = 0.7f)),
            )
          }
        }
      }
    }
  }
}

@Composable
private fun HoldButton(
  tag: String,
  accent: Color,
  modifier: Modifier = Modifier,
  onPressChange: (Boolean) -> Unit,
  content: @Composable () -> Unit,
) {
  var pressed by remember { mutableStateOf(false) }

  Box(
    modifier = modifier
      .testTag(tag)
      .clip(RoundedCornerShape(16.dp))
      .background(
        if (pressed) accent.copy(alpha = 0.34f) else SmokedGlass
      )
      .border(
        width = if (pressed) 2.dp else 1.dp,
        color = if (pressed) accent else GlassBorder,
        shape = RoundedCornerShape(16.dp),
      )
      .pointerInput(Unit) {
        awaitEachGesture {
          awaitFirstDown(requireUnconsumed = false)
          pressed = true
          onPressChange(true)
          try {
            while (true) {
              val event = awaitPointerEvent()
              if (event.changes.none { it.pressed }) break
            }
          } finally {
            pressed = false
            onPressChange(false)
          }
        }
      },
    contentAlignment = Alignment.Center,
  ) {
    content()
  }
}

@Composable
private fun VirtualSteeringWheel(
  onSteerChange: (Float) -> Unit,
  modifier: Modifier = Modifier,
) {
  var wheelDeg by remember { mutableFloatStateOf(0f) }

  Box(
    modifier = modifier
      .testTag("virtual_steering_wheel")
      .clip(CircleShape)
      .background(SmokedGlass)
      .border(1.dp, GlassBorder, CircleShape)
      .pointerInput(Unit) {
        awaitEachGesture {
          val down = awaitFirstDown(requireUnconsumed = false)
          val center = Offset(size.width / 2f, size.height / 2f)
          fun updateFrom(pos: Offset) {
            val dx = pos.x - center.x
            val dy = pos.y - center.y
            val angleDeg = atan2(dx, -dy) * (180f / Math.PI.toFloat())
            val clamped = angleDeg.coerceIn(-95f, 95f)
            wheelDeg = clamped
            onSteerChange((clamped / 75f).coerceIn(-1f, 1f))
          }
          updateFrom(down.position)
          try {
            while (true) {
              val ev = awaitPointerEvent()
              val active = ev.changes.firstOrNull { it.pressed } ?: break
              updateFrom(active.position)
            }
          } finally {
            wheelDeg = 0f
            onSteerChange(0f)
          }
        }
      },
    contentAlignment = Alignment.Center,
  ) {
    Canvas(modifier = Modifier.matchParentSize().padding(14.dp)) {
      val radius = size.minDimension / 2f
      val center = Offset(size.width / 2f, size.height / 2f)
      rotate(wheelDeg, pivot = center) {
        // Outer GT rim
        drawCircle(
          brush = Brush.sweepGradient(listOf(Color(0xFF343B4B), Color(0xFF1A1F2B), Color(0xFF343B4B))),
          radius = radius,
          center = center,
          style = Stroke(width = 16f),
        )
        // Top center racing stripe
        drawArc(
          color = ApexCrimson,
          startAngle = -100f,
          sweepAngle = 20f,
          useCenter = false,
          style = Stroke(width = 16f, cap = StrokeCap.Butt),
        )
        // Spokes
        drawLine(
          color = Color(0xFF4F596E),
          start = Offset(center.x - radius + 8f, center.y),
          end = Offset(center.x + radius - 8f, center.y),
          strokeWidth = 10f,
          cap = StrokeCap.Round,
        )
        drawLine(
          color = Color(0xFF4F596E),
          start = center,
          end = Offset(center.x, center.y + radius - 8f),
          strokeWidth = 10f,
          cap = StrokeCap.Round,
        )
        drawCircle(color = ApexCyan, radius = 14f, center = center)
      }
    }
  }
}
