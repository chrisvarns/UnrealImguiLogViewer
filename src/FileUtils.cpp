#include "FileUtils.h"

#include <fstream>

namespace FileUtils
{

std::vector<std::string> ReadFileContents(const std::string& FilePath)
{
    std::vector<std::string> FileContents;
    
    std::ifstream FileStream (FilePath);
    if (FileStream.is_open())
    {
        std::string Line;
        while (std::getline(FileStream, Line))
        {
            FileContents.emplace_back(std::move(Line));
        }
        FileStream.close();
    }
    
	return FileContents;
}

}
