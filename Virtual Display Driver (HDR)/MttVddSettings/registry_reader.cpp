#include "registry_reader.h"
#include <iostream>


bool Refactoring::RegistryReader::OpenRegistry()
{
	reg_handle_key = nullptr;
	LONG lResult =
		RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\MikeTheTech\\VirtualDisplayDriver", 0, KEY_READ, &reg_handle_key);

	if (lResult == ERROR_SUCCESS)
	{
		return true;
	}
	return false;
}

bool Refactoring::RegistryReader::CloseRegistry()
{
	LONG lResult = RegCloseKey(reg_handle_key);

	if (lResult == ERROR_SUCCESS)
	{
		reg_handle_key = nullptr;
		return true;
	}
	return false;
}

bool Refactoring::RegistryReader::IsRegistryOpen() const
{
	return reg_handle_key != nullptr;
}

void Refactoring::RegistryReader::InitializePath(std::string &path) const
{
	if (!IsRegistryOpen())
		return;

	DWORD dwBufferSize = sizeof(path);
	LONG lResult;

	lResult = RegQueryValueExW(reg_handle_key, L"VDDPATH", NULL, NULL, (LPBYTE)&path[0], &dwBufferSize);
	if (lResult == ERROR_SUCCESS)
	{
		std::cout << "Config Path updated: " + path;
		return;
	}

	std::cout << "Failed to open registry key for vdd path override. Error code: " << lResult;
	std::cout << "Config Path remains at default value.";
}

std::string Refactoring::RegistryReader::GetRawRegistryValue(HKEY hKey, const std::vector<std::string> &setting)
{
	std::string reg_name = setting[0];
	CharUpperBuff(&reg_name[0], static_cast<DWORD>(reg_name.size()));

	std::string key = setting[1];
	CharUpperBuff(&key[0], static_cast<DWORD>(key.size()));

	DWORD type = 0;
	DWORD buffer_size = 0;

	LONG lResult = RegGetValue(hKey, reg_name.c_str(), key.c_str(), 0, &type, NULL, &buffer_size);
	if (lResult != ERROR_SUCCESS)
		return "";

	if (type == REG_DWORD)
	{
		DWORD value = 0;
		buffer_size = sizeof(value);
		RegGetValue(hKey, reg_name.c_str(), key.c_str(), 0, NULL, (LPBYTE)&value, &buffer_size);
		return std::to_string(value); // 1 → L"1", 0 → L"0"
	}
	else if (type == REG_SZ)
	{
		std::string value(buffer_size / sizeof(char), L'\0');
		RegGetValue(hKey, reg_name.c_str(), key.c_str(), 0, NULL, (LPBYTE)&value[0], &buffer_size);
		return value;
	}

	return "";
}