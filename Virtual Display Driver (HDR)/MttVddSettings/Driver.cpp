#include "globals.h"
#include <atlcomcli.h>
#include <chrono>
#include <cstdio>
// #include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <shlwapi.h>
#include <sstream>
#include <string>
#include <windows.h>
#include <xmllite.h>

#define PIPE_NAME L"\\\\.\\pipe\\MTTVirtualDisplayPipe"

#pragma comment(lib, "xmllite.lib")
#pragma comment(lib, "shlwapi.lib")

namespace Refactoring
{

void vddlog(const char *type, const char *message);

DriverSettings g_settings;

//std::wstring confpath = L"C:\\VirtualDisplayDriver";
std::wstring confpath = L"C:\\data\\repos\\Sandbox\\Virtual-Display-Driver-Ref\\Virtual Display Driver (HDR)";
std::map<std::wstring, std::wstring> SettingsQueryMap = {
	{L"LoggingEnabled", L"logging"},
	{L"DebugLoggingEnabled", L"debuglogging"},
	{L"CustomEdidEnabled", L"CustomEdid"},

	{L"PreventMonitorSpoof", L"PreventSpoof"},
	{L"EdidCeaOverride", L"EdidCeaOverride"},
	{L"SendLogsThroughPipe", L"SendLogsThroughPipe"},

	// Cursor Begin
	{L"HardwareCursorEnabled", L"HardwareCursor"},
	{L"AlphaCursorSupport", L"AlphaCursorSupport"},
	{L"CursorMaxX", L"CursorMaxX"},
	{L"CursorMaxY", L"CursorMaxY"},
	{L"XorCursorSupportLevel", L"XorCursorSupportLevel"},
	// Cursor End

	// Colour Begin
	{L"HDRPlusEnabled", L"HDRPlus"},
	{L"SDR10Enabled", L"SDR10bit"},
	{L"ColourFormat", L"ColourFormat"},
	// Colour End

	// EDID Integration Begin
	{L"EdidIntegrationEnabled", L"enabled"},
	{L"AutoConfigureFromEdid", L"auto_configure_from_edid"},
	{L"EdidProfilePath", L"edid_profile_path"},
	{L"OverrideManualSettings", L"override_manual_settings"},
	{L"FallbackOnError", L"fallback_on_error"},
	// EDID Integration End

	// HDR Advanced Begin
	{L"Hdr10StaticMetadataEnabled", L"enabled"},
	{L"MaxDisplayMasteringLuminance", L"max_display_mastering_luminance"},
	{L"MinDisplayMasteringLuminance", L"min_display_mastering_luminance"},
	{L"MaxContentLightLevel", L"max_content_light_level"},
	{L"MaxFrameAvgLightLevel", L"max_frame_avg_light_level"},
	{L"ColorPrimariesEnabled", L"enabled"},
	{L"RedX", L"red_x"},
	{L"RedY", L"red_y"},
	{L"GreenX", L"green_x"},
	{L"GreenY", L"green_y"},
	{L"BlueX", L"blue_x"},
	{L"BlueY", L"blue_y"},
	{L"WhiteX", L"white_x"},
	{L"WhiteY", L"white_y"},
	{L"ColorSpaceEnabled", L"enabled"},
	{L"GammaCorrection", L"gamma_correction"},
	{L"PrimaryColorSpace", L"primary_color_space"},
	{L"EnableMatrixTransform", L"enable_matrix_transform"},
	// HDR Advanced End

	// Auto Resolutions Begin
	{L"AutoResolutionsEnabled", L"enabled"},
	{L"SourcePriority", L"source_priority"},
	{L"MinRefreshRate", L"min_refresh_rate"},
	{L"MaxRefreshRate", L"max_refresh_rate"},
	{L"ExcludeFractionalRates", L"exclude_fractional_rates"},
	{L"MinResolutionWidth", L"min_resolution_width"},
	{L"MinResolutionHeight", L"min_resolution_height"},
	{L"MaxResolutionWidth", L"max_resolution_width"},
	{L"MaxResolutionHeight", L"max_resolution_height"},
	{L"UseEdidPreferred", L"use_edid_preferred"},
	{L"FallbackWidth", L"fallback_width"},
	{L"FallbackHeight", L"fallback_height"},
	{L"FallbackRefresh", L"fallback_refresh"},
	// Auto Resolutions End

	// Color Advanced Begin
	{L"AutoSelectFromColorSpace", L"auto_select_from_color_space"},
	{L"ForceBitDepth", L"force_bit_depth"},
	{L"Fp16SurfaceSupport", L"fp16_surface_support"},
	{L"WideColorGamut", L"wide_color_gamut"},
	{L"HdrToneMapping", L"hdr_tone_mapping"},
	{L"SdrWhiteLevel", L"sdr_white_level"},
	// Color Advanced End

	// Monitor Emulation Begin
	{L"MonitorEmulationEnabled", L"enabled"},
	{L"EmulatePhysicalDimensions", L"emulate_physical_dimensions"},
	{L"PhysicalWidthMm", L"physical_width_mm"},
	{L"PhysicalHeightMm", L"physical_height_mm"},
	{L"ManufacturerEmulationEnabled", L"enabled"},
	{L"ManufacturerName", L"manufacturer_name"},
	{L"ModelName", L"model_name"},
	{L"SerialNumber", L"serial_number"},
	// Monitor Emulation End
};

void LogQueries(const char *severity, const std::wstring &xmlName)
{
	if (xmlName.find(L"logging") == std::wstring::npos)
	{
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, xmlName.c_str(), (int)xmlName.size(), NULL, 0, NULL, NULL);
		if (size_needed > 0)
		{
			std::string strMessage(size_needed, 0);
			WideCharToMultiByte(CP_UTF8, 0, xmlName.c_str(), (int)xmlName.size(), &strMessage[0], size_needed, NULL,
								NULL);
			vddlog(severity, strMessage.c_str());
		}
	}
}

