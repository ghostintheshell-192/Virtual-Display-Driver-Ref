#include "xml_reader.h"
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

bool Refactoring::XmlReader::CloseFile()
{
	return true;
}

bool Refactoring::XmlReader::IsFileOpen() const
{
	return false;
}