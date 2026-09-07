#pragma once
#include "tinyxml2.h"
#include <string>
#include "globals.h"

namespace Refactoring
{
class XmlReader
{
  public:
	XmlReader() : settings_file() {};
	~XmlReader() = default;

	bool OpenFile(std::string path);

	bool GetSetting(const std::string &value, const SettingValuePtr &result);

  protected:
  private:
	tinyxml2::XMLDocument settings_file;
};
} // namespace Refactoring