std::string WStringToString(const std::wstring &wstr)
{ // basically just a function for converting strings since codecvt is depricated in c++ 17
	if (wstr.empty())
		return "";

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string str(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, NULL, NULL);
	return str;
}

bool EnabledQuery(const std::wstring &settingKey)
{
	auto it = SettingsQueryMap.find(settingKey);
	if (it == SettingsQueryMap.end())
	{
		vddlog("e", "requested data not found in xml, consider updating xml!");
		return false;
	}

	std::wstring xmlName = it->second;

	std::wstring settingsname = confpath + L"\\vdd_settings.xml";
	HKEY hKey;
	DWORD dwValue;
	DWORD dwBufferSize = sizeof(dwValue);
	LONG lResult =
		RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\MikeTheTech\\VirtualDisplayDriver", 0, KEY_READ, &hKey);

	if (lResult == ERROR_SUCCESS)
	{
		std::wstring regName = xmlName;
		CharUpperBuffW(&regName[0], static_cast<DWORD>(regName.length()));
		lResult = RegQueryValueExW(hKey, regName.c_str(), NULL, NULL, (LPBYTE)&dwValue, &dwBufferSize);
		if (lResult == ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			if (dwValue == 1)
			{
				LogQueries("d", xmlName + L" - is enabled (value = 1).");
				return true;
			}
			else if (dwValue == 0)
			{
				goto check_xml;
			}
		}
		else
		{
			LogQueries("d", xmlName + L" - Failed to retrieve value from registry. Attempting to read as string.");
			wchar_t path[MAX_PATH];
			dwBufferSize = sizeof(path);
			lResult = RegQueryValueExW(hKey, regName.c_str(), NULL, NULL, (LPBYTE)path, &dwBufferSize);
			if (lResult == ERROR_SUCCESS)
			{
				std::wstring logValue(path);
				RegCloseKey(hKey);
				if (logValue == L"true" || logValue == L"1")
				{
					LogQueries("d", xmlName + L" - is enabled (string value).");
					return true;
				}
				else if (logValue == L"false" || logValue == L"0")
				{
					goto check_xml;
				}
			}
			RegCloseKey(hKey);
			LogQueries("d", xmlName + L" - Failed to retrieve string value from registry.");
		}
	}

check_xml:
	CComPtr<IStream> pFileStream;
	HRESULT hr =
		SHCreateStreamOnFileEx(settingsname.c_str(), STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &pFileStream);
	if (FAILED(hr))
	{
		LogQueries("d", xmlName + L" - Failed to create file stream for XML settings.");
		return false;
	}

	CComPtr<IXmlReader> pReader;
	hr = CreateXmlReader(__uuidof(IXmlReader), (void **)&pReader, nullptr);
	if (FAILED(hr))
	{
		LogQueries("d", xmlName + L" - Failed to create XML reader.");
		return false;
	}

	hr = pReader->SetInput(pFileStream);
	if (FAILED(hr))
	{
		LogQueries("d", xmlName + L" - Failed to set input for XML reader.");
		return false;
	}

	XmlNodeType nodeType;
	const wchar_t *pwszLocalName;
	bool xmlLoggingValue = false;

	while (S_OK == pReader->Read(&nodeType))
	{
		if (nodeType == XmlNodeType_Element)
		{
			pReader->GetLocalName(&pwszLocalName, nullptr);
			if (pwszLocalName && wcscmp(pwszLocalName, xmlName.c_str()) == 0)
			{
				pReader->Read(&nodeType);
				if (nodeType == XmlNodeType_Text)
				{
					const wchar_t *pwszValue;
					pReader->GetValue(&pwszValue, nullptr);
					if (pwszValue)
					{
						xmlLoggingValue = (wcscmp(pwszValue, L"true") == 0);
					}
					LogQueries("i", xmlName + (xmlLoggingValue ? L" is enabled." : L" is disabled."));
					break;
				}
			}
		}
	}

	return xmlLoggingValue;
}

