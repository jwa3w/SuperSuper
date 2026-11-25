package com.example.ui.theme

import androidx.compose.material3.Typography
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.Font
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp
import com.example.R

val OrbitronFamily = FontFamily(Font(R.font.orbitron, FontWeight.Bold))
val RajdhaniFamily = FontFamily(Font(R.font.rajdhani, FontWeight.SemiBold))
val JetBrainsMonoFamily = FontFamily(Font(R.font.jetbrains_mono, FontWeight.Medium))

val Typography = Typography(
  displayLarge = TextStyle(
    fontFamily = OrbitronFamily,
    fontWeight = FontWeight.Bold,
    fontSize = 36.sp,
    lineHeight = 40.sp,
    letterSpacing = 1.sp,
  ),
  headlineMedium = TextStyle(
    fontFamily = OrbitronFamily,
    fontWeight = FontWeight.Bold,
    fontSize = 20.sp,
    lineHeight = 24.sp,
    letterSpacing = 0.8.sp,
  ),
  titleMedium = TextStyle(
    fontFamily = RajdhaniFamily,
    fontWeight = FontWeight.Bold,
    fontSize = 16.sp,
    lineHeight = 20.sp,
    letterSpacing = 0.6.sp,
  ),
  bodyLarge = TextStyle(
    fontFamily = RajdhaniFamily,
    fontWeight = FontWeight.Medium,
    fontSize = 15.sp,
    lineHeight = 20.sp,
  ),
  labelMedium = TextStyle(
    fontFamily = JetBrainsMonoFamily,
    fontWeight = FontWeight.Medium,
    fontSize = 12.sp,
    lineHeight = 16.sp,
    letterSpacing = 0.4.sp,
  ),
  labelSmall = TextStyle(
    fontFamily = JetBrainsMonoFamily,
    fontWeight = FontWeight.Medium,
    fontSize = 10.sp,
    lineHeight = 14.sp,
    letterSpacing = 0.5.sp,
  ),
)
