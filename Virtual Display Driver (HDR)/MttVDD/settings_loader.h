#pragma once
#include "globals.h"
#include "logger.h"
#include "registry_reader.h"
#include "xml_reader.h"
#include <vector>

namespace Refactoring
{
	class SettingsLoader
	{
	  public:
		SettingsLoader(Logger *log, DriverSettings *ext_settings);
		~SettingsLoader() = default;

		void Init();
		void LoadSettings();

		bool GetSetting(const std::string &value);
		bool SetSetting(const std::string &key, const std::string &pipe_str_value);

	  protected:
	  private:
		Logger * m_log;
		RegistryReader reg_reader;
		XmlReader xml_reader;
		DriverSettings * m_settings;

		bool check_registry;
		bool check_xml;

		std::string conf_path;

		std::vector<DataElements> entries;
	};
}