int GetIntegerSetting(const std::wstring &settingKey)
{
	auto it = SettingsQueryMap.find(settingKey);
	if (it == SettingsQueryMap.end())
	{
		vddlog("e", "requested data not found in xml, consider updating xml!");
		return -1;
	}

	std::wstring xmlName = it->second;

	std::wstring settingsname = confpath + L"\\vdd_settings.xml";
	HKEY hKey;
	DWORD dwValue;
	DWORD dwBufferSize = sizeof(dwValue);
	LONG lResult =
		RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\MikeTheTech\\VirtualDisplayDriver", 0, KEY_READ, &hKey);

	if (lResult == ERROR_SUCCESS)
	{
		std::wstring regName = xmlName;
		CharUpperBuffW(&regName[0], static_cast<DWORD>(regName.length()));
		lResult = RegQueryValueExW(hKey, regName.c_str(), NULL, NULL, (LPBYTE)&dwValue, &dwBufferSize);
		if (lResult == ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			LogQueries("d", xmlName + L" - Retrieved integer value: " + std::to_wstring(dwValue));
			return static_cast<int>(dwValue);
		}
		else
		{
			LogQueries("d",
					   xmlName + L" - Failed to retrieve integer value from registry. Attempting to read as string.");
			wchar_t path[MAX_PATH];
			dwBufferSize = sizeof(path);
			lResult = RegQueryValueExW(hKey, regName.c_str(), NULL, NULL, (LPBYTE)path, &dwBufferSize);
			RegCloseKey(hKey);
			if (lResult == ERROR_SUCCESS)
			{
				try
				{
					int logValue = std::stoi(path);
					LogQueries("d", xmlName + L" - Retrieved string value: " + std::to_wstring(logValue));
					return logValue;
				}
				catch (const std::exception &)
				{
					LogQueries("d", xmlName + L" - Failed to convert registry string value to integer.");
				}
			}
		}
	}

	CComPtr<IStream> pFileStream;
	HRESULT hr =
		SHCreateStreamOnFileEx(settingsname.c_str(), STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &pFileStream);
	if (FAILED(hr))
	{
		LogQueries("d", xmlName + L" - Failed to create file stream for XML settings.");
		return -1;
	}

	CComPtr<IXmlReader> pReader;
	hr = CreateXmlReader(__uuidof(IXmlReader), (void **)&pReader, nullptr);
	if (FAILED(hr))
	{
		LogQueries("d", xmlName + L" - Failed to create XML reader.");
		return -1;
	}

	hr = pReader->SetInput(pFileStream);
	if (FAILED(hr))
	{
		LogQueries("d", xmlName + L" - Failed to set input for XML reader.");
		return -1;
	}

	XmlNodeType nodeType;
	const wchar_t *pwszLocalName;
	int xmlLoggingValue = -1;

	while (S_OK == pReader->Read(&nodeType))
	{
		if (nodeType == XmlNodeType_Element)
		{
			pReader->GetLocalName(&pwszLocalName, nullptr);
			if (pwszLocalName && wcscmp(pwszLocalName, xmlName.c_str()) == 0)
			{
				pReader->Read(&nodeType);
				if (nodeType == XmlNodeType_Text)
				{
					const wchar_t *pwszValue;
					pReader->GetValue(&pwszValue, nullptr);
					if (pwszValue)
					{
						try
						{
							xmlLoggingValue = std::stoi(pwszValue);
							LogQueries("i", xmlName + L" - Retrieved from XML: " + std::to_wstring(xmlLoggingValue));
						}
						catch (const std::exception &)
						{
							LogQueries("d", xmlName + L" - Failed to convert XML string value to integer.");
						}
					}
					break;
				}
			}
		}
	}

	return xmlLoggingValue;
}

