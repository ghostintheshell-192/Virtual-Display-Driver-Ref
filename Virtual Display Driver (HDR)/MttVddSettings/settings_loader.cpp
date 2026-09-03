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

	check_xml = xml_reader.OpenFile(conf_path + "\\vdd_settings.xml");

	entries.push_back({"logging.logging", &settings.logs.enable_standard_logs});
	entries.push_back({"logging.debuglogging", &settings.logs.enable_debug_logs});
	entries.push_back({"logging.SendLogsThroughPipe", &settings.logs.send_logs_through_pipe});
	entries.push_back({"edid.CustomEdid", &settings.edid.custom_edid});
	entries.push_back({"edid.PreventSpoof", &settings.edid.prevent_manufacturer_spoof});
	entries.push_back({"edid.EdidCeaOverride", &settings.edid.edid_cea_override});
	entries.push_back({"colour.HDRPlus", &settings.colours.hdr_plus});
	entries.push_back({"colour.SDR10bit", &settings.colours.sdr10});
	entries.push_back({"colour.ColourFormat", &settings.colours.color_format});
	entries.push_back({"cursor.HardwareCursor", &settings.cursor.hardware_cursor});
	entries.push_back({"cursor.AlphaCursorSupport", &settings.cursor.alpha_cursor_support});
	entries.push_back({"cursor.CursorMaxX", &settings.cursor.max_x});
	entries.push_back({"cursor.CursorMaxY", &settings.cursor.max_y});
	entries.push_back({"edid_integration.edid_profile_path", &settings.edid_integration.profile_path});
	entries.push_back({"edid_integration.enabled", &settings.edid_integration.enabled});
	entries.push_back({"edid_integration.auto_configure_from_edid", &settings.edid_integration.auto_configure});
	entries.push_back(
		{"edid_integration.override_manual_settings", &settings.edid_integration.override_manual_settings});
	entries.push_back({"edid_integration.fallback_on_error", &settings.edid_integration.fallback_on_error});
	entries.push_back({"hdr_advanced.hdr10_static_metadata.enabled", &settings.hdr_advanced.static_metadata_enabled});
	entries.push_back({"hdr_advanced.hdr10_static_metadata.max_display_mastering_luminance",
					   &settings.hdr_advanced.max_display_mastering_luminance});
	entries.push_back({"hdr_advanced.hdr10_static_metadata.min_display_mastering_luminance",
					   &settings.hdr_advanced.min_display_mastering_luminance});
	entries.push_back(
		{"hdr_advanced.hdr10_static_metadata.max_content_light_level", &settings.hdr_advanced.max_content_light_level});
	entries.push_back({"hdr_advanced.hdr10_static_metadata.max_frame_avg_light_level",
					   &settings.hdr_advanced.max_frame_avg_light_level});
	entries.push_back({"auto_resolutions.source_priority", &settings.auto_resolutions.source_priority});
	entries.push_back({"auto_resolutions.enabled", &settings.auto_resolutions.enabled});
	entries.push_back({"auto_resolutions.edid_mode_filtering.exclude_fractional_rates",
					   &settings.auto_resolutions.edid_mode_filtering.exclude_fractional_rates});
	entries.push_back({"auto_resolutions.edid_mode_filtering.min_refresh_rate",
					   &settings.auto_resolutions.edid_mode_filtering.min_refresh_rate});
	entries.push_back({"auto_resolutions.edid_mode_filtering.max_refresh_rate",
					   &settings.auto_resolutions.edid_mode_filtering.max_refresh_rate});
	entries.push_back({"auto_resolutions.edid_mode_filtering.min_resolution_width",
					   &settings.auto_resolutions.edid_mode_filtering.min_resolution_width});
	entries.push_back({"auto_resolutions.edid_mode_filtering.min_resolution_height",
					   &settings.auto_resolutions.edid_mode_filtering.min_resolution_height});
	entries.push_back({"auto_resolutions.edid_mode_filtering.max_resolution_width",
					   &settings.auto_resolutions.edid_mode_filtering.max_resolution_width});
	entries.push_back({"auto_resolutions.edid_mode_filtering.max_resolution_height",
					   &settings.auto_resolutions.edid_mode_filtering.max_resolution_height});
	entries.push_back(
		{"auto_resolutions.preferred_mode.use_edid_preferred", &settings.auto_resolutions.preferred_mode.preferred});
	entries.push_back(
		{"auto_resolutions.preferred_mode.fallback_width", &settings.auto_resolutions.preferred_mode.fallback_width});
	entries.push_back(
		{"auto_resolutions.preferred_mode.fallback_height", &settings.auto_resolutions.preferred_mode.fallback_height});
	entries.push_back({"auto_resolutions.preferred_mode.fallback_refresh",
					   &settings.auto_resolutions.preferred_mode.fallback_refresh});
	entries.push_back({"color_advanced.bit_depth_management.force_bit_depth",
					   &settings.color_advanced.bit_depth_management.force_bit_depth});
	entries.push_back({"color_advanced.bit_depth_management.auto_select_from_color_space",
					   &settings.color_advanced.bit_depth_management.auto_select_from_color_space});
	entries.push_back({"color_advanced.bit_depth_management.fp16_surface_support",
					   &settings.color_advanced.bit_depth_management.fp16_surface_support});
	entries.push_back(
		{"hdr_advanced.color_space.primary_color_space", &settings.hdr_advanced.color_space.primary_color_space});
	entries.push_back({"hdr_advanced.color_space.enabled", &settings.hdr_advanced.color_space.enabled});
	entries.push_back(
		{"hdr_advanced.color_space.gamma_correction", &settings.hdr_advanced.color_space.gamma_correction});
	entries.push_back({"hdr_advanced.color_space.enable_matrix_transform",
					   &settings.hdr_advanced.color_space.enable_matrix_transform});
	entries.push_back(
		{"hdr_advanced.color_primaries.enabled", &settings.hdr_advanced.color_primaries.primaries_enabled});
	entries.push_back({"hdr_advanced.color_primaries.red_x", &settings.hdr_advanced.color_primaries.redX});
	entries.push_back({"hdr_advanced.color_primaries.red_y", &settings.hdr_advanced.color_primaries.redY});
	entries.push_back({"hdr_advanced.color_primaries.green_x", &settings.hdr_advanced.color_primaries.greenX});
	entries.push_back({"hdr_advanced.color_primaries.green_y", &settings.hdr_advanced.color_primaries.greenY});
	entries.push_back({"hdr_advanced.color_primaries.blue_x", &settings.hdr_advanced.color_primaries.blueX});
	entries.push_back({"hdr_advanced.color_primaries.blue_y", &settings.hdr_advanced.color_primaries.blueY});
	entries.push_back({"hdr_advanced.color_primaries.white_x", &settings.hdr_advanced.color_primaries.whiteX});
	entries.push_back({"hdr_advanced.color_primaries.white_y", &settings.hdr_advanced.color_primaries.whiteY});
	entries.push_back({"color_advanced.color_format_extended.sdr_white_level",
					   &settings.color_advanced.color_format_extended.sdr_white_level});
}

void Refactoring::SettingsLoader::LoadSettings()
{
	if (check_registry)
		reg_reader.OpenRegistry();

	for (const auto &entry : entries)
	{
		if (check_xml)
			xml_reader.GetSetting(entry.key, entry.container);
		if (check_registry)
			reg_reader.GetSetting(entry.key, entry.container);
	}

	if (check_registry)
		reg_reader.CloseRegistry();
}