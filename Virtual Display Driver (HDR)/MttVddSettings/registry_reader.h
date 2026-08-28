#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include "utilities.h"
// #include <iostream>

namespace Refactoring
{
class RegistryReader
{
  public:
	RegistryReader() : reg_handle_key(nullptr) {};
	~RegistryReader() = default;

	bool OpenRegistry();
	bool CloseRegistry();
	bool IsRegistryOpen() const;

	void InitializePath(std::string &path) const;

	template <typename T> bool GetSetting(const std::vector<std::wstring> &values, T &result)
	{
		//values are always 2: parent and settingname
		std::wstring complete_reg_name = values[0] + L"_" + values[1];
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
};
} // namespace Refactoring