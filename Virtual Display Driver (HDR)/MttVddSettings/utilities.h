#pragma once
#include <string>

namespace Refactoring
{
template <typename T> static T convert_setting(const std::string &value) = delete;

// specializzazioni:
template <> static bool convert_setting<bool>(const std::string &value)
{
	return (strcmp(value.c_str(), "true") == 0 || value == "1");
}

template <> static int convert_setting<int>(const std::string &value)
{
	return std::stoi(value);
}

template <> static double convert_setting<double>(const std::string &value)
{
	return std::stod(value);
}

template <> static std::string convert_setting<std::string>(const std::string &value)
{
	return value;
}
} // namespace Refactoring
