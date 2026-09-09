#pragma once
#include "tinyxml2.h"
#include "logger.h"
#include <string>
#include "globals_new.h"

namespace Refactoring
{
class XmlReader
{
  public:
	XmlReader(Logger * log) : settings_file(), m_log(log) {};
	~XmlReader() = default;

	bool OpenFile(std::string path);

	bool GetSetting(const std::string &value, const SettingValuePtr &result);

  protected:
  private:
	tinyxml2::XMLDocument settings_file;
	Logger *m_log;
};
} // namespace Refactoring