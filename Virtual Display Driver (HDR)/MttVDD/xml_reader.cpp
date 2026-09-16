#include "xml_reader.h"
#include "utilities.h"

void Refactoring::XmlReader::SetConfigurationFile(std::string path)
{
	file_path = path;
	m_log->Message(LogType::Info, "[XmlReader] Reading XML file: " + file_path + "\n");
}

bool Refactoring::XmlReader::OpenFile()
{
	if (file_path.empty())
	{
		m_log->Message(LogType::Error, "[XmlReader] XML File to open not provided.\n");
		return false;
	}

	tinyxml2::XMLError err = settings_file.LoadFile(file_path.c_str());

	if (err == tinyxml2::XML_SUCCESS)
	{
		m_log->Message(LogType::Info, "[XmlReader] File open at path : " + file_path + "\n");
		return true;
	}

	m_log->Message(LogType::Error, "[XmlReader] Failed to open file at path : " + file_path + "\n");
	return false;
}

tinyxml2::XMLElement* Refactoring::XmlReader::TraverseXml(const std::string& value)
{
	std::vector<std::string> values = tokenize(value, '.');

	tinyxml2::XMLElement *current = settings_file.RootElement();

	if (!current)
		return nullptr;

	for (const auto &segment : values)
	{
		current = current->FirstChildElement(segment.c_str());
		if (!current)
		{
			m_log->Message(LogType::Error, "[XmlReader] Node not found in xml: " + segment + "\n");
			return nullptr;
		}
	}
	return current;
}

bool Refactoring::XmlReader::GetSetting(const std::string &value, const SettingValuePtr &result)
{
	tinyxml2::XMLElement *current = TraverseXml(value);

	if (!current)
		return false;

	const char * text = current->GetText();

	if (!text)
		return false;

	std::string raw_value = text;

	if (raw_value.empty())
		return false;

	std::visit(
		[&raw_value, &value, this](auto *ptr) {
			using T = std::remove_pointer_t<decltype(ptr)>;

			T old_val = *ptr;
			*ptr = convert_setting<T>(raw_value);
			if (old_val != *ptr)
				m_log->Message(LogType::Debug, value + " now has value = " + raw_value);
		},
		result);

	return true;
}

bool Refactoring::XmlReader::SetSetting(const std::string& value, const std::string& pipe_value, const SettingValuePtr& result)
{
	tinyxml2::XMLElement * current = TraverseXml(value);
	if (!current)
		return false;

	current->SetText(pipe_value.c_str());
	settings_file.SaveFile(file_path.c_str(), false);

    std::visit(
		[&pipe_value, &value, this](auto *ptr) {
			using T = std::remove_pointer_t<decltype(ptr)>;

			T old_val = *ptr;
			*ptr = convert_setting<T>(pipe_value);
			if (old_val != *ptr)
				m_log->Message(LogType::Debug, value + " now has value = " + pipe_value);
		},
		result);
	return true;
}