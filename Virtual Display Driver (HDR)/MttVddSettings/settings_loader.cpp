#include "settings_loader.h"
#include <iostream>
#include <string>

void Refactoring::SettingsLoader::Init()
{
	conf_path = "C:\\data\\repos\\Sandbox\\Virtual-Display-Driver-Ref\\Virtual Display Driver (HDR)";
	std::cout << "Config Path is at default value: " + conf_path + "\n";

	check_registry = reg_reader.OpenRegistry();

	if (check_registry)
	{
		reg_reader.InitializePath(conf_path);
		reg_reader.CloseRegistry();
	}
}

void Refactoring::SettingsLoader::LoadSettings()
{
	if (check_registry)
	{
		reg_reader.OpenRegistry();
		OverrideDefaultsRegistry();
		reg_reader.CloseRegistry();
	}
}

void Refactoring::SettingsLoader::OverrideDefaultsRegistry()
{
	reg_reader.GetSetting<bool>({L"logging", L"logging"}, settings.logs.enable_standard_logs);
	reg_reader.GetSetting<bool>({L"logging", L"debuglogging"}, settings.logs.enable_debug_logs);
	reg_reader.GetSetting<bool>({L"logging", L"SendLogsThroughPipe"}, settings.logs.send_logs_through_pipe);

	reg_reader.GetSetting<bool>({L"edid", L"CustomEdid"}, settings.edid.custom_edid);
	reg_reader.GetSetting<bool>({L"edid", L"PreventSpoof"}, settings.edid.prevent_manufacturer_spoof);
	reg_reader.GetSetting<bool>({L"edid", L"EdidCeaOverride"}, settings.edid.edid_cea_override);

	// colour
	reg_reader.GetSetting<bool>({L"colour", L"HDRPlus"}, settings.colours.hdr_plus);
	reg_reader.GetSetting<bool>({L"colour", L"SDR10bit"}, settings.colours.sdr10);
	// g_settings.colors.HDR_COLOR = g_settings.colors.hdr_plus ? IDDCX_BITS_PER_COMPONENT_12 :
	// IDDCX_BITS_PER_COMPONENT_10; g_settings.colors.SDR_COLOR = g_settings.colors.sdr10 ? IDDCX_BITS_PER_COMPONENT_10
	// : IDDCX_BITS_PER_COMPONENT_8;
	reg_reader.GetSetting<std::wstring>({L"colour", L"ColourFormat"}, settings.colours.color_format);

	// Cursor
	reg_reader.GetSetting<bool>({L"cursor", L"HardwareCursor"}, settings.cursor.hardware_cursor);
	reg_reader.GetSetting<bool>({L"cursor", L"AlphaCursorSupport"}, settings.cursor.alpha_cursor_support);
	reg_reader.GetSetting<int>({L"cursor", L"CursorMaxX"}, settings.cursor.max_x);
	reg_reader.GetSetting<int>({L"cursor", L"CursorMaxY"}, settings.cursor.max_y);

	// int xorCursorSupportLevelInt = GetSetting<int>(hKey, L"XorCursorSupportLevel", );
	// std::string xorCursorSupportLevelName;

	// if (xorCursorSupportLevelInt < 0 || xorCursorSupportLevelInt > 3) {
	//	vddlog("w", "Selected Xor Level unsupported, defaulting to IDDCX_XOR_CURSOR_SUPPORT_FULL");
	//	g_settings.cursor.xor_cursor_support_level = IDDCX_XOR_CURSOR_SUPPORT_FULL;
	// }
	// else {
	//	g_settings.cursor.xor_cursor_support_level = static_cast<IDDCX_XOR_CURSOR_SUPPORT>(xorCursorSupportLevelInt);
	// }

	// === LOAD NEW EDID INTEGRATION SETTINGS ===
	reg_reader.GetSetting<std::wstring>({L"edid_integration", L"edid_profile_path"},
										settings.edid_integration.profile_path);
	reg_reader.GetSetting<bool>({L"edid_integration", L"enabled"}, settings.edid_integration.enabled);
	reg_reader.GetSetting<bool>({L"edid_integration", L"auto_configure_from_edid"},
								settings.edid_integration.auto_configure);
	reg_reader.GetSetting<bool>({L"edid_integration", L"override_manual_settings"},
								settings.edid_integration.override_manual_settings);
	reg_reader.GetSetting<bool>({L"edid_integration", L"fallback_on_error"},
								settings.edid_integration.fallback_on_error);

	// === LOAD HDR ADVANCED SETTINGS ===
	reg_reader.GetSetting<bool>({L"color_space", L"enable_matrix_transform"},
								settings.hdr_advanced.color_space.enable_matrix_transform);
	reg_reader.GetSetting<bool>({L"hdr10_static_metadata", L"enabled"}, settings.hdr_advanced.static_metadata_enabled);
	reg_reader.GetSetting<double>(
		{L"hdr10_static_metadata", L"max_display_mastering_luminance"},
		settings.hdr_advanced.max_display_mastering_luminance); // sta sotto hdr_advanced/hdr10_static_metadata
	reg_reader.GetSetting<double>(
		{L"hdr10_static_metadata", L"min_display_mastering_luminance"},
		settings.hdr_advanced.min_display_mastering_luminance); // sta sotto hdr_advanced/hdr10_static_metadata
	reg_reader.GetSetting<int>(
		{L"hdr10_static_metadata", L"max_content_light_level"},
		settings.hdr_advanced.max_content_light_level); // sta sotto hdr_advanced/hdr10_static_metadata
	reg_reader.GetSetting<int>(
		{L"hdr10_static_metadata", L"max_frame_avg_light_level"},
		settings.hdr_advanced.max_frame_avg_light_level); // sta sotto hdr_advanced/hdr10_static_metadata

	// === LOAD AUTO RESOLUTIONS SETTINGS ===
	reg_reader.GetSetting<std::wstring>({L"auto_resolutions", L"source_priority"},
										settings.auto_resolutions.source_priority);
	reg_reader.GetSetting<bool>({L"auto_resolutions", L"enabled"}, settings.auto_resolutions.enabled);

	reg_reader.GetSetting<bool>({L"edid_mode_filtering", L"exclude_fractional_rates"},
								settings.auto_resolutions.edid_mode_filtering
									.exclude_fractional_rates); // sta sotto auto_resolutions/edid_mode_filtering
	reg_reader.GetSetting<int>({L"edid_mode_filtering", L"min_refresh_rate"},
							   settings.auto_resolutions.edid_mode_filtering
								   .min_refresh_rate); // sta sotto auto_resolutions/edid_mode_filtering
	reg_reader.GetSetting<int>({L"edid_mode_filtering", L"max_refresh_rate"},
							   settings.auto_resolutions.edid_mode_filtering
								   .max_refresh_rate); // sta sotto auto_resolutions/edid_mode_filtering
	reg_reader.GetSetting<int>({L"edid_mode_filtering", L"min_resolution_width"},
							   settings.auto_resolutions.edid_mode_filtering
								   .min_resolution_width); // sta sotto auto_resolutions/edid_mode_filtering
	reg_reader.GetSetting<int>({L"edid_mode_filtering", L"min_resolution_height"},
							   settings.auto_resolutions.edid_mode_filtering
								   .min_resolution_height); // sta sotto auto_resolutions/edid_mode_filtering
	reg_reader.GetSetting<int>({L"edid_mode_filtering", L"max_resolution_width"},
							   settings.auto_resolutions.edid_mode_filtering
								   .max_resolution_width); // sta sotto auto_resolutions/edid_mode_filtering
	reg_reader.GetSetting<int>({L"edid_mode_filtering", L"max_resolution_height"},
							   settings.auto_resolutions.edid_mode_filtering
								   .max_resolution_height); // sta sotto auto_resolutions/edid_mode_filtering

	reg_reader.GetSetting<bool>({L"preferred_mode", L"use_edid_preferred"},
								settings.auto_resolutions.preferred_mode.preferred);
	reg_reader.GetSetting<int>(
		{L"preferred_mode", L"fallback_width"},
		settings.auto_resolutions.preferred_mode.fallback_width); // sta sotto auto_resolutions/preferred_mode
	reg_reader.GetSetting<int>({L"preferred_mode", L"fallback_height"},
							   settings.auto_resolutions.preferred_mode.fallback_height);
	reg_reader.GetSetting<int>({L"preferred_mode", L"fallback_refresh"},
							   settings.auto_resolutions.preferred_mode.fallback_refresh);
	// manca use_edid_preferred di auto_res - errore dell'xml? (c'è anche in un altro nodo)

	// === LOAD COLOR ADVANCED SETTINGS ===
	reg_reader.GetSetting<std::wstring>({L"bit_depth_management", L"force_bit_depth"},
		settings.color_advanced.bit_depth_management.force_bit_depth); // stanno in color_advanced/bit_depth_management
	reg_reader.GetSetting<bool>({L"bit_depth_management", L"auto_select_from_color_space"},
								settings.color_advanced.bit_depth_management
									.auto_select_from_color_space); // sta in color_advanced/bit_depth_management
	reg_reader.GetSetting<bool>({L"bit_depth_management", L"fp16_surface_support"},
								settings.color_advanced.bit_depth_management
									.fp16_surface_support); // sta in color_advanced/bit_depth_management
	reg_reader.GetSetting<bool>({L"not_implemented", L"not_implemented"},
								settings.color_advanced.color_format_extended.wide_color_gamut); // not implemented
	reg_reader.GetSetting<bool>({L"not_implemented", L"not_implemented"},
								settings.color_advanced.color_format_extended.hdr_tone_mapping); // not

	reg_reader.GetSetting<std::wstring>({L"color_space", L"primary_color_space"},
		settings.hdr_advanced.color_space.primary_color_space); // sta in hdr_advanced/color_space
	reg_reader.GetSetting<bool>({L"color_space", L"enabled"},
								settings.hdr_advanced.color_space.enabled); // sta in hdr_advanced/color_space
	reg_reader.GetSetting<double>(
		{L"color_space", L"gamma_correction"},
		settings.hdr_advanced.color_space.gamma_correction); // sta in hdr_advanced/color_space

	reg_reader.GetSetting<bool>(
		{L"color_primaries", L"enabled"},
		settings.hdr_advanced.color_primaries.primaries_enabled); // sta in hdr_advanced/color_primaries
	reg_reader.GetSetting<double>({L"color_primaries", L"RedX"},
								  settings.hdr_advanced.color_primaries.redX); // sta in hdr_advanced/color_primaries
	reg_reader.GetSetting<double>({L"color_primaries", L"RedY"},
								  settings.hdr_advanced.color_primaries.redY); // sta in hdr_advanced/color_primaries
	reg_reader.GetSetting<double>({L"color_primaries", L"GreenX"},
								  settings.hdr_advanced.color_primaries.greenX); // sta in hdr_advanced/color_primaries
	reg_reader.GetSetting<double>({L"color_primaries", L"GreenY"},
								  settings.hdr_advanced.color_primaries.greenY); // sta in hdr_advanced/color_primaries
	reg_reader.GetSetting<double>({L"color_primaries", L"BlueX"},
								  settings.hdr_advanced.color_primaries.blueX); // sta in hdr_advanced/color_primaries
	reg_reader.GetSetting<double>({L"color_primaries", L"BlueY"},
								  settings.hdr_advanced.color_primaries.blueY); // sta in hdr_advanced/color_primaries
	reg_reader.GetSetting<double>({L"color_primaries", L"WhiteX"},
								  settings.hdr_advanced.color_primaries.whiteX); // sta in hdr_advanced/color_primaries
	reg_reader.GetSetting<double>({L"color_primaries", L"WhiteY"},
								  settings.hdr_advanced.color_primaries.whiteY); // sta in hdr_advanced/color_primaries

	reg_reader.GetSetting<double>(
		{L"color_format_extended", L"sdr_white_level"},
		settings.color_advanced.color_format_extended.sdr_white_level); // sta in color_advanced/color_format_extended

	// === LOAD MONITOR EMULATION SETTINGS ===
	//
	// loaded from defaults but not implemented
	reg_reader.GetSetting<bool>({L"not_implemented", L"not_implemented"},
								settings.monitor_emulation.manufacturer_emulation_enabled);
	reg_reader.GetSetting<bool>({L"not_implemented", L"not_implemented"},
								settings.monitor_emulation.enabled);
	reg_reader.GetSetting<bool>({L"not_implemented", L"not_implemented"},
								settings.monitor_emulation.emulate_physical_dimensions);
	reg_reader.GetSetting<int>({L"not_implemented", L"not_implemented"},
							   settings.monitor_emulation.physical_width);
	reg_reader.GetSetting<int>({L"not_implemented", L"not_implemented"},
							   settings.monitor_emulation.physical_height);

	reg_reader.GetSetting<std::wstring>({L"not_implemented", L"not_implemented"},
										settings.monitor_emulation.manufacturer_name);
	reg_reader.GetSetting<std::wstring>({L"not_implemented", L"not_implemented"},
										settings.monitor_emulation.model_name);
	reg_reader.GetSetting<std::wstring>({L"not_implemented", L"not_implemented"},
										settings.monitor_emulation.serial_number);

	// xorCursorSupportLevelName = XorCursorSupportLevelToString(g_settings.cursor.xor_cursor_support_level);
	// vddlog("i", ("Selected Xor Cursor Support Level: " + xorCursorSupportLevelName).c_str());
}

