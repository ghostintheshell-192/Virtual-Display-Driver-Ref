#include "xml_reader.h"
#include "utilities.h"
#include "globals.h"

bool Refactoring::XmlReader::OpenFile(std::string path)
{
	tinyxml2::XMLError err = settings_file.LoadFile(path.c_str());

	if (err == tinyxml2::XML_SUCCESS)
	{
		m_log->Message(LogType::Info, "[XmlReader] File open at path : " + path + "\n");
		return true;
	}

	m_log->Message(LogType::Error, "[XmlReader] Failed to open file at path : " + path + "\n");
	return false;
}

bool Refactoring::XmlReader::GetSetting(const std::string &value, const SettingValuePtr &result)
{
	std::vector<std::string> values = tokenize(value, '.');
	std::string raw_value;

	tinyxml2::XMLElement *current = settings_file.RootElement();

	if (!current)
		return false;

	for (const auto &segment : values)
	{
		current = current->FirstChildElement(segment.c_str());
		if (!current)
		{
			m_log->Message(LogType::Error, "[XmlReader] Node not found in xml: " + segment + "\n");
			return false;
		}
	}

	const char * text = current->GetText();

	if (!text)
		return false;

	raw_value = text;

	if (raw_value.empty())
		return false;

	std::visit(
		[&raw_value](auto *ptr) {
			using T = std::remove_pointer_t<decltype(ptr)>;
			*ptr = convert_setting<T>(raw_value);
		},
		result);

	return true;
}