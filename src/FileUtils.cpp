#include "FileUtils.h"

#include <windows.h>
#include <fileapi.h>

namespace FileUtils
{

std::string ReadFileContents(const std::string& FilePath)
{
	std::stringstream FileStream;
	HANDLE FileHandle = CreateFile(
		FilePath.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
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
	}
	return FileStream.str();
}

}