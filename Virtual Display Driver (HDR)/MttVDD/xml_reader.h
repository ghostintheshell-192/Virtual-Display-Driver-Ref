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
	XmlReader(Logger * log) : settings_file(), file_path(), m_log(log) {};
	~XmlReader() = default;

	void SetConfigurationFile(const std::string &path);

	bool OpenFile();

	bool GetSetting(const std::string &value, const SettingValuePtr &result);

	bool SetSetting(const std::string &value, const std::string &pipe_value, const SettingValuePtr &result);

  protected:
  private:
	tinyxml2::XMLElement *TraverseXml(const std::string &value);

	tinyxml2::XMLDocument settings_file;
	std::string file_path;
	Logger *m_log;
};
} // namespace Refactoring