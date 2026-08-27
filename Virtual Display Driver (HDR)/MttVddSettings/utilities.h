#pragma once
#include <string>

namespace Refactoring
{
template <typename T> static T convert_setting(const std::wstring &value) = delete;

// specializzazioni:
template <> static bool convert_setting<bool>(const std::wstring &value)
{
	return (_wcsicmp(value.c_str(), L"true") == 0 || value == L"1");
}

template <> static int convert_setting<int>(const std::wstring &value)
{
	return std::stoi(value);
}

template <> static double convert_setting<double>(const std::wstring &value)
{
	return std::stod(value);
}

template <> static std::wstring convert_setting<std::wstring>(const std::wstring &value)
{
	return value;
}
} // namespace Refactoring
