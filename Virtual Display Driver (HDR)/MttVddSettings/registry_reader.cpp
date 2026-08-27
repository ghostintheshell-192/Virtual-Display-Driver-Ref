#include "registry_reader.h"
#include <iostream>


bool Refactoring::RegistryReader::OpenRegistry()
{
	LONG lResult;
	lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\MikeTheTech\\VirtualDisplayDriver", 0, KEY_READ, &reg_handle_key);

	if (lResult == ERROR_SUCCESS)
	{
		registry_open = true;
		return true;
	}
	registry_open = false;
	return false;
}

bool Refactoring::RegistryReader::CloseRegistry()
{
	LONG lResult;
	lResult = RegCloseKey(reg_handle_key);

	if (lResult == ERROR_SUCCESS)
	{
		registry_open = false;
		return true;
	}
	registry_open = true;
	return false;
}

bool Refactoring::RegistryReader::IsRegistryOpen() const
{
	return registry_open;
}

void Refactoring::RegistryReader::InitializePath(std::wstring path)
{
	OpenRegistry();
	char * szPath[MAX_PATH];
	DWORD dwBufferSize = sizeof(szPath);
	LONG lResult;

	lResult = RegQueryValueExW(reg_handle_key, L"VDDPATH", NULL, NULL, (LPBYTE)path[0], &dwBufferSize);
	if (lResult == ERROR_SUCCESS)
	{
		std::wcout << L"Config Path updated: " + path;
	}

	std::cout << "Failed to open registry key for vdd path override. Error code: " << lResult;
	std::cout << "Config Path remains at default value.";
	CloseRegistry();
}

std::wstring Refactoring::RegistryReader::GetRawRegistryValue(HKEY hKey, const std::wstring &setting_name)
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

//template <typename T>
//bool Refactoring::RegistryReader::GetSetting(const std::wstring &parent, const std::wstring &setting_name, T &result)
//{
//	std::wstring complete_reg_name = parent + L"_" + setting_name;
//	std::wstring raw_reg_value = GetRawRegistryValue(reg_handle_key, complete_reg_name);
//
//	if (raw_reg_value.empty())
//		return false;
//
//	result = convert_setting<T>(raw_reg_value);
//	return true;
//}