std::wstring GetStringSetting(const std::wstring &settingKey)
{
	auto it = SettingsQueryMap.find(settingKey);
	if (it == SettingsQueryMap.end())
	{
		vddlog("e", "requested data not found in xml, consider updating xml!");
		return L"";
	}

	std::wstring xmlName = it->second;

	std::wstring settingsname = confpath + L"\\vdd_settings.xml";
	HKEY hKey;
	DWORD dwBufferSize = MAX_PATH;
	wchar_t buffer[MAX_PATH];

	LONG lResult =
		RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\MikeTheTech\\VirtualDisplayDriver", 0, KEY_READ, &hKey);
	if (lResult == ERROR_SUCCESS)
	{
		std::wstring regName = xmlName;
		CharUpperBuffW(&regName[0], static_cast<DWORD>(regName.length()));
		lResult = RegQueryValueExW(hKey, regName.c_str(), NULL, NULL, (LPBYTE)buffer, &dwBufferSize);
		RegCloseKey(hKey);

		if (lResult == ERROR_SUCCESS)
		{
			LogQueries("d", xmlName + L" - Retrieved string value from registry: " + buffer);
			return std::wstring(buffer);
		}
		else
		{
			LogQueries("d", xmlName + L" - Failed to retrieve string value from registry. Attempting to read as XML.");
		}
	}

	CComPtr<IStream> pFileStream;
	HRESULT hr =
		SHCreateStreamOnFileEx(settingsname.c_str(), STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &pFileStream);
	if (FAILED(hr))
	{
		LogQueries("d", xmlName + L" - Failed to create file stream for XML settings.");
		return L"";
	}

	CComPtr<IXmlReader> pReader;
	hr = CreateXmlReader(__uuidof(IXmlReader), (void **)&pReader, nullptr);
	if (FAILED(hr))
	{
		LogQueries("d", xmlName + L" - Failed to create XML reader.");
		return L"";
	}

	hr = pReader->SetInput(pFileStream);
	if (FAILED(hr))
	{
		LogQueries("d", xmlName + L" - Failed to set input for XML reader.");
		return L"";
	}

	XmlNodeType nodeType;
	const wchar_t *pwszLocalName;
	std::wstring xmlLoggingValue = L"";

	while (S_OK == pReader->Read(&nodeType))
	{
		if (nodeType == XmlNodeType_Element)
		{
			pReader->GetLocalName(&pwszLocalName, nullptr);
			if (pwszLocalName && wcscmp(pwszLocalName, xmlName.c_str()) == 0)
			{
				pReader->Read(&nodeType);
				if (nodeType == XmlNodeType_Text)
				{
					const wchar_t *pwszValue;
					pReader->GetValue(&pwszValue, nullptr);
					if (pwszValue)
					{
						xmlLoggingValue = pwszValue;
					}
					LogQueries("i", xmlName + L" - Retrieved from XML: " + xmlLoggingValue);
					break;
				}
			}
		}
	}

	return xmlLoggingValue;
}

