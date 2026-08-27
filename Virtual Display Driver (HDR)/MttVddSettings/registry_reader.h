#pragma once
#include <Windows.h>
#include <string>
#include "utilities.h"
// #include <iostream>

namespace Refactoring
{
class RegistryReader
{
  public:
	bool OpenRegistry();
	bool CloseRegistry();
	bool IsRegistryOpen() const;

	void InitializePath(std::wstring path);

	template <typename T> bool GetSetting(const std::wstring &parent, const std::wstring &setting_name, T &result)
	{
		std::wstring complete_reg_name = parent + L"_" + setting_name;
		std::wstring raw_reg_value = GetRawRegistryValue(reg_handle_key, complete_reg_name);

		if (raw_reg_value.empty())
			return false;

		result = convert_setting<T>(raw_reg_value);
		return true;
	}

  protected:
  private:
	std::wstring GetRawRegistryValue(HKEY hKey, const std::wstring &setting_name);
	HKEY reg_handle_key;
	bool registry_open;
};
} // namespace Refactoring