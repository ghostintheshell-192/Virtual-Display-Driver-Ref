#pragma once
#include "utilities.h"
#include "globals.h"
#include <Windows.h>
#include <string>

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

	bool GetSetting(std::string value_key, const SettingValuePtr &result);

  protected:
  private:
	std::string GetRawRegistryValue(HKEY hKey, const std::string &setting_name);
	HKEY reg_handle_key;
};
} // namespace Refactoring