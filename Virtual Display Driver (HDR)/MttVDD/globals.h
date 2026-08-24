#pragma once


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

struct DriverSettings
{
	LogSettings logs;
};