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

static std::string WStringToString(const std::wstring &wstr)
{ // basically just a function for converting strings since codecvt is depricated in c++ 17
	if (wstr.empty())
		return "";

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string str(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, NULL, NULL);
	return str;
}

static std::wstring StringToWstring(const std::string &str)
{
	if (str.empty())
		return std::wstring();

	// Calcola la dimensione necessaria per la stringa di destinazione
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);

	std::wstring wstrTo(size_needed, 0);
	// Esegue la conversione effettiva
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);

	return wstrTo;
}

template <typename T> T apply_range(double value, double min, double max, double multiplier)
{
	if (value < min)
		value = min;
	if (value > max)
		value = max;
	return static_cast<T>(value * multiplier);
}
} // namespace Refactoring
