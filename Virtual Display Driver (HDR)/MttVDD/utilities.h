#pragma once
#include <string>
#include <vector>
#include <algorithm>

namespace Refactoring
{

template <typename T> static T convert_setting(const std::vector<std::string> &values) = delete;

// specializzazioni:
template <> static std::vector<bool> convert_setting<std::vector<bool>>(const std::vector<std::string> &values)
{
	std::vector<bool> result_vec;
	result_vec.reserve(values.size());

	for (const auto &value : values)
		result_vec.emplace_back(strcmp(value.c_str(), "true") == 0 || value == "1");

	return result_vec;
}

template <> static std::vector<int> convert_setting<std::vector<int>>(const std::vector<std::string> &values)
{
	std::vector<int> result_vec;
	result_vec.reserve(values.size());
	for (const auto &value : values)
		result_vec.emplace_back(std::stoi(value));
	return result_vec;
}

template <> static std::vector<double> convert_setting<std::vector<double>>(const std::vector<std::string> &values)
{
	std::vector<double> result_vec;
	result_vec.reserve(values.size());

	for (const auto &value : values)
		result_vec.emplace_back(std::stod(value));

	return result_vec;
}

template <> static std::vector<std::string> convert_setting<std::vector<std::string>>(const std::vector<std::string> &values)
{
	return values;
}

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

template <>
static Refactoring::ColorSpaceType convert_setting<Refactoring::ColorSpaceType>(const std::string& value)
{
	if (value == "sRGB" || value == "SRGB")
		return Refactoring::ColorSpaceType::sRGB;
	if (value == "DCI_P3" || value == "DCI-P3")
		return Refactoring::ColorSpaceType::DCI_P3;
	if (value == "REC_2020" || value == "REC.2020")
		return Refactoring::ColorSpaceType::REC_2020;
	if (value == "Adobe_RGB" || value == "ADOBE_RGB")
		return Refactoring::ColorSpaceType::ADOBE_RGB;

	//default, always valid
	return Refactoring::ColorSpaceType::sRGB;
}

static std::vector<std::string> tokenize(std::string str, char divider)
{

	std::vector<std::string> tokens;
	size_t pos = 0;

	while (pos < std::string::npos)
	{
		pos = str.find(divider);
		tokens.push_back(str.substr(0, pos));

		if (pos == std::string::npos)
			break;

		str = str.substr(pos + 1, str.size());
	}

	return tokens;
}

static void vector_trim(std::vector<std::string>& tokens, char ch)
{
	for (auto & tok : tokens)
		tok.erase(std::remove(tok.begin(), tok.end(), ch), tok.end());
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
