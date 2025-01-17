
#ifndef BASE_COLOR_H
#define BASE_COLOR_H

#include "vmath.h"

/*
	Title: Color handling
*/

/*
	Function: HueToRgb
		Converts Hue to RGB
*/
inline float HueToRgb(float v1, float v2, float h)
{
	if (h < 0.0f)
		h += 1;
	if (h > 1.0f)
		h -= 1;
	if ((6.0f * h) < 1.0f)
		return v1 + (v2 - v1) * 6.0f * h;
	if ((2.0f * h) < 1.0f)
		return v2;
	if ((3.0f * h) < 2.0f)
		return v1 + (v2 - v1) * ((2.0f / 3.0f) - h) * 6.0f;
	return v1;
}

/*
	Function: HslToRgb
		Converts HSL to RGB
*/
inline vec3 HslToRgb(vec3 HSL)
{
	if (HSL.s == 0.0f)
		return vec3(HSL.l, HSL.l, HSL.l);
	else
	{
		const float v2 = HSL.l < 0.5f ? HSL.l * (1.0f + HSL.s) : (HSL.l + HSL.s) - (HSL.s * HSL.l);
		const float v1 = 2.0f * HSL.l - v2;

		return vec3(HueToRgb(v1, v2, HSL.h + (1.0f / 3.0f)), HueToRgb(v1, v2, HSL.h), HueToRgb(v1, v2, HSL.h - (1.0f / 3.0f)));
	}
}

/*
	Function: HexToRgba
		Converts Hex to Rgba

	Remarks: Hex should be RGBA8
*/
inline vec4 HexToRgba(int hex)
{
	vec4 c;
	c.r = ((hex >> 24) & 0xFF) / 255.0f;
	c.g = ((hex >> 16) & 0xFF) / 255.0f;
	c.b = ((hex >> 8) & 0xFF) / 255.0f;
	c.a = (hex & 0xFF) / 255.0f;
	return c;
}

/*
	Function: HsvToRgb
		Converts Hsv to Rgb
*/
inline vec3 HsvToRgb(vec3 hsv)
{
	const int h = static_cast<int>(hsv.x * 6.0f);
	const float f = hsv.x * 6.0f - h;
	const float p = hsv.z * (1.0f - hsv.y);
	const float q = hsv.z * (1.0f - hsv.y * f);
	const float t = hsv.z * (1.0f - hsv.y * (1.0f - f));

	vec3 rgb = vec3(0.0f, 0.0f, 0.0f);

	switch (h % 6)
	{
	case 0:
		rgb.r = hsv.z;
		rgb.g = t;
		rgb.b = p;
		break;

	case 1:
		rgb.r = q;
		rgb.g = hsv.z;
		rgb.b = p;
		break;

	case 2:
		rgb.r = p;
		rgb.g = hsv.z;
		rgb.b = t;
		break;

	case 3:
		rgb.r = p;
		rgb.g = q;
		rgb.b = hsv.z;
		break;

	case 4:
		rgb.r = t;
		rgb.g = p;
		rgb.b = hsv.z;
		break;

	case 5:
		rgb.r = hsv.z;
		rgb.g = p;
		rgb.b = q;
		break;
	default:
		break;
	}

	return rgb;
}

/*
	Function: RgbToHsv
		Converts Rgb to Hsv
*/
inline vec3 RgbToHsv(vec3 rgb)
{
	const float h_min = min(min(rgb.r, rgb.g), rgb.b);
	const float h_max = max(max(rgb.r, rgb.g), rgb.b);

	// hue
	float hue;
	if (h_max == h_min)
		hue = 0.0f;
	else if (h_max == rgb.r)
		hue = (rgb.g - rgb.b) / (h_max - h_min);
	else if (h_max == rgb.g)
		hue = 2.0f + (rgb.b - rgb.r) / (h_max - h_min);
	else
		hue = 4.0f + (rgb.r - rgb.g) / (h_max - h_min);

	hue /= 6.0f;

	if (hue < 0.0f)
		hue += 1.0f;

	// saturation
	float s = 0.0f;
	if (h_max != 0.0f)
		s = (h_max - h_min) / h_max;

	// value
	const float v = h_max;

	return vec3(hue, s, v);
}

inline vec3 RgbToLab(vec3 rgb)
{
	const vec3 adapt(0.950467f, 1, 1.088969f);
	const vec3 xyz(
		0.412424f * rgb.r + 0.357579f * rgb.g + 0.180464f * rgb.b,
		0.212656f * rgb.r + 0.715158f * rgb.g + 0.0721856f * rgb.b,
		0.0193324f * rgb.r + 0.119193f * rgb.g + 0.950444f * rgb.b);

#define RGB_TO_LAB_H(VAL) ((VAL > 0.008856f) ? powf(VAL, 0.333333f) : (7.787f * VAL + 0.137931f))

	return vec3(
		116 * RGB_TO_LAB_H(xyz.y / adapt.y) - 16,
		500 * (RGB_TO_LAB_H(xyz.x / adapt.x) - RGB_TO_LAB_H(xyz.y / adapt.y)),
		200 * (RGB_TO_LAB_H(xyz.y / adapt.y) - RGB_TO_LAB_H(xyz.z / adapt.z)));
#undef RGB_TO_LAB_H
}

inline float LabDistance(vec3 labA, vec3 labB)
{
	const float ld = labA.x - labB.x;
	const float ad = labA.y - labB.y;
	const float bd = labA.z - labB.z;
	return sqrtf(ld * ld + ad * ad + bd * bd);
}

// Reset color
#define COLOR_RESET "\033[0m"

// Basic colors
#define COLOR_BLACK "\033[30m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"

// Bright colors
#define COLOR_BLACK_BRIGHT "\033[90m"
#define COLOR_RED_BRIGHT "\033[91m"
#define COLOR_GREEN_BRIGHT "\033[92m"
#define COLOR_YELLOW_BRIGHT "\033[93m"
#define COLOR_BLUE_BRIGHT "\033[94m"
#define COLOR_MAGENTA_BRIGHT "\033[95m"
#define COLOR_CYAN_BRIGHT "\033[96m"
#define COLOR_WHITE_BRIGHT "\033[97m"

// Background colors
#define COLOR_BLACK_BG "\033[40m"
#define COLOR_RED_BG "\033[41m"
#define COLOR_GREEN_BG "\033[42m"
#define COLOR_YELLOW_BG "\033[43m"
#define COLOR_BLUE_BG "\033[44m"
#define COLOR_MAGENTA_BG "\033[45m"
#define COLOR_CYAN_BG "\033[46m"
#define COLOR_WHITE_BG "\033[47m"

// Bright background colors
#define COLOR_BLACK_BG_BRIGHT "\033[100m"
#define COLOR_RED_BG_BRIGHT "\033[101m"
#define COLOR_GREEN_BG_BRIGHT "\033[102m"
#define COLOR_YELLOW_BG_BRIGHT "\033[103m"
#define COLOR_BLUE_BG_BRIGHT "\033[104m"
#define COLOR_MAGENTA_BG_BRIGHT "\033[105m"
#define COLOR_CYAN_BG_BRIGHT "\033[106m"
#define COLOR_WHITE_BG_BRIGHT "\033[107m"

#endif