double GetDoubleSetting(const std::wstring &settingKey)
{
	auto it = SettingsQueryMap.find(settingKey);
	if (it == SettingsQueryMap.end())
	{
		vddlog("e", "requested data not found in xml, consider updating xml!");
		return 0.0;
	}

	std::wstring xmlName = it->second;

	std::wstring settingsname = confpath + L"\\vdd_settings.xml";
	HKEY hKey;
	DWORD dwBufferSize = MAX_PATH;
	wchar_t buffer[MAX_PATH];

	LONG lResult =
		RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\MikeTheTech\\VirtualDisplayDriver", 0, KEY_READ, &hKey);
	if (lResult == ERROR_SUCCESS)
	{
		std::wstring regName = xmlName;
		CharUpperBuffW(&regName[0], static_cast<DWORD>(regName.length()));
		lResult = RegQueryValueExW(hKey, regName.c_str(), NULL, NULL, (LPBYTE)buffer, &dwBufferSize);
		if (lResult == ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			try
			{
				double regValue = std::stod(buffer);
				LogQueries("d", xmlName + L" - Retrieved from registry: " + std::to_wstring(regValue));
				return regValue;
			}
			catch (const std::exception &)
			{
				LogQueries("d", xmlName + L" - Failed to convert registry value to double.");
			}
		}
		else
		{
			RegCloseKey(hKey);
		}
	}

	CComPtr<IStream> pFileStream;
	HRESULT hr =
		SHCreateStreamOnFileEx(settingsname.c_str(), STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &pFileStream);
	if (FAILED(hr))
	{
		LogQueries("d", xmlName + L" - Failed to create file stream for XML settings.");
		return 0.0;
	}

	CComPtr<IXmlReader> pReader;
	hr = CreateXmlReader(__uuidof(IXmlReader), (void **)&pReader, nullptr);
	if (FAILED(hr))
	{
		LogQueries("d", xmlName + L" - Failed to create XML reader.");
		return 0.0;
	}

	hr = pReader->SetInput(pFileStream);
	if (FAILED(hr))
	{
		LogQueries("d", xmlName + L" - Failed to set input for XML reader.");
		return 0.0;
	}

	XmlNodeType nodeType;
	const wchar_t *pwszLocalName;
	double xmlLoggingValue = 0.0;

	while (S_OK == pReader->Read(&nodeType))
	{
		if (nodeType == XmlNodeType_Element)
		{
			pReader->GetLocalName(&pwszLocalName, nullptr);
			if (pwszLocalName && wcscmp(pwszLocalName, xmlName.c_str()) == 0)
			{
				pReader->Read(&nodeType);
				if (nodeType == XmlNodeType_Text)
				{
					const wchar_t *pwszValue;
					pReader->GetValue(&pwszValue, nullptr);
					if (pwszValue)
					{
						try
						{
							xmlLoggingValue = std::stod(pwszValue);
							LogQueries("i", xmlName + L" - Retrieved from XML: " + std::to_wstring(xmlLoggingValue));
						}
						catch (const std::exception &)
						{
							LogQueries("d", xmlName + L" - Failed to convert XML value to double.");
						}
					}
					break;
				}
			}
		}
	}

	return xmlLoggingValue;
}

