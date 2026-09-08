#pragma once
#include "globals.h"
#include "registry_reader.h"
#include "xml_reader.h"
#include <vector>

namespace Refactoring
{
	class SettingsLoader
	{
	  public:
		SettingsLoader() : check_registry(false), check_xml(false){}
		~SettingsLoader() = default;

		void Init();
		void LoadSettings();

	  protected:
	  private:
		DriverSettings settings;
		RegistryReader reg_reader;
		XmlReader xml_reader;

		bool check_registry;
		bool check_xml;

		std::string conf_path;

		std::vector<DataElements> entries;
	};
}