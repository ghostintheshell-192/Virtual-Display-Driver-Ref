#pragma once
#include "tinyxml2.h"
#include <string>
#include <variant>

namespace Refactoring
{
class XmlReader
{
  public:
	XmlReader() : settings_file() {};
	~XmlReader() = default;

	bool OpenFile(std::string path);
	bool CloseFile();
	bool IsFileOpen() const;

	bool GetSetting(const std::string &value, const std::variant<bool *, int *, double *, std::string *> &result);

  protected:
  private:
	tinyxml2::XMLDocument settings_file;
};
} // namespace Refactoring