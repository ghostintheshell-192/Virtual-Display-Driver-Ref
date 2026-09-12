#pragma once
#include <string>
#include <variant>
#include <vector>

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

typedef std::variant<bool *, int *, double *, std::string *> SettingValuePtr;

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

	int xor_cursor_support_level = 2; //IDDCX_XOR_CURSOR_SUPPORT_UNINITIALIZED = 0, IDDCX_XOR_CURSOR_SUPPORT_NONE = 1, IDDCX_XOR_CURSOR_SUPPORT_FULL = 2, IDDCX_XOR_CURSOR_SUPPORT_EMULATION = 3 - see IddCx.h
};

struct EdidSettings
{
	bool custom_edid = false;
	bool prevent_manufacturer_spoof = false;
	bool edid_cea_override = false;
};

struct EdidIntegrationSettings
{
	std::string profile_path = "EDID/monitor_profile.xml";
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

struct DataElements
{
	std::string key;
	SettingValuePtr container;
};

struct Resolution
{
	int width = 1920;
	int height = 1080;
	int refresh_num = 6000;
	int refresh_den = 100;

	Resolution() = default;
	Resolution(int w, int h, int num, int den) : width(w), height(h), refresh_num(num), refresh_den(den) {};
};

struct ColorMatrix
{
	FLOAT matrix[3][4];
	bool isValid = false;

	ColorMatrix()
	{
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				matrix[i][j] = (i == j) ? 1.0f : 0.0f;
			}
		}
	}
};

struct MonitorProfile
{
	bool hdr10_plus_supported = false;
	bool hdr10_supported = false;
	bool dolby_vision_supported = false;

	//hdr advanced
	double max_mastering_luminance = 0.0;
	double min_mastering_luminance = 0.0;
	double gamma_correction = 2.2;

	//monitor resolution
	Resolution preferred_res{};

	std::vector<Resolution> modes;

	//color_space
	std::string primary_color_space = "sRGB"; //sRGB, DCI-P3, etc

	struct ColorPrimaries
	{
		double redX = 0.64;
		double redY = 0.33;
		double greenX = 0.30;
		double greenY = 0.60;
		double blueX = 0.15;
		double blueY = 0.06;
		double whiteX = 0.3127;
		double whiteY = 0.3290;
	} primaries;

	ColorMatrix Get_sRGB()
	{
		ColorMatrix t_matrix;
		t_matrix[0][0] = gamma_correction / 2.2f; // Red
		t_matrix[1][1] = gamma_correction / 2.2f; // Green
		t_matrix[2][2] = gamma_correction / 2.2f; // Blue

		return t_matrix;
	}
	ColorMatrix Get_DCI_P3()
	{
		ColorMatrix t_matrix;
		t_matrix[0][0] = 1.2249f * (gamma_correction / 2.4f);
		t_matrix[0][1] = -0.2247f;
		t_matrix[0][2] = 0.0f;
		t_matrix[1][0] = -0.0420f;
		t_matrix[1][1] = 1.0419f * (gamma_correction / 2.4f);
		t_matrix[1][2] = 0.0f;
		t_matrix[2][0] = -0.0196f;
		t_matrix[2][1] = -0.0786f;
		t_matrix[2][2] = 1.0982f * (gamma_correction / 2.4f);
	}
	ColorMatrix Get_REC_2020()
	{
		ColorMatrix t_matrix;
		t_matrix[0][0] = 1.7347f * (gamma_correction / 2.4f);
		t_matrix[0][1] = -0.7347f;
		t_matrix[0][2] = 0.0f;
		t_matrix[1][0] = -0.1316f;
		t_matrix[1][1] = 1.1316f * (gamma_correction / 2.4f);
		t_matrix[1][2] = 0.0f;
		t_matrix[2][0] = -0.0241f;
		t_matrix[2][1] = -0.1289f;
		t_matrix[2][2] = 1.1530f * (gamma_correction / 2.4f);
		return t_matrix;
	}
	ColorMatrix Get_Adobe_RGB()
	{
		ColorMatrix t_matrix;
		t_matrix[0][0] = 1.0f * (gamma_correction / 2.2f);
		t_matrix[1][1] = 1.0f * (gamma_correction / 2.2f);
		t_matrix[2][2] = 1.0f * (gamma_correction / 2.2f);
		return t_matrix;
	};
};

} // namespace Refactoring