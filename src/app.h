#pragma once

#include <string>

namespace App
{

	void AppStartup(int argc, char** argv);

	// Returns false if we should exit
	bool RenderWindow();

	void OpenAdditionalFile(const std::string& FilePath);

}
