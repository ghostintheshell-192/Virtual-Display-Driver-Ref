#include "settings_loader.h"
#include <iostream>
#include <string>

typedef std::vector<std::string> keys;

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

	check_xml = xml_reader.OpenFile(conf_path);
}

void Refactoring::SettingsLoader::LoadSettings()
{
	if (check_registry)
	{
		reg_reader.OpenRegistry();
		OverrideDefaultsRegistry();
		reg_reader.CloseRegistry();
	}

	if (check_xml)
	{
		OverrideDefaultsXml();
	}
}

void Refactoring::SettingsLoader::OverrideDefaultsRegistry()
{
	reg_reader.GetSetting<bool>(keys({"logging", "logging"}), settings.logs.enable_standard_logs);
	reg_reader.GetSetting<bool>(keys({"logging", "debuglogging"}), settings.logs.enable_debug_logs);
	reg_reader.GetSetting<bool>(keys({"logging", "SendLogsThroughPipe"}), settings.logs.send_logs_through_pipe);

	reg_reader.GetSetting<bool>(keys({"edid", "CustomEdid"}), settings.edid.custom_edid);
	reg_reader.GetSetting<bool>(keys({"edid", "PreventSpoof"}), settings.edid.prevent_manufacturer_spoof);
	reg_reader.GetSetting<bool>(keys({"edid", "EdidCeaOverride"}), settings.edid.edid_cea_override);

	// colour
	reg_reader.GetSetting<bool>(keys({"colour", "HDRPlus"}), settings.colours.hdr_plus);
	reg_reader.GetSetting<bool>(keys({"colour", "SDR10bit"}), settings.colours.sdr10);
	// g_settings.colors.HDR_COLOR = g_settings.colors.hdr_plus ? IDDCX_BITS_PER_COMPONENT_12 :
	// IDDCX_BITS_PER_COMPONENT_10; g_settings.colors.SDR_COLOR = g_settings.colors.sdr10 ? IDDCX_BITS_PER_COMPONENT_10
	// : IDDCX_BITS_PER_COMPONENT_8;
	reg_reader.GetSetting<std::string>(keys({"colour", "ColourFormat"}), settings.colours.color_format);

	// Cursor
	reg_reader.GetSetting<bool>(keys({"cursor", "HardwareCursor"}), settings.cursor.hardware_cursor);
	reg_reader.GetSetting<bool>(keys({"cursor", "AlphaCursorSupport"}), settings.cursor.alpha_cursor_support);
	reg_reader.GetSetting<int>(keys({"cursor", "CursorMaxX"}), settings.cursor.max_x);
	reg_reader.GetSetting<int>(keys({"cursor", "CursorMaxY"}), settings.cursor.max_y);

	// int xorCursorSupportLevelInt = GetSetting<int>(hKey, "XorCursorSupportLevel", );
	// std::string xorCursorSupportLevelName;

	// if (xorCursorSupportLevelInt < 0 || xorCursorSupportLevelInt > 3) {
	//	vddlog("w", "Selected Xor Level unsupported, defaulting to IDDCX_XOR_CURSOR_SUPPORT_FULL");
	//	g_settings.cursor.xor_cursor_support_level = IDDCX_XOR_CURSOR_SUPPORT_FULL;
	// }
	// else {
	//	g_settings.cursor.xor_cursor_support_level = static_cast<IDDCX_XOR_CURSOR_SUPPORT>(xorCursorSupportLevelInt);
	// }

	// === LOAD NEW EDID INTEGRATION SETTINGS ===
	reg_reader.GetSetting<std::string>(keys({"edid_integration", "edid_profile_path"}),
									   settings.edid_integration.profile_path);
	reg_reader.GetSetting<bool>(keys({"edid_integration", "enabled"}), settings.edid_integration.enabled);
	reg_reader.GetSetting<bool>(keys({"edid_integration", "auto_configure_from_edid"}),
								settings.edid_integration.auto_configure);
	reg_reader.GetSetting<bool>(keys({"edid_integration", "override_manual_settings"}),
								settings.edid_integration.override_manual_settings);
	reg_reader.GetSetting<bool>(keys({"edid_integration", "fallback_on_error"}),
								settings.edid_integration.fallback_on_error);

	// === LOAD HDR ADVANCED SETTINGS ===
	reg_reader.GetSetting<bool>(keys({"color_space", "enable_matrix_transform"}),
								settings.hdr_advanced.color_space.enable_matrix_transform);
	reg_reader.GetSetting<bool>(keys({"hdr10_static_metadata", "enabled"}),
								settings.hdr_advanced.static_metadata_enabled);
	reg_reader.GetSetting<double>(keys({"hdr10_static_metadata", "max_display_mastering_luminance"}),
								  settings.hdr_advanced.max_display_mastering_luminance);
	reg_reader.GetSetting<double>(keys({"hdr10_static_metadata", "min_display_mastering_luminance"}),
								  settings.hdr_advanced.min_display_mastering_luminance);
	reg_reader.GetSetting<int>(keys({"hdr10_static_metadata", "max_content_light_level"}),
							   settings.hdr_advanced.max_content_light_level);
	reg_reader.GetSetting<int>(keys({"hdr10_static_metadata", "max_frame_avg_light_level"}),
							   settings.hdr_advanced.max_frame_avg_light_level);

	// === LOAD AUTO RESOLUTIONS SETTINGS ===
	reg_reader.GetSetting<std::string>(keys({"auto_resolutions", "source_priority"}),
									   settings.auto_resolutions.source_priority);
	reg_reader.GetSetting<bool>(keys({"auto_resolutions", "enabled"}), settings.auto_resolutions.enabled);

	reg_reader.GetSetting<bool>(keys({"auto_resolutions", "edid_mode_filtering", "exclude_fractional_rates"}),
								settings.auto_resolutions.edid_mode_filtering.exclude_fractional_rates);
	reg_reader.GetSetting<int>(keys({"auto_resolutions", "edid_mode_filtering", "min_refresh_rate"}),
							   settings.auto_resolutions.edid_mode_filtering.min_refresh_rate);
	reg_reader.GetSetting<int>(keys({"auto_resolutions", "edid_mode_filtering", "max_refresh_rate"}),
							   settings.auto_resolutions.edid_mode_filtering.max_refresh_rate);
	reg_reader.GetSetting<int>(keys({"auto_resolutions", "edid_mode_filtering", "min_resolution_width"}),
							   settings.auto_resolutions.edid_mode_filtering.min_resolution_width);
	reg_reader.GetSetting<int>(keys({"edid_mode_filtering", "min_resolution_height"}),
							   settings.auto_resolutions.edid_mode_filtering.min_resolution_height);
	reg_reader.GetSetting<int>(
		keys({"auto_resolutions", "auto_resolutions", "edid_mode_filtering", "max_resolution_width"}),
		settings.auto_resolutions.edid_mode_filtering.max_resolution_width);
	reg_reader.GetSetting<int>(keys({"auto_resolutions", "edid_mode_filtering", "max_resolution_height"}),
							   settings.auto_resolutions.edid_mode_filtering.max_resolution_height);

	reg_reader.GetSetting<bool>(keys({"auto_resolutions", "preferred_mode", "use_edid_preferred"}),
								settings.auto_resolutions.preferred_mode.preferred);
	reg_reader.GetSetting<int>(keys({"auto_resolutions", "preferred_mode", "fallback_width"}),
							   settings.auto_resolutions.preferred_mode.fallback_width);
	reg_reader.GetSetting<int>(keys({"auto_resolutions", "preferred_mode", "fallback_height"}),
							   settings.auto_resolutions.preferred_mode.fallback_height);
	reg_reader.GetSetting<int>(keys({"auto_resolutions", "preferred_mode", "fallback_refresh"}),
							   settings.auto_resolutions.preferred_mode.fallback_refresh);

	// === LOAD COLOR ADVANCED SETTINGS ===
	reg_reader.GetSetting<std::string>(
		keys({"color_advanced", "bit_depth_management", "force_bit_depth"}),
		settings.color_advanced.bit_depth_management.force_bit_depth); // stanno in color_advanced/bit_depth_management
	reg_reader.GetSetting<bool>(keys({"bit_depth_management", "auto_select_from_color_space"}),
								settings.color_advanced.bit_depth_management.auto_select_from_color_space);
	reg_reader.GetSetting<bool>(keys({"bit_depth_management", "fp16_surface_support"}),
								settings.color_advanced.bit_depth_management.fp16_surface_support);
	reg_reader.GetSetting<bool>(keys({"not_implemented", "not_implemented"}),
								settings.color_advanced.color_format_extended.wide_color_gamut);
	reg_reader.GetSetting<bool>(keys({"not_implemented", "not_implemented"}),
								settings.color_advanced.color_format_extended.hdr_tone_mapping);

	reg_reader.GetSetting<std::string>(keys({"color_space", "primary_color_space"}),
									   settings.hdr_advanced.color_space.primary_color_space);
	reg_reader.GetSetting<bool>(keys({"color_space", "enabled"}), settings.hdr_advanced.color_space.enabled);
	reg_reader.GetSetting<double>(keys({"color_space", "gamma_correction"}),
								  settings.hdr_advanced.color_space.gamma_correction);

	reg_reader.GetSetting<bool>(keys({"color_primaries", "enabled"}),
								settings.hdr_advanced.color_primaries.primaries_enabled);
	reg_reader.GetSetting<double>(keys({"color_primaries", "RedX"}), settings.hdr_advanced.color_primaries.redX);
	reg_reader.GetSetting<double>(keys({"color_primaries", "RedY"}), settings.hdr_advanced.color_primaries.redY);
	reg_reader.GetSetting<double>(keys({"color_primaries", "GreenX"}), settings.hdr_advanced.color_primaries.greenX);
	reg_reader.GetSetting<double>(keys({"color_primaries", "GreenY"}), settings.hdr_advanced.color_primaries.greenY);
	reg_reader.GetSetting<double>(keys({"color_primaries", "BlueX"}), settings.hdr_advanced.color_primaries.blueX);
	reg_reader.GetSetting<double>(keys({"color_primaries", "BlueY"}), settings.hdr_advanced.color_primaries.blueY);
	reg_reader.GetSetting<double>(keys({"color_primaries", "WhiteX"}), settings.hdr_advanced.color_primaries.whiteX);
	reg_reader.GetSetting<double>(keys({"color_primaries", "WhiteY"}), settings.hdr_advanced.color_primaries.whiteY);

	reg_reader.GetSetting<double>(
		keys({"color_format_extended", "sdr_white_level"}),
		settings.color_advanced.color_format_extended.sdr_white_level); // sta in color_advanced/color_format_extended

	// === LOAD MONITOR EMULATION SETTINGS ===
	//
	// loaded from defaults but not implemented
	reg_reader.GetSetting<bool>(keys({"not_implemented", "not_implemented"}),
								settings.monitor_emulation.manufacturer_emulation_enabled);
	reg_reader.GetSetting<bool>(keys({"not_implemented", "not_implemented"}), settings.monitor_emulation.enabled);
	reg_reader.GetSetting<bool>(keys({"not_implemented", "not_implemented"}),
								settings.monitor_emulation.emulate_physical_dimensions);
	reg_reader.GetSetting<int>(keys({"not_implemented", "not_implemented"}), settings.monitor_emulation.physical_width);
	reg_reader.GetSetting<int>(keys({"not_implemented", "not_implemented"}),
							   settings.monitor_emulation.physical_height);

	reg_reader.GetSetting<std::string>(keys({"not_implemented", "not_implemented"}),
									   settings.monitor_emulation.manufacturer_name);
	reg_reader.GetSetting<std::string>(keys({"not_implemented", "not_implemented"}),
									   settings.monitor_emulation.model_name);
	reg_reader.GetSetting<std::string>(keys({"not_implemented", "not_implemented"}),
									   settings.monitor_emulation.serial_number);

	// xorCursorSupportLevelName = XorCursorSupportLevelToString(g_settings.cursor.xor_cursor_support_level);
	// vddlog("i", ("Selected Xor Cursor Support Level: " + xorCursorSupportLevelName).c_str());
}

