#pragma once
#include "tinyxml2.h"
// #include "globals.h"
#include "utilities.h"
#include <iostream>
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

		if (!element)
		{
			std::cout << "Failed to read nodes in xml file\n";
			return false;
		}
		std::string raw_value;

		tinyxml2::XMLElement *current = settings_file.RootElement();
		for (const auto &segment : values)
		{
			current = current->FirstChildElement(segment.c_str());
			if (!current)
			{
				std::cout << "Node not found in xml: " << segment.c_str();
				return false;
			}

		}
		raw_value = current->GetText();

		if (raw_value.empty())
			return false;

		result = convert_setting<T>(raw_value);
		return true;
	}

  protected:
  private:
	tinyxml2::XMLDocument settings_file;
};
} // namespace Refactoring