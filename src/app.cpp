#include "imgui/imgui.h"
#include "FileUtils.h"

#include <string>
#include <vector>

struct LogFile
{
public:
	std::string FilePath;
	std::string FileContents;
};

static std::vector<LogFile> OpenFiles;

bool RenderWindow()
{
	//	ImGui::ShowDemoWindow(&show_demo_window);

	bool bExitApp = true;
	// MainMenu
	{
		ImGui::BeginMainMenuBar();
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open"))
			{
				// Open sesame
			}
			if (ImGui::MenuItem("Exit"))
			{
				bExitApp = false;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	for (const LogFile& File : OpenFiles)
	{
		if (ImGui::Begin(File.FilePath.c_str(), nullptr, ImGuiWindowFlags_None))
		{
			ImGui::Text(File.FileContents.c_str());
		}
		ImGui::End();
	}

	return bExitApp;
}

void OpenAdditionalFile(const std::string& FilePath)
{
	OpenFiles.push_back({ FilePath, FileUtils::ReadFileContents(FilePath) });
}