void Refactoring::SettingsLoader::OverrideDefaultsXml()
{
	reg_reader.GetSetting<bool>(keys({"logging", "logging"}), settings.logs.enable_standard_logs);
	reg_reader.GetSetting<bool>(keys({"logging", "debuglogging"}), settings.logs.enable_debug_logs);
	reg_reader.GetSetting<bool>(keys({"logging", "SendLogsThroughPipe"}), settings.logs.send_logs_through_pipe);

	reg_reader.GetSetting<bool>(keys({"edid", "CustomEdid"}), settings.edid.custom_edid);
	reg_reader.GetSetting<bool>(keys({"edid", "PreventSpoof"}), settings.edid.prevent_manufacturer_spoof);
	reg_reader.GetSetting<bool>(keys({"edid", "EdidCeaOverride"}), settings.edid.edid_cea_override);

	// colour
	reg_reader.GetSetting<bool>(keys({"colour", "HDRPlus"}), settings.colours.hdr_plus);
	reg_reader.GetSetting<bool>(keys({"colour", "SDR10bit"}), settings.colours.sdr10);
	// g_settings.colors.HDR_COLOR = g_settings.colors.hdr_plus ? IDDCX_BITS_PER_COMPONENT_12 :
	// IDDCX_BITS_PER_COMPONENT_10; g_settings.colors.SDR_COLOR = g_settings.colors.sdr10 ? IDDCX_BITS_PER_COMPONENT_10
	// : IDDCX_BITS_PER_COMPONENT_8;
	reg_reader.GetSetting<std::string>(keys({"colour", "ColourFormat"}), settings.colours.color_format);

	// Cursor
	reg_reader.GetSetting<bool>(keys({"cursor", "HardwareCursor"}), settings.cursor.hardware_cursor);
	reg_reader.GetSetting<bool>(keys({"cursor", "AlphaCursorSupport"}), settings.cursor.alpha_cursor_support);
	reg_reader.GetSetting<int>(keys({"cursor", "CursorMaxX"}), settings.cursor.max_x);
	reg_reader.GetSetting<int>(keys({"cursor", "CursorMaxY"}), settings.cursor.max_y);

	// int xorCursorSupportLevelInt = GetSetting<int>(hKey, "XorCursorSupportLevel", );
	// std::string xorCursorSupportLevelName;

	// if (xorCursorSupportLevelInt < 0 || xorCursorSupportLevelInt > 3) {
	//	vddlog("w", "Selected Xor Level unsupported, defaulting to IDDCX_XOR_CURSOR_SUPPORT_FULL");
	//	g_settings.cursor.xor_cursor_support_level = IDDCX_XOR_CURSOR_SUPPORT_FULL;
	// }
	// else {
	//	g_settings.cursor.xor_cursor_support_level = static_cast<IDDCX_XOR_CURSOR_SUPPORT>(xorCursorSupportLevelInt);
	// }

	// === LOAD NEW EDID INTEGRATION SETTINGS ===
	reg_reader.GetSetting<std::string>(keys({"edid_integration", "edid_profile_path"}),
									   settings.edid_integration.profile_path);
	reg_reader.GetSetting<bool>(keys({"edid_integration", "enabled"}), settings.edid_integration.enabled);
	reg_reader.GetSetting<bool>(keys({"edid_integration", "auto_configure_from_edid"}),
								settings.edid_integration.auto_configure);
	reg_reader.GetSetting<bool>(keys({"edid_integration", "override_manual_settings"}),
								settings.edid_integration.override_manual_settings);
	reg_reader.GetSetting<bool>(keys({"edid_integration", "fallback_on_error"}),
								settings.edid_integration.fallback_on_error);

	// === LOAD HDR ADVANCED SETTINGS ===
	reg_reader.GetSetting<bool>(keys({"hdr_advanced", "hdr10_static_metadata", "enabled"}),
								settings.hdr_advanced.static_metadata_enabled);
	reg_reader.GetSetting<double>(keys({"hdr_advanced", "hdr10_static_metadata", "max_display_mastering_luminance"}),
								  settings.hdr_advanced.max_display_mastering_luminance);
	reg_reader.GetSetting<double>(keys({"hdr_advanced", "hdr10_static_metadata", "min_display_mastering_luminance"}),
								  settings.hdr_advanced.min_display_mastering_luminance);
	reg_reader.GetSetting<int>(keys({"hdr_advanced", "hdr10_static_metadata", "max_content_light_level"}),
							   settings.hdr_advanced.max_content_light_level);
	reg_reader.GetSetting<int>(keys({"hdr_advanced", "hdr10_static_metadata", "max_frame_avg_light_level"}),
							   settings.hdr_advanced.max_frame_avg_light_level);

	// === LOAD AUTO RESOLUTIONS SETTINGS ===
	reg_reader.GetSetting<std::string>(keys({"auto_resolutions", "source_priority"}),
									   settings.auto_resolutions.source_priority);
	reg_reader.GetSetting<bool>(keys({"auto_resolutions", "enabled"}), settings.auto_resolutions.enabled);

	reg_reader.GetSetting<bool>(keys({"edid_mode_filtering", "exclude_fractional_rates"}),
								settings.auto_resolutions.edid_mode_filtering.exclude_fractional_rates);
	reg_reader.GetSetting<int>(keys({"edid_mode_filtering", "min_refresh_rate"}),
							   settings.auto_resolutions.edid_mode_filtering.min_refresh_rate);
	reg_reader.GetSetting<int>(keys({"edid_mode_filtering", "max_refresh_rate"}),
							   settings.auto_resolutions.edid_mode_filtering.max_refresh_rate);
	reg_reader.GetSetting<int>(keys({"edid_mode_filtering", "min_resolution_width"}),
							   settings.auto_resolutions.edid_mode_filtering.min_resolution_width);
	reg_reader.GetSetting<int>(keys({"edid_mode_filtering", "min_resolution_height"}),
							   settings.auto_resolutions.edid_mode_filtering.min_resolution_height);
	reg_reader.GetSetting<int>(keys({"edid_mode_filtering", "max_resolution_width"}),
							   settings.auto_resolutions.edid_mode_filtering.max_resolution_width);
	reg_reader.GetSetting<int>(keys({"edid_mode_filtering", "max_resolution_height"}),
							   settings.auto_resolutions.edid_mode_filtering.max_resolution_height);

	reg_reader.GetSetting<bool>(keys({"preferred_mode", "use_edid_preferred"}),
								settings.auto_resolutions.preferred_mode.preferred);
	reg_reader.GetSetting<int>(keys({"auto_resolutions", "preferred_mode", "fallback_width"}),
							   settings.auto_resolutions.preferred_mode.fallback_width);
	reg_reader.GetSetting<int>(keys({"preferred_mode", "fallback_height"}),
							   settings.auto_resolutions.preferred_mode.fallback_height);
	reg_reader.GetSetting<int>(keys({"preferred_mode", "fallback_refresh"}),
							   settings.auto_resolutions.preferred_mode.fallback_refresh);
	// manca use_edid_preferred di auto_res - errore dell'xml? (c'è anche in un altro nodo)

	// === LOAD COLOR ADVANCED SETTINGS ===
	reg_reader.GetSetting<std::string>(
		keys({"bit_depth_management", "force_bit_depth"}),
		settings.color_advanced.bit_depth_management.force_bit_depth); // stanno in color_advanced/bit_depth_management
	reg_reader.GetSetting<bool>(keys({"bit_depth_management", "auto_select_from_color_space"}),
								settings.color_advanced.bit_depth_management.auto_select_from_color_space);
	reg_reader.GetSetting<bool>(keys({"bit_depth_management", "fp16_surface_support"}),
								settings.color_advanced.bit_depth_management.fp16_surface_support);
	reg_reader.GetSetting<bool>(keys({"not_implemented", "not_implemented"}),
								settings.color_advanced.color_format_extended.wide_color_gamut); // not implemented
	reg_reader.GetSetting<bool>(keys({"not_implemented", "not_implemented"}),
								settings.color_advanced.color_format_extended.hdr_tone_mapping);

	reg_reader.GetSetting<std::string>(keys({"hdr_advanced", "color_space", "primary_color_space"}),
									   settings.hdr_advanced.color_space.primary_color_space);
	reg_reader.GetSetting<bool>(keys({"hdr_advanced", "color_space", "enabled"}),
								settings.hdr_advanced.color_space.enabled);
	reg_reader.GetSetting<double>(keys({"hdr_advanced", "color_space", "gamma_correction"}),
								  settings.hdr_advanced.color_space.gamma_correction);
	reg_reader.GetSetting<bool>(keys({"hdr_advanced", "color_space", "enable_matrix_transform"}),
								settings.hdr_advanced.color_space.enable_matrix_transform);

	reg_reader.GetSetting<bool>(keys({"hdr_advanced", "color_primaries", "enabled"}),
								settings.hdr_advanced.color_primaries.primaries_enabled);
	reg_reader.GetSetting<double>(keys({"hdr_advanced", "color_primaries", "RedX"}),
								  settings.hdr_advanced.color_primaries.redX);
	reg_reader.GetSetting<double>(keys({"hdr_advanced", "color_primaries", "RedY"}),
								  settings.hdr_advanced.color_primaries.redY);
	reg_reader.GetSetting<double>(keys({"hdr_advanced", "color_primaries", "GreenX"}),
								  settings.hdr_advanced.color_primaries.greenX);
	reg_reader.GetSetting<double>(keys({"hdr_advanced", "color_primaries", "GreenY"}),
								  settings.hdr_advanced.color_primaries.greenY);
	reg_reader.GetSetting<double>(keys({"hdr_advanced", "color_primaries", "BlueX"}),
								  settings.hdr_advanced.color_primaries.blueX);
	reg_reader.GetSetting<double>(keys({"hdr_advanced", "color_primaries", "BlueY"}),
								  settings.hdr_advanced.color_primaries.blueY);
	reg_reader.GetSetting<double>(keys({"hdr_advanced", "color_primaries", "WhiteX"}),
								  settings.hdr_advanced.color_primaries.whiteX);
	reg_reader.GetSetting<double>(keys({"hdr_advanced", "color_primaries", "WhiteY"}),
								  settings.hdr_advanced.color_primaries.whiteY);

	reg_reader.GetSetting<double>(
		keys({"color_format_extended", "sdr_white_level"}),
		settings.color_advanced.color_format_extended.sdr_white_level); // sta in color_advanced/color_format_extended

	// === LOAD MONITOR EMULATION SETTINGS ===
	//
	// loaded from defaults but not implemented
	// reg_reader.GetSetting<bool>(keys({"not_implemented", "not_implemented"}),
	//							settings.monitor_emulation.manufacturer_emulation_enabled);
	// reg_reader.GetSetting<bool>(keys({"not_implemented", "not_implemented"}), settings.monitor_emulation.enabled);
	// reg_reader.GetSetting<bool>(keys({"not_implemented", "not_implemented"}),
	//							settings.monitor_emulation.emulate_physical_dimensions);
	// reg_reader.GetSetting<int>(keys({"not_implemented", "not_implemented"}),
	// settings.monitor_emulation.physical_width); reg_reader.GetSetting<int>(keys({"not_implemented",
	// "not_implemented"}), 						   settings.monitor_emulation.physical_height);

	// reg_reader.GetSetting<std::string>(keys({"not_implemented", "not_implemented"}),
	//								   settings.monitor_emulation.manufacturer_name);
	// reg_reader.GetSetting<std::string>(keys({"not_implemented", "not_implemented"}),
	//								   settings.monitor_emulation.model_name);
	// reg_reader.GetSetting<std::string>(keys({"not_implemented", "not_implemented"}),
	//								   settings.monitor_emulation.serial_number);

	// xorCursorSupportLevelName = XorCursorSupportLevelToString(g_settings.cursor.xor_cursor_support_level);
	// vddlog("i", ("Selected Xor Cursor Support Level: " + xorCursorSupportLevelName).c_str());
}