#pragma once
#include <Windows.h>
#include <string>
//#include <vector>
#include <variant>
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

	bool GetSetting(std::string value_key, const std::variant<bool *, int *, double *, std::string *> &result)
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

  protected:
  private:
	std::string GetRawRegistryValue(HKEY hKey, const std::string &setting_name);
	HKEY reg_handle_key;
};
} // namespace Refactoring