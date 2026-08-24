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
};

struct DriverSettings
{
	EdidSettings edid;
	CursorSettings cursor;
	LogSettings logs;
};