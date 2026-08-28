#include "xml_reader.h"
#include "utilities.h"
#include "globals.h"
#include <iostream>

bool Refactoring::XmlReader::OpenFile(std::string path)
{
	tinyxml2::XMLError err = settings_file.LoadFile(path.c_str());

	if (err == tinyxml2::XML_SUCCESS)
	{
		std::cout << "File open at path : " + path + "\n";
		return true;
	}

	std::cout << "Failed to open file at path : " + path + "\n";
	return false;
}

bool Refactoring::XmlReader::GetSetting(const std::string &value, const SettingValuePtr &result)
{
	std::vector<std::string> values = tokenize(value, '.');

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
			std::cout << "Node not found in xml: " << segment.c_str() << "\n";
			return false;
		}
	}
	raw_value = current->GetText();

	if (raw_value.empty())
		return false;

	std::visit(
		[&raw_value](auto *ptr) {
			using T = std::remove_pointer_t<decltype(ptr)>;
			*ptr = convert_setting<T>(raw_value);
		},
		result);

	// result = convert_setting<T>(raw_value);
	return true;
}