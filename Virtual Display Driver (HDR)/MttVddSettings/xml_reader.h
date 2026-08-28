#pragma once
#include "tinyxml2.h"
//#include "globals.h"
#include "utilities.h"
#include <string>
#include <vector>

namespace Refactoring
{
class XmlReader
{
  public:
	XmlReader() : settings_file() {};
	~XmlReader() = default;

	bool OpenFile(std::string path);
	bool CloseFile();
	bool IsFileOpen() const;

	template <typename T> bool GetSetting(const std::vector<std::string> &values, T &result)
	{
		tinyxml2::XMLElement *element = settings_file.RootElement();
		char *raw_value = nullptr;

		if (values.size() == 3)
		{
			raw_value = element->FirstChildElement(values[0].c_str())->FirstChildElement(values[1].c_str())->FindAttribute(values[2].c_str())->Value();
		}
		else if (values.size() == 2)
		{
			raw_value =
				element->FirstChildElement(values[0].c_str())->FindAttribute(values[1].c_str())->Value();
		}

		result = convert_setting(std::to_string(raw_value));
		return true;
	}

  protected:
  private:
	tinyxml2::XMLDocument settings_file;
};
}