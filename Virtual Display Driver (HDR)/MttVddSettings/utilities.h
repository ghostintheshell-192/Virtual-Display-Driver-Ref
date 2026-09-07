#pragma once
#include <string>
#include <vector>

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

static std::vector<std::string> tokenize(std::string str, char divider)
{

	std::vector<std::string> tokens;
	size_t pos = 0;

	while (pos != std::string::npos)
	{
		pos = 0;
		pos = str.find(divider);

		std::string new_str = str.substr(0, pos);
		str = str.substr(pos + 1, str.size());

		tokens.push_back(new_str);
	}

	return tokens;
}
} // namespace Refactoring
