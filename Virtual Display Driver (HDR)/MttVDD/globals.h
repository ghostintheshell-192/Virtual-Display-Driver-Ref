#pragma once
#include <IddCx.h>
#include <string>


/* DriverSettings(una istanza globale : g_settings)
├── LogSettings(logs, debug, send_through_pipe)
├── EdidSettings(custom_edid, prevent_spoof, cea_override,
│ integration_enabled, auto_configure, profile_path, ...)
├── CursorSettings(hardware_cursor, alpha_support, max_x, max_y, xor_level)
├── ColourSettings(hdr_plus, sdr10, format, primaries, color_space,
│ gamma, force_bit_depth, fp16, wide_gamut, tone_mapping, ...)
├── HdrSettings(static_metadata_enabled, luminance, content_light_levels)
├── AutoResSettings(enabled, source_priority, min / max refresh, min / max resolution, ...)
└── MonitorEmulSettings(enabled, physical_dimensions, manufacturer, model, serial) */

struct LogSettings
{
	bool enable_standard_logs = false;
	bool enable_debug_logs = false;
	bool send_logs_through_pipe = true;
};

struct CursorSettings
{
	bool hardware_cursor = false;
	bool alpha_cursor_support = true;

	int max_x = 128;
	int max_y = 128;

	IDDCX_XOR_CURSOR_SUPPORT xor_cursor_support_level = IDDCX_XOR_CURSOR_SUPPORT_FULL;
};

struct EdidSettings
{
	std::wstring profile_path = L"EDID/monitor_profile.xml";

	bool enabled = false;
	bool preferred = false;
	bool auto_configure = false;
	bool custom_edid = false;
	bool prevent_manufacturer_spoof = false;
	bool edid_cea_override = false;
	bool override_manual_settings = false;
	bool fallback_on_error = true;
};

struct ColorSettings
{
	std::wstring force_bit_depth = L"auto";
	std::wstring primary_color_space = L"sRGB";
	std::wstring color_format = L"RGB";

	bool color_space_enabled = false;
	bool hdr_plus = false;
	bool sdr10 = false;
	bool auto_select_from_color_space = false;
	bool fp16_surface_support = true;
	bool wide_color_gamut = false;
	bool hdr_tone_mapping = false;

	bool primaries_enabled = false;

	struct primaries_defaults
	{
		double redX = 0.708;
		double redY = 0.292;
		double greenX = 0.170;
		double greenY = 0.797;
		double blueX = 0.131;
		double blueY = 0.046;
		double whiteX = 0.3127;
		double whiteY = 0.3290;
	} defaults;

	double sdr_white_level = 80.0;
	double gamma_correction = 2.4;

	IDDCX_BITS_PER_COMPONENT SDR_COLOR = IDDCX_BITS_PER_COMPONENT_8;
	IDDCX_BITS_PER_COMPONENT HDR_COLOR = IDDCX_BITS_PER_COMPONENT_10;

};

struct HdrSettings
{
	bool static_metadata_enabled = false;
	bool matrix_transform_enabled = false;

	int max_content_light_level = 1000;
	int max_frame_avg_light_level = 400;

	double max_display_mastering_luminance = 1000.0; //standard SMPTE ST.2086 - UoM nits (candle over squared meter) - indicates how luminous a display is, even when it is virtual.
	double min_display_mastering_luminance = 0.05;
};

struct AutoResolutionSettings
{
	std::wstring source_priority = L"manual";

	bool enabled = false;
	bool exclude_fractional_rates = false;

	int min_refresh_rate = 24;
	int max_refresh_rate = 240;
	
	int min_resolution_width = 640;
	int min_resolution_height = 480;
	int max_resolution_width = 7680;
	int max_resolution_height = 4320;

	int fallback_width = 1920;
	int fallback_height = 1080;
	int fallback_refresh = 60;
};

struct DriverSettings
{
	AutoResolutionSettings auto_res;
	HdrSettings hdr;
	ColorSettings colors;
	EdidSettings edid;
	CursorSettings cursor;
	LogSettings logs;
};