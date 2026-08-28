#pragma once
#include <Windows.h>
#include "globals.h"
#include "registry_reader.h"
#include "xml_reader.h"
#include <vector>
#include <variant>

namespace Refactoring
{
	class SettingsLoader
	{
	  public:
		SettingsLoader() = default;
		~SettingsLoader() = default;

		void Init();
		void LoadSettings();
		void OverrideDefaultsRegistry();
		void OverrideDefaultsXml();

	  protected:
	  private:
		DriverSettings settings;
		RegistryReader reg_reader;
		XmlReader xml_reader;

		bool check_registry;
		bool check_xml;

		std::string conf_path;

		struct data
		{
			std::string key;
			std::variant<bool *, int *, double *, std::string *> container_value;
		};

		std::vector<data> entries;
	};
}