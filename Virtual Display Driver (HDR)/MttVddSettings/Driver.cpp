#include "globals.h"
#include "utilities.h"
#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>

namespace Refactoring
{

void override_defaults();

DriverSettings g_settings;

std::wstring confpath = L"C:\\data\\repos\\Sandbox\\Virtual-Display-Driver-Ref\\Virtual Display Driver (HDR)";

static bool InitializePath()
{
	HKEY hKey;
	wchar_t szPath[MAX_PATH];
	DWORD dwBufferSize = sizeof(szPath);
	LONG lResult;
	std::ostringstream oss;
	oss << "Config Path is at default value.";

	lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\MikeTheTech\\VirtualDisplayDriver", 0, KEY_READ, &hKey);

	if (lResult != ERROR_SUCCESS)
	{
		std::ostringstream oss;
		oss << "Failed to open registry key for path. Error code: " << lResult;
		oss << "Registry will not be analyzed";
		return true;
	}

	lResult = RegQueryValueExW(hKey, L"VDDPATH", NULL, NULL, (LPBYTE)szPath, &dwBufferSize);
	if (lResult != ERROR_SUCCESS)
	{

		oss << "Failed to open registry key for vdd path override. Error code: " << lResult;
		oss << "Config Path remains at default value.";
		RegCloseKey(hKey);
		return true;
	}

	confpath = szPath;

	RegCloseKey(hKey);

	return true;
}

/* parte mia */
std::wstring get_raw_registry_value(HKEY hKey, const std::wstring &setting_name)
{
	std::wstring reg_name = setting_name;
	CharUpperBuffW(&reg_name[0], static_cast<DWORD>(reg_name.length()));

	DWORD type = 0;
	DWORD buffer_size = 0;

	LONG lResult = RegQueryValueExW(hKey, setting_name.c_str(), NULL, &type, NULL, &buffer_size);
	if (lResult != ERROR_SUCCESS)
		return L"";

	if (type == REG_DWORD)
	{
		DWORD value = 0;
		buffer_size = sizeof(value);
		RegQueryValueExW(hKey, reg_name.c_str(), NULL, NULL, (LPBYTE)&value, &buffer_size);
		return std::to_wstring(value); // 1 → L"1", 0 → L"0"
	}
	else if (type == REG_SZ)
	{
		std::wstring value(buffer_size / sizeof(wchar_t), L'\0');
		RegQueryValueExW(hKey, reg_name.c_str(), NULL, NULL, (LPBYTE)&value[0], &buffer_size);
		return value;
	}

	return L"";
}


template <typename T> 
bool get_setting(HKEY hKey, const std::wstring& parent, const std::wstring& setting_name, T& result)
{
	std::wstring complete_reg_name = parent + L"_" + setting_name;
	std::wstring raw_reg_value = get_raw_registry_value(hKey, complete_reg_name);

	if (raw_reg_value.empty())
		return false;
	
	result = convert_setting<T>(raw_reg_value);
	return true;
}

void read_xml()
{
	HKEY xml_hKey = nullptr;
	std::wstring file_name = confpath + L"\\vdd_setting.xml";

	auto lpResult = CreateFile(file_name.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, &xml_hKey);

}


void override_defaults()
{
	HKEY reg_hKey;

	LONG lResult =
		RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\MikeTheTech\\VirtualDisplayDriver", 0, KEY_READ, &reg_hKey);

	get_setting<bool>(reg_hKey, L"logging", L"logging", g_settings.logs.enable_standard_logs);
	get_setting<bool>(reg_hKey, L"logging", L"debuglogging", g_settings.logs.enable_debug_logs);
	get_setting<bool>(reg_hKey, L"logging", L"SendLogsThroughPipe", g_settings.logs.send_logs_through_pipe);

	get_setting<bool>(reg_hKey, L"edid", L"CustomEdid", g_settings.edid.custom_edid);
	get_setting<bool>(reg_hKey, L"edid", L"PreventSpoof", g_settings.edid.prevent_manufacturer_spoof);
	get_setting<bool>(reg_hKey, L"edid", L"EdidCeaOverride", g_settings.edid.edid_cea_override);

	// colour
	get_setting<bool>(reg_hKey, L"colour", L"HDRPlus", g_settings.colours.hdr_plus);
	get_setting<bool>(reg_hKey, L"colour", L"SDR10bit", g_settings.colours.sdr10);
	// g_settings.colors.HDR_COLOR = g_settings.colors.hdr_plus ? IDDCX_BITS_PER_COMPONENT_12 :
	// IDDCX_BITS_PER_COMPONENT_10; g_settings.colors.SDR_COLOR = g_settings.colors.sdr10 ? IDDCX_BITS_PER_COMPONENT_10
	// : IDDCX_BITS_PER_COMPONENT_8;
	get_setting<std::wstring>(reg_hKey, L"colour", L"ColourFormat", g_settings.colours.color_format);

	// Cursor
	get_setting<bool>(reg_hKey, L"cursor", L"HardwareCursor", g_settings.cursor.hardware_cursor);
	get_setting<bool>(reg_hKey, L"cursor", L"AlphaCursorSupport", g_settings.cursor.alpha_cursor_support);
	get_setting<int>(reg_hKey, L"cursor", L"CursorMaxX", g_settings.cursor.max_x);
	get_setting<int>(reg_hKey, L"cursor", L"CursorMaxY", g_settings.cursor.max_y);

	//int xorCursorSupportLevelInt = GetSetting<int>(hKey, L"XorCursorSupportLevel", );
	//std::string xorCursorSupportLevelName;

	// if (xorCursorSupportLevelInt < 0 || xorCursorSupportLevelInt > 3) {
	//	vddlog("w", "Selected Xor Level unsupported, defaulting to IDDCX_XOR_CURSOR_SUPPORT_FULL");
	//	g_settings.cursor.xor_cursor_support_level = IDDCX_XOR_CURSOR_SUPPORT_FULL;
	// }
	// else {
	//	g_settings.cursor.xor_cursor_support_level = static_cast<IDDCX_XOR_CURSOR_SUPPORT>(xorCursorSupportLevelInt);
	// }
	
	// === LOAD NEW EDID INTEGRATION SETTINGS ===
	get_setting<std::wstring>(reg_hKey, L"edid_integration", L"edid_profile_path",
							  g_settings.edid_integration.profile_path);
	get_setting<bool>(reg_hKey, L"edid_integration", L"enabled", g_settings.edid_integration.enabled);
	get_setting<bool>(reg_hKey, L"edid_integration", L"auto_configure_from_edid",
					  g_settings.edid_integration.auto_configure);
	get_setting<bool>(reg_hKey, L"edid_integration", L"override_manual_settings",
					  g_settings.edid_integration.override_manual_settings);
	get_setting<bool>(reg_hKey, L"edid_integration", L"fallback_on_error",
					  g_settings.edid_integration.fallback_on_error);

	// === LOAD HDR ADVANCED SETTINGS ===
	get_setting<bool>(reg_hKey, L"color_space", L"enable_matrix_transform",
					  g_settings.hdr_advanced.color_space.enable_matrix_transform);
	get_setting<bool>(reg_hKey, L"hdr10_static_metadata", L"enabled", g_settings.hdr_advanced.static_metadata_enabled);
	get_setting<double>(reg_hKey, L"hdr10_static_metadata", L"max_display_mastering_luminance", g_settings.hdr_advanced.max_display_mastering_luminance); //sta sotto hdr_advanced/hdr10_static_metadata
	get_setting<double>(reg_hKey, L"hdr10_static_metadata", L"min_display_mastering_luminance", g_settings.hdr_advanced.min_display_mastering_luminance); //sta sotto hdr_advanced/hdr10_static_metadata
	get_setting<int>(reg_hKey, L"hdr10_static_metadata", L"max_content_light_level", g_settings.hdr_advanced.max_content_light_level); //sta sotto hdr_advanced/hdr10_static_metadata
	get_setting<int>(reg_hKey, L"hdr10_static_metadata", L"max_frame_avg_light_level", g_settings.hdr_advanced.max_frame_avg_light_level); //sta sotto hdr_advanced/hdr10_static_metadata

	// === LOAD AUTO RESOLUTIONS SETTINGS ===
	get_setting<std::wstring>(reg_hKey, L"auto_resolutions", L"source_priority", g_settings.auto_resolutions.source_priority);
	get_setting<bool>(reg_hKey, L"auto_resolutions", L"enabled", g_settings.auto_resolutions.enabled);

	get_setting<bool>(reg_hKey, L"edid_mode_filtering", L"exclude_fractional_rates",
					  g_settings.auto_resolutions.edid_mode_filtering
						  .exclude_fractional_rates); // sta sotto auto_resolutions/edid_mode_filtering
	get_setting<int>(reg_hKey, L"edid_mode_filtering", L"min_refresh_rate",
					 g_settings.auto_resolutions.edid_mode_filtering
						 .min_refresh_rate); // sta sotto auto_resolutions/edid_mode_filtering
	get_setting<int>(reg_hKey, L"edid_mode_filtering", L"max_refresh_rate",
					 g_settings.auto_resolutions.edid_mode_filtering
						 .max_refresh_rate); // sta sotto auto_resolutions/edid_mode_filtering
	get_setting<int>(reg_hKey, L"edid_mode_filtering", L"min_resolution_width", g_settings.auto_resolutions.edid_mode_filtering.min_resolution_width); //sta sotto auto_resolutions/edid_mode_filtering
	get_setting<int>(reg_hKey, L"edid_mode_filtering", L"min_resolution_height",
					 g_settings.auto_resolutions.edid_mode_filtering
						 .min_resolution_height); // sta sotto auto_resolutions/edid_mode_filtering
	get_setting<int>(reg_hKey, L"edid_mode_filtering", L"max_resolution_width",
					 g_settings.auto_resolutions.edid_mode_filtering
						 .max_resolution_width); // sta sotto auto_resolutions/edid_mode_filtering
	get_setting<int>(reg_hKey, L"edid_mode_filtering", L"max_resolution_height",
					 g_settings.auto_resolutions.edid_mode_filtering
						 .max_resolution_height); // sta sotto auto_resolutions/edid_mode_filtering

	get_setting<bool>(reg_hKey, L"preferred_mode", L"use_edid_preferred", g_settings.auto_resolutions.preferred_mode.preferred);
	get_setting<int>(
		reg_hKey, L"preferred_mode", L"fallback_width",
		g_settings.auto_resolutions.preferred_mode.fallback_width); // sta sotto auto_resolutions/preferred_mode
	get_setting<int>(reg_hKey, L"preferred_mode", L"fallback_height",
					 g_settings.auto_resolutions.preferred_mode.fallback_height);
	get_setting<int>(reg_hKey, L"preferred_mode", L"fallback_refresh",
					 g_settings.auto_resolutions.preferred_mode.fallback_refresh);
	//manca use_edid_preferred di auto_res - errore dell'xml? (c'è anche in un altro nodo)

	// === LOAD COLOR ADVANCED SETTINGS ===
	get_setting<std::wstring>(reg_hKey, L"bit_depth_management", L"force_bit_depth",
							  g_settings.color_advanced.bit_depth_management
								  .force_bit_depth); // stanno in color_advanced/bit_depth_management
	get_setting<bool>(reg_hKey, L"bit_depth_management", L"auto_select_from_color_space",
					  g_settings.color_advanced.bit_depth_management
						  .auto_select_from_color_space); // sta in color_advanced/bit_depth_management
	get_setting<bool>(reg_hKey, L"bit_depth_management", L"fp16_surface_support",
					  g_settings.color_advanced.bit_depth_management
						  .fp16_surface_support); // sta in color_advanced/bit_depth_management
	get_setting<bool>(reg_hKey, L"not_implemented", L"not_implemented", g_settings.color_advanced.color_format_extended.wide_color_gamut); //not implemented
	get_setting<bool>(reg_hKey, L"not_implemented", L"not_implemented", g_settings.color_advanced.color_format_extended.hdr_tone_mapping); //not 

	get_setting<std::wstring>(reg_hKey, L"color_space", L"primary_color_space", g_settings.hdr_advanced.color_space.primary_color_space); // sta in hdr_advanced/color_space
	get_setting<bool>(reg_hKey, L"color_space", L"enabled", g_settings.hdr_advanced.color_space.enabled); //sta in hdr_advanced/color_space
	get_setting<double>(reg_hKey, L"color_space", L"gamma_correction",
						g_settings.hdr_advanced.color_space.gamma_correction); // sta in hdr_advanced/color_space

	get_setting<bool>(reg_hKey, L"color_primaries", L"enabled", g_settings.hdr_advanced.color_primaries.primaries_enabled); //sta in hdr_advanced/color_primaries
	get_setting<double>(reg_hKey, L"color_primaries", L"RedX", g_settings.hdr_advanced.color_primaries.redX); //sta in hdr_advanced/color_primaries
	get_setting<double>(reg_hKey, L"color_primaries", L"RedY", g_settings.hdr_advanced.color_primaries.redY); //sta in hdr_advanced/color_primaries
	get_setting<double>(reg_hKey, L"color_primaries", L"GreenX", g_settings.hdr_advanced.color_primaries.greenX); //sta in hdr_advanced/color_primaries
	get_setting<double>(reg_hKey, L"color_primaries", L"GreenY", g_settings.hdr_advanced.color_primaries.greenY); //sta in hdr_advanced/color_primaries
	get_setting<double>(reg_hKey, L"color_primaries", L"BlueX", g_settings.hdr_advanced.color_primaries.blueX); //sta in hdr_advanced/color_primaries
	get_setting<double>(reg_hKey, L"color_primaries", L"BlueY", g_settings.hdr_advanced.color_primaries.blueY); //sta in hdr_advanced/color_primaries
	get_setting<double>(reg_hKey, L"color_primaries", L"WhiteX", g_settings.hdr_advanced.color_primaries.whiteX); //sta in hdr_advanced/color_primaries
	get_setting<double>(reg_hKey, L"color_primaries", L"WhiteY", g_settings.hdr_advanced.color_primaries.whiteY); //sta in hdr_advanced/color_primaries

	get_setting<double>(reg_hKey, L"color_format_extended", L"sdr_white_level", g_settings.color_advanced.color_format_extended.sdr_white_level); //sta in color_advanced/color_format_extended

	// === LOAD MONITOR EMULATION SETTINGS ===
	// 
	// loaded from defaults but not implemented
	get_setting<bool>(reg_hKey, L"not_implemented", L"not_implemented", g_settings.monitor_emulation.manufacturer_emulation_enabled);
	get_setting<bool>(reg_hKey, L"not_implemented", L"not_implemented", g_settings.monitor_emulation.enabled);
	get_setting<bool>(reg_hKey, L"not_implemented", L"not_implemented", g_settings.monitor_emulation.emulate_physical_dimensions);
	get_setting<int>(reg_hKey, L"not_implemented", L"not_implemented", g_settings.monitor_emulation.physical_width);
	get_setting<int>(reg_hKey, L"not_implemented", L"not_implemented", g_settings.monitor_emulation.physical_height);

	get_setting<std::wstring>(reg_hKey, L"not_implemented", L"not_implemented", g_settings.monitor_emulation.manufacturer_name);
	get_setting<std::wstring>(reg_hKey, L"not_implemented", L"not_implemented", g_settings.monitor_emulation.model_name);
	get_setting<std::wstring>(reg_hKey, L"not_implemented", L"not_implemented", g_settings.monitor_emulation.serial_number);

	// xorCursorSupportLevelName = XorCursorSupportLevelToString(g_settings.cursor.xor_cursor_support_level);
	//vddlog("i", ("Selected Xor Cursor Support Level: " + xorCursorSupportLevelName).c_str());
	
	//vddlog("i", "Driver Starting");
	//std::string utf8_confpath = WStringToString(confpath);
	//std::string logtext = "VDD Path: " + utf8_confpath;
	//vddlog("i", logtext.c_str());

	RegCloseKey(reg_hKey);
}

} // namespace Refactoring
