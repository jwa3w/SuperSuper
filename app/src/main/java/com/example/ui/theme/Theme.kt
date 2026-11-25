package com.example.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable

private val ApexDarkColorScheme = darkColorScheme(
  primary = ApexCrimson,
  onPrimary = TextPrimary,
  secondary = ApexCyan,
  onSecondary = ObsidianBg,
  tertiary = TelemetryAmber,
  background = ObsidianBg,
  onBackground = TextPrimary,
  surface = CarbonSurface,
  onSurface = TextPrimary,
  surfaceVariant = CarbonSurfaceVariant,
  onSurfaceVariant = TextSecondary,
  outline = GlassBorder,
)

@Composable
fun ApexGtTheme(
  content: @Composable () -> Unit,
) {
  MaterialTheme(
    colorScheme = ApexDarkColorScheme,
    typography = Typography,
    content = content,
  )
}
