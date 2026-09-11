#pragma once
#include "tinyxml2.h"
#include "logger.h"
#include <string>
#include "globals.h"

namespace Refactoring
{
class XmlReader
{
  public:
	XmlReader(Logger * log) : settings_file(), m_log(log) {};
	~XmlReader() = default;

	bool OpenFile(std::string path);

	bool GetSetting(const std::string &value, const SettingValuePtr &result);

	bool SetSetting(const std::string &value, const std::string &pipe_value, const SettingValuePtr &result);

  protected:
  private:
	tinyxml2::XMLElement *TraverseXml(const std::string &value);

	tinyxml2::XMLDocument settings_file;
	Logger *m_log;
};
} // namespace Refactoring