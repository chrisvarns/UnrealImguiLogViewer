#include "FileUtils.h"

#include <windows.h>
#include <fileapi.h>

#include <sstream>

namespace FileUtils
{

std::vector<std::string> ReadFileContents(const std::string& FilePath)
{
	std::vector<std::string> FileContents;

	HANDLE FileHandle = CreateFile(
		FilePath.c_str(),
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		NULL
	);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{		
		char ReadBuffer[1024];
		DWORD BytesRead;
		bool bReadSuccess = true;

		std::stringstream FileStream;
		while (bReadSuccess)
		{
			bReadSuccess = ReadFile(
				FileHandle,
				ReadBuffer,
				1023,
				&BytesRead,
				NULL
			);
			bReadSuccess &= BytesRead > 0;
			if (bReadSuccess)
			{
				FileStream.write(ReadBuffer, BytesRead);
			}
		}
		CloseHandle(FileHandle);

		std::string Line;
		while (std::getline(FileStream, Line))
		{
			FileContents.emplace_back(std::move(Line));
		}
	}
	return FileContents;
}

}