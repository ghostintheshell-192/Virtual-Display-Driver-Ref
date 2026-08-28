#pragma once
#include "tinyxml2.h"
//#include "globals.h"
#include "utilities.h"
#include <string>
#include <vector>
#include <iostream>

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

		if (!element)
		{
			std::cout << "Failed to read nodes in xml file\n";
			return false;
		}
		std::string raw_value;

		if (values.size() == 3)
		{
			raw_value = element->FirstChildElement(values[0].c_str())
							->FirstChildElement(values[1].c_str())
							->FirstChildElement(values[2].c_str())
							->GetText();
		}
		else if (values.size() == 2)
		{
			raw_value = element->FirstChildElement(values[0].c_str())->FirstChildElement(values[1].c_str())->GetText();
		}

		if (raw_value.empty())
			return false;

		result = convert_setting<T>(raw_value);
		return true;
	}

  protected:
  private:
	tinyxml2::XMLDocument settings_file;
};
}