void vddlog(const char *type, const char *message)
{
	if (!g_settings.logs.enable_standard_logs)
	{
		return;
	}

	if (type != nullptr && type[0] == 'd' && !g_settings.logs.enable_debug_logs)
	{
		return;
	}

	FILE *logFile;
	std::wstring logsDir = confpath + L"\\Logs";

	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	tm tm_buf;
	localtime_s(&tm_buf, &in_time_t);
	wchar_t date_str[11];
	wcsftime(date_str, sizeof(date_str) / sizeof(wchar_t), L"%Y-%m-%d", &tm_buf);

	std::wstring logPath = logsDir + L"\\log_" + date_str + L".txt";

	if (!CreateDirectoryW(logsDir.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
	{
		// Best effort only.
	}

	std::string narrow_logPath = WStringToString(logPath);
	const char *mode = "a";
	errno_t err = fopen_s(&logFile, narrow_logPath.c_str(), mode);
	if (err == 0 && logFile != nullptr)
	{
		std::stringstream ss;
		ss << std::put_time(&tm_buf, "%Y-%m-%d %X");

		const char logTypeCode = (type != nullptr) ? type[0] : '\0';
		std::string logType;
		switch (logTypeCode)
		{
		case 'e':
			logType = "ERROR";
			break;
		case 'i':
			logType = "INFO";
			break;
		case 'p':
			logType = "PIPE";
			break;
		case 'd':
			logType = "DEBUG";
			break;
		case 'w':
			logType = "WARNING";
			break;
		case 't':
			logType = "TESTING";
			break;
		case 'c':
			logType = "COMPANION";
			break;
		default:
			logType = "UNKNOWN";
			break;
		}

		fprintf(logFile, "[%s] [%s] %s\n", ss.str().c_str(), logType.c_str(), message);

		fclose(logFile);

		std::string logMessage = ss.str() + " [" + logType + "] " + message + "\n";
		std::cout << logMessage;
	}
}

bool initpath()
{
	HKEY hKey;
	wchar_t szPath[MAX_PATH];
	DWORD dwBufferSize = sizeof(szPath);
	LONG lResult;
	lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\MikeTheTech\\VirtualDisplayDriver", 0, KEY_READ, &hKey);
	if (lResult != ERROR_SUCCESS)
	{
		std::ostringstream oss;
		oss << "Failed to open registry key for path. Error code: " << lResult;
		return false;
	}

	lResult = RegQueryValueExW(hKey, L"VDDPATH", NULL, NULL, (LPBYTE)szPath, &dwBufferSize);
	if (lResult != ERROR_SUCCESS)
	{
		std::ostringstream oss;
		oss << "Failed to open registry key for path. Error code: " << lResult;
		RegCloseKey(hKey);
		return false;
	}

	confpath = szPath;

	RegCloseKey(hKey);

	return true;
}

bool GetSettings()
{
	initpath();
	g_settings.logs.enable_standard_logs = EnabledQuery(L"LoggingEnabled");
	g_settings.logs.enable_debug_logs = EnabledQuery(L"DebugLoggingEnabled");

	g_settings.edid.custom_edid = EnabledQuery(L"CustomEdidEnabled");
	g_settings.edid.prevent_manufacturer_spoof = EnabledQuery(L"PreventMonitorSpoof");
	g_settings.edid.edid_cea_override = EnabledQuery(L"EdidCeaOverride");
	g_settings.logs.send_logs_through_pipe = EnabledQuery(L"SendLogsThroughPipe");

	// colour
	g_settings.colors.hdr_plus = EnabledQuery(L"HDRPlusEnabled");
	g_settings.colors.sdr10 = EnabledQuery(L"SDR10Enabled");
	// g_settings.colors.HDR_COLOR = g_settings.colors.hdr_plus ? IDDCX_BITS_PER_COMPONENT_12 :
	// IDDCX_BITS_PER_COMPONENT_10; g_settings.colors.SDR_COLOR = g_settings.colors.sdr10 ? IDDCX_BITS_PER_COMPONENT_10
	// : IDDCX_BITS_PER_COMPONENT_8;
	g_settings.colors.color_format = GetStringSetting(L"ColourFormat");

	// Cursor
	g_settings.cursor.hardware_cursor = EnabledQuery(L"HardwareCursorEnabled");
	g_settings.cursor.alpha_cursor_support = EnabledQuery(L"AlphaCursorSupport");
	g_settings.cursor.max_x = GetIntegerSetting(L"CursorMaxX");
	g_settings.cursor.max_y = GetIntegerSetting(L"CursorMaxY");

	int xorCursorSupportLevelInt = GetIntegerSetting(L"XorCursorSupportLevel");
	std::string xorCursorSupportLevelName;

	// if (xorCursorSupportLevelInt < 0 || xorCursorSupportLevelInt > 3) {
	//	vddlog("w", "Selected Xor Level unsupported, defaulting to IDDCX_XOR_CURSOR_SUPPORT_FULL");
	//	g_settings.cursor.xor_cursor_support_level = IDDCX_XOR_CURSOR_SUPPORT_FULL;
	// }
	// else {
	//	g_settings.cursor.xor_cursor_support_level = static_cast<IDDCX_XOR_CURSOR_SUPPORT>(xorCursorSupportLevelInt);
	// }

	// === LOAD NEW EDID INTEGRATION SETTINGS ===
	g_settings.edid.enabled = EnabledQuery(L"EdidIntegrationEnabled");
	g_settings.edid.auto_configure = EnabledQuery(L"AutoConfigureFromEdid");
	g_settings.edid.profile_path = GetStringSetting(L"EdidProfilePath");
	g_settings.edid.override_manual_settings = EnabledQuery(L"OverrideManualSettings");
	g_settings.edid.fallback_on_error = EnabledQuery(L"FallbackOnError");
	g_settings.edid.preferred = EnabledQuery(L"UseEdidPreferred");

	// === LOAD HDR ADVANCED SETTINGS ===
	g_settings.hdr.static_metadata_enabled = EnabledQuery(L"Hdr10StaticMetadataEnabled");
	g_settings.hdr.max_display_mastering_luminance = GetDoubleSetting(L"MaxDisplayMasteringLuminance");
	g_settings.hdr.min_display_mastering_luminance = GetDoubleSetting(L"MinDisplayMasteringLuminance");
	g_settings.hdr.max_content_light_level = GetIntegerSetting(L"MaxContentLightLevel");
	g_settings.hdr.max_frame_avg_light_level = GetIntegerSetting(L"MaxFrameAvgLightLevel");
	g_settings.hdr.matrix_transform_enabled = EnabledQuery(L"EnableMatrixTransform");

	g_settings.colors.primaries_enabled = EnabledQuery(L"ColorPrimariesEnabled");
	g_settings.colors.defaults.redX = GetDoubleSetting(L"RedX");
	g_settings.colors.defaults.redY = GetDoubleSetting(L"RedY");
	g_settings.colors.defaults.greenX = GetDoubleSetting(L"GreenX");
	g_settings.colors.defaults.greenY = GetDoubleSetting(L"GreenY");
	g_settings.colors.defaults.blueX = GetDoubleSetting(L"BlueX");
	g_settings.colors.defaults.blueY = GetDoubleSetting(L"BlueY");
	g_settings.colors.defaults.whiteX = GetDoubleSetting(L"WhiteX");
	g_settings.colors.defaults.whiteY = GetDoubleSetting(L"WhiteY");

	g_settings.colors.color_space_enabled = EnabledQuery(L"ColorSpaceEnabled");
	g_settings.colors.gamma_correction = GetDoubleSetting(L"GammaCorrection");
	g_settings.colors.primary_color_space = GetStringSetting(L"PrimaryColorSpace");

	// === LOAD AUTO RESOLUTIONS SETTINGS ===
	g_settings.auto_res.enabled = EnabledQuery(L"AutoResolutionsEnabled");
	g_settings.auto_res.source_priority = GetStringSetting(L"SourcePriority");
	g_settings.auto_res.min_refresh_rate = GetIntegerSetting(L"MinRefreshRate");
	g_settings.auto_res.max_refresh_rate = GetIntegerSetting(L"MaxRefreshRate");
	g_settings.auto_res.exclude_fractional_rates = EnabledQuery(L"ExcludeFractionalRates");
	g_settings.auto_res.min_resolution_width = GetIntegerSetting(L"MinResolutionWidth");
	g_settings.auto_res.min_resolution_height = GetIntegerSetting(L"MinResolutionHeight");
	g_settings.auto_res.max_resolution_width = GetIntegerSetting(L"MaxResolutionWidth");
	g_settings.auto_res.max_resolution_height = GetIntegerSetting(L"MaxResolutionHeight");
	g_settings.auto_res.fallback_width = GetIntegerSetting(L"FallbackWidth");
	g_settings.auto_res.fallback_height = GetIntegerSetting(L"FallbackHeight");
	g_settings.auto_res.fallback_refresh = GetIntegerSetting(L"FallbackRefresh");

	// === LOAD COLOR ADVANCED SETTINGS ===
	g_settings.colors.auto_select_from_color_space = EnabledQuery(L"AutoSelectFromColorSpace");
	g_settings.colors.force_bit_depth = GetStringSetting(L"ForceBitDepth");
	g_settings.colors.fp16_surface_support = EnabledQuery(L"Fp16SurfaceSupport");
	g_settings.colors.wide_color_gamut = EnabledQuery(L"WideColorGamut");
	g_settings.colors.hdr_tone_mapping = EnabledQuery(L"HdrToneMapping");
	g_settings.colors.sdr_white_level = GetDoubleSetting(L"SdrWhiteLevel");

	// === LOAD MONITOR EMULATION SETTINGS ===
	g_settings.mon_emul.enabled = EnabledQuery(L"MonitorEmulationEnabled");
	g_settings.mon_emul.emulate_physical_dimensions = EnabledQuery(L"EmulatePhysicalDimensions");
	g_settings.mon_emul.physical_width = GetIntegerSetting(L"PhysicalWidthMm");
	g_settings.mon_emul.physical_height = GetIntegerSetting(L"PhysicalHeightMm");
	g_settings.mon_emul.manufacturer_emulation_enabled = EnabledQuery(L"ManufacturerEmulationEnabled");
	g_settings.mon_emul.manufacturer_name = GetStringSetting(L"ManufacturerName");
	g_settings.mon_emul.model_name = GetStringSetting(L"ModelName");
	g_settings.mon_emul.serial_number = GetStringSetting(L"SerialNumber");

	// xorCursorSupportLevelName = XorCursorSupportLevelToString(g_settings.cursor.xor_cursor_support_level);

	vddlog("i", ("Selected Xor Cursor Support Level: " + xorCursorSupportLevelName).c_str());

	vddlog("i", "Driver Starting");
	std::string utf8_confpath = WStringToString(confpath);
	std::string logtext = "VDD Path: " + utf8_confpath;
	vddlog("i", logtext.c_str());

	return true;
}
} // namespace Refactoring
