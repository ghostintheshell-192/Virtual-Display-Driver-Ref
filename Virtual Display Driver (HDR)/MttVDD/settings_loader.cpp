#include "settings_loader.h"
#include <string>
#include <algorithm>

Refactoring::SettingsLoader::SettingsLoader(Logger *log, DriverSettings *g_settings)
	: m_log(log), reg_reader(log), xml_reader(log), m_settings(g_settings), check_registry(false), check_xml(false), conf_path()
{
}

void Refactoring::SettingsLoader::Init()
{
	conf_path = "C:\\data\\repos\\Sandbox\\Virtual-Display-Driver-Ref\\Virtual Display Driver (HDR)";
	m_log->Message(LogType::Info, "[SettingsLoader] Config Path is at default value: " + conf_path + "\n");

	check_registry = reg_reader.OpenRegistry();

	if (check_registry)
	{
		reg_reader.InitializePath(conf_path);
		reg_reader.CloseRegistry();
	}

	check_xml = xml_reader.OpenFile(conf_path + "\\vdd_settings.xml");

	entries.push_back({"logging.logging", &(m_settings->logs.enable_standard_logs)});
	entries.push_back({"logging.debuglogging", &(m_settings->logs.enable_debug_logs)});
	entries.push_back({"logging.SendLogsThroughPipe", &(m_settings->logs.send_logs_through_pipe)});
	entries.push_back({"edid.CustomEdid", &(m_settings->edid.custom_edid)});
	entries.push_back({"edid.PreventSpoof", &(m_settings->edid.prevent_manufacturer_spoof)});
	entries.push_back({"edid.EdidCeaOverride", &(m_settings->edid.edid_cea_override)});
	entries.push_back({"colour.HDRPlus", &(m_settings->colours.hdr_plus)});
	entries.push_back({"colour.SDR10bit", &(m_settings->colours.sdr10)});
	entries.push_back({"colour.ColourFormat", &(m_settings->colours.color_format)});
	entries.push_back({"cursor.HardwareCursor", &(m_settings->cursor.hardware_cursor)});
	entries.push_back({"cursor.AlphaCursorSupport", &(m_settings->cursor.alpha_cursor_support)});
	entries.push_back({"cursor.CursorMaxX", &(m_settings->cursor.max_x)});
	entries.push_back({"cursor.CursorMaxY", &(m_settings->cursor.max_y)});
	entries.push_back({"cursor.XorCursorSupportLevel", &(m_settings->cursor.xor_cursor_support_level)});
	entries.push_back({"edid_integration.edid_profile_path", &(m_settings->edid_integration.profile_path)});
	entries.push_back({"edid_integration.enabled", &(m_settings->edid_integration.enabled)});
	entries.push_back({"edid_integration.auto_configure_from_edid", &(m_settings->edid_integration.auto_configure)});
	entries.push_back(
		{"edid_integration.override_manual_settings", &(m_settings->edid_integration.override_manual_settings)});
	entries.push_back({"edid_integration.fallback_on_error", &(m_settings->edid_integration.fallback_on_error)});
	entries.push_back({"hdr_advanced.hdr10_static_metadata.enabled", &(m_settings->hdr_advanced.static_metadata_enabled)});
	entries.push_back({"hdr_advanced.hdr10_static_metadata.max_display_mastering_luminance",
					   &(m_settings->hdr_advanced.max_display_mastering_luminance)});
	entries.push_back({"hdr_advanced.hdr10_static_metadata.min_display_mastering_luminance",
					   &(m_settings->hdr_advanced.min_display_mastering_luminance)});
	entries.push_back(
		{"hdr_advanced.hdr10_static_metadata.max_content_light_level", &(m_settings->hdr_advanced.max_content_light_level)});
	entries.push_back({"hdr_advanced.hdr10_static_metadata.max_frame_avg_light_level",
					   &(m_settings->hdr_advanced.max_frame_avg_light_level)});
	entries.push_back({"auto_resolutions.source_priority", &(m_settings->auto_resolutions.source_priority)});
	entries.push_back({"auto_resolutions.enabled", &(m_settings->auto_resolutions.enabled)});
	entries.push_back({"auto_resolutions.edid_mode_filtering.exclude_fractional_rates",
					   &(m_settings->auto_resolutions.edid_mode_filtering.exclude_fractional_rates)});
	entries.push_back({"auto_resolutions.edid_mode_filtering.min_refresh_rate",
					   &(m_settings->auto_resolutions.edid_mode_filtering.min_refresh_rate)});
	entries.push_back({"auto_resolutions.edid_mode_filtering.max_refresh_rate",
					   &(m_settings->auto_resolutions.edid_mode_filtering.max_refresh_rate)});
	entries.push_back({"auto_resolutions.edid_mode_filtering.min_resolution_width",
					   &(m_settings->auto_resolutions.edid_mode_filtering.min_resolution_width)});
	entries.push_back({"auto_resolutions.edid_mode_filtering.min_resolution_height",
					   &(m_settings->auto_resolutions.edid_mode_filtering.min_resolution_height)});
	entries.push_back({"auto_resolutions.edid_mode_filtering.max_resolution_width",
					   &(m_settings->auto_resolutions.edid_mode_filtering.max_resolution_width)});
	entries.push_back({"auto_resolutions.edid_mode_filtering.max_resolution_height",
					   &(m_settings->auto_resolutions.edid_mode_filtering.max_resolution_height)});
	entries.push_back(
		{"auto_resolutions.preferred_mode.use_edid_preferred", &(m_settings->auto_resolutions.preferred_mode.preferred)});
	entries.push_back(
		{"auto_resolutions.preferred_mode.fallback_width", &(m_settings->auto_resolutions.preferred_mode.fallback_width)});
	entries.push_back(
		{"auto_resolutions.preferred_mode.fallback_height", &(m_settings->auto_resolutions.preferred_mode.fallback_height)});
	entries.push_back({"auto_resolutions.preferred_mode.fallback_refresh",
					   &(m_settings->auto_resolutions.preferred_mode.fallback_refresh)});
	entries.push_back({"color_advanced.bit_depth_management.force_bit_depth",
					   &(m_settings->color_advanced.bit_depth_management.force_bit_depth)});
	entries.push_back({"color_advanced.bit_depth_management.auto_select_from_color_space",
					   &(m_settings->color_advanced.bit_depth_management.auto_select_from_color_space)});
	entries.push_back({"color_advanced.bit_depth_management.fp16_surface_support",
					   &(m_settings->color_advanced.bit_depth_management.fp16_surface_support)});
	entries.push_back(
		{"hdr_advanced.color_space.primary_color_space", &(m_settings->hdr_advanced.color_space.primary_color_space)});
	entries.push_back({"hdr_advanced.color_space.enabled", &(m_settings->hdr_advanced.color_space.enabled)});
	entries.push_back(
		{"hdr_advanced.color_space.gamma_correction", &(m_settings->hdr_advanced.color_space.gamma_correction)});
	entries.push_back({"hdr_advanced.color_space.enable_matrix_transform",
					   &(m_settings->hdr_advanced.color_space.enable_matrix_transform)});
	entries.push_back(
		{"hdr_advanced.color_primaries.enabled", &(m_settings->hdr_advanced.color_primaries.primaries_enabled)});
	entries.push_back({"hdr_advanced.color_primaries.red_x", &(m_settings->hdr_advanced.color_primaries.redX)});
	entries.push_back({"hdr_advanced.color_primaries.red_y", &(m_settings->hdr_advanced.color_primaries.redY)});
	entries.push_back({"hdr_advanced.color_primaries.green_x", &(m_settings->hdr_advanced.color_primaries.greenX)});
	entries.push_back({"hdr_advanced.color_primaries.green_y", &(m_settings->hdr_advanced.color_primaries.greenY)});
	entries.push_back({"hdr_advanced.color_primaries.blue_x", &(m_settings->hdr_advanced.color_primaries.blueX)});
	entries.push_back({"hdr_advanced.color_primaries.blue_y", &(m_settings->hdr_advanced.color_primaries.blueY)});
	entries.push_back({"hdr_advanced.color_primaries.white_x", &(m_settings->hdr_advanced.color_primaries.whiteX)});
	entries.push_back({"hdr_advanced.color_primaries.white_y", &(m_settings->hdr_advanced.color_primaries.whiteY)});
	entries.push_back({"color_advanced.color_format_extended.sdr_white_level",
					   &(m_settings->color_advanced.color_format_extended.sdr_white_level)});
}

void Refactoring::SettingsLoader::LoadSettings()
{
	if (check_registry)
		check_registry = reg_reader.OpenRegistry();

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

void Refactoring::SettingsLoader::SetSetting(std::string key, std::string pipe_str_value)
{
	if (!check_xml)
		return;

	for (const auto& entry : entries)
	{

		if (entry.key == key)
		{
			xml_reader.SetSetting(entry.key, pipe_str_value, entry.container);
			return;
		}
	}
}