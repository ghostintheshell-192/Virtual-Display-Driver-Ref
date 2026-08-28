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

	template <typename T> bool GetSetting(const std::vector<std::string> &values, T &result)
	{
		//values are always 2: parent and settingname
		std::string raw_reg_value = GetRawRegistryValue(reg_handle_key, values);

		if (raw_reg_value.empty())
			return false;

		result = convert_setting<T>(raw_reg_value);
		return true;
	}

  protected:
  private:
	std::string GetRawRegistryValue(HKEY hKey, const std::vector<std::string> &setting_name);
	HKEY reg_handle_key;
};
} // namespace Refactoring