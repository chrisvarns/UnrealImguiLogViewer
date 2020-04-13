#pragma once

#include <string>
#include <vector>

namespace FileUtils
{
	std::vector<std::string> ReadFileContents(const std::string& FilePath);
}