#include "registry_reader.h"
#include <iostream>
#include <algorithm>


bool Refactoring::RegistryReader::OpenRegistry()
{
	reg_handle_key = nullptr;
	LONG lResult =
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\MikeTheTech\\VirtualDisplayDriver", 0, KEY_READ, &reg_handle_key);

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

	DWORD dwBufferSize = 0;
	LONG lResult;

	lResult = RegGetValue(reg_handle_key, "", "VDDPATH", RRF_RT_REG_SZ, NULL, NULL, &dwBufferSize);

	if (lResult != ERROR_SUCCESS)
	{
		std::cout << "Failed to open registry key for vdd path override. Error code: " << lResult;
		std::cout << "Config Path remains at default value.";
		return;
	}
	if (dwBufferSize == 0)
	{
		std::cout << "Config Path was not updated. VDDPATH present in registry, but value is empty.";
		return;
	}

	path.resize(dwBufferSize - 1);

	lResult = RegGetValue(reg_handle_key, "", "VDDPATH", RRF_RT_REG_SZ, NULL, (LPBYTE)&path[0], &dwBufferSize);
	if (lResult == ERROR_SUCCESS)
	{
		std::cout << "Config Path updated: " + path;
		return;
	}
}

std::string Refactoring::RegistryReader::GetRawRegistryValue(HKEY hKey, const std::string &setting)
{
	std::string reg_name = setting;

	std::replace(reg_name.begin(), reg_name.end(), '.', '_');
	CharUpperBuff(reg_name.data(), static_cast<DWORD>(reg_name.size()));

	DWORD type = 0;
	DWORD buffer_size = 0;

	LONG lResult = RegGetValue(hKey, "", reg_name.c_str(), 0, &type, NULL, &buffer_size);
	if (lResult != ERROR_SUCCESS)
		return "";

	if (type == REG_DWORD)
	{
		DWORD value = 0;
		buffer_size = sizeof(value);
		lResult = RegGetValue(hKey, "", reg_name.c_str(), 0, NULL, (LPBYTE)&value, &buffer_size);
		if (lResult != ERROR_SUCCESS)
			return "";

		return std::to_string(value); // 1 → "1", 0 → "0"
	}
	else if (type == REG_SZ)
	{
		std::string value(buffer_size - 1, '\0');
		lResult = RegGetValue(hKey, "", reg_name.c_str(), 0, NULL, (LPBYTE)&value[0], &buffer_size);
		if (lResult != ERROR_SUCCESS)
			return "";
		return value;
	}

	return "";
}

bool Refactoring::RegistryReader::GetSetting(std::string value_key, const SettingValuePtr &result)
{
	std::string raw_reg_value = GetRawRegistryValue(reg_handle_key, value_key);

	if (raw_reg_value.empty())
		return false;

	std::visit(
		[&raw_reg_value](auto *ptr) {
			using T = std::remove_pointer_t<decltype(ptr)>;
			*ptr = convert_setting<T>(raw_reg_value);
		},
		result);
	return true;
}