#pragma once

#include <string>

namespace App
{

	void Startup(int argc, char** argv);

	// Returns false if we should exit
	bool RenderWindow();

	void OpenAdditionalFile(const std::string& FilePath);

}
