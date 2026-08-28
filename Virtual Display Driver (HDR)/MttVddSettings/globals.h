#pragma once
//#include <IddCx.h>
#include <string>

/* DriverSettings(una istanza globale : g_settings)
├── LogSettings(logs, debug, send_through_pipe)
├── EdidSettings(custom_edid, prevent_spoof, cea_override)
├── EdidIntegrationSettings(integration_enabled, auto_configure, profile_path, ...)
├── CursorSettings(hardware_cursor, alpha_support, max_x, max_y, xor_level)
├── ColourSettings(hdr_plus, sdr10, format)
├── HdrAdvancedSettings(static_metadata_enabled, luminance, content_light_levels, primaries, color_space,
| gamma, force_bit_depth, fp16, wide_gamut, tone_mapping, ...)
├── AutoResolutionSettings(enabled, source_priority, min / max refresh, min / max resolution, ...)
└── MonitorEmulationSettings(enabled, physical_dimensions, manufacturer, model, serial) */


namespace Refactoring
{
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

	// IDDCX_XOR_CURSOR_SUPPORT xor_cursor_support_level = IDDCX_XOR_CURSOR_SUPPORT_FULL;
};

struct EdidSettings
{
	bool custom_edid = false;
	bool prevent_manufacturer_spoof = false;
	bool edid_cea_override = false;
};

struct EdidIntegrationSettings
{
	std::string profile_path = "EDID/monitor_profile.xm";
	bool override_manual_settings = false;
	bool fallback_on_error = true;
	bool enabled = false;
	bool auto_configure = false;
};

struct ColourSettings
{
	bool hdr_plus = false;
	bool sdr10 = false;
	std::string color_format = "RGB";

	// IDDCX_BITS_PER_COMPONENT SDR_COLOR = IDDCX_BITS_PER_COMPONENT_8;
	// IDDCX_BITS_PER_COMPONENT HDR_COLOR = IDDCX_BITS_PER_COMPONENT_10;
};

struct BitDepthManagementSettings
{
	bool auto_select_from_color_space = false;
	bool fp16_surface_support = true;

	std::string force_bit_depth = "auto";
};

struct ColorFormatExtendedSettings
{
	bool wide_color_gamut = false; // not used
	bool hdr_tone_mapping = false; // not used
	double sdr_white_level = 80.0;
};

struct ColorAdvancedSettings
{
	BitDepthManagementSettings bit_depth_management;
	ColorFormatExtendedSettings color_format_extended;
};

struct ColorPrimariesSettings
{
	bool primaries_enabled = false;
	double redX = 0.708;
	double redY = 0.292;
	double greenX = 0.170;
	double greenY = 0.797;
	double blueX = 0.131;
	double blueY = 0.046;
	double whiteX = 0.3127;
	double whiteY = 0.3290;
};

struct ColorSpaceSettings
{
	bool enabled = false;
	bool enable_matrix_transform = false;
	double gamma_correction = 2.4;

	std::string primary_color_space = "sRGB";
};

struct HdrAdvancedSettings
{
	bool static_metadata_enabled = false;

	int max_content_light_level = 1000;
	int max_frame_avg_light_level = 400;

	double max_display_mastering_luminance = 1000.0; // standard SMPTE ST.2086 - UoM nits (candle over squared meter) -
													 // indicates how luminous a display is, even when it is virtual.
	double min_display_mastering_luminance = 0.05;

	ColorPrimariesSettings color_primaries;
	ColorSpaceSettings color_space;
};

struct EdidModeFilteringSettings
{
	bool exclude_fractional_rates = false;

	int min_refresh_rate = 24;
	int max_refresh_rate = 240;

	int min_resolution_width = 640;
	int min_resolution_height = 480;
	int max_resolution_width = 7680;
	int max_resolution_height = 4320;
};

struct PreferredModeSettings
{
	bool preferred = false;
	int fallback_width = 1920;
	int fallback_height = 1080;
	int fallback_refresh = 60;
};

struct AutoResolutionSettings
{
	std::string source_priority = "manual";
	bool enabled = false;
	EdidModeFilteringSettings edid_mode_filtering;
	PreferredModeSettings preferred_mode;
};

struct MonitorEmulationSettings
{
	std::string manufacturer_name = "Generic";
	std::string model_name = "Virtual Display";
	std::string serial_number = "VDD001";

	bool enabled = false;
	bool emulate_physical_dimensions = false;
	bool manufacturer_emulation_enabled = false;

	int physical_width = 510;  // UoM: millimeters
	int physical_height = 287; // UoM: millimeters
};

struct DriverSettings
{
	MonitorEmulationSettings monitor_emulation;
	AutoResolutionSettings auto_resolutions;
	HdrAdvancedSettings hdr_advanced;
	ColourSettings colours;
	ColorAdvancedSettings color_advanced;
	EdidSettings edid;
	EdidIntegrationSettings edid_integration;
	CursorSettings cursor;
	LogSettings logs;
};
}