#pragma once
#include "globals_new.h"
#include "logger.h"

#include <Windows.h>
#include <string>

namespace Refactoring
{
class RegistryReader
{
  public:
	RegistryReader(Logger * log) : reg_handle_key(nullptr), m_log(log) {};
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
	Logger *m_log;
};
} // namespace Refactoring