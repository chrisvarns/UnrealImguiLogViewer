#include "imgui/imgui.h"
#include "FileUtils.h"

#include <string>
#include <vector>

struct FLogFile
{
public:
	std::string FilePath;
	std::string FileContents;
};

static std::vector<FLogFile> OpenFiles;

bool RenderWindow()
{
	if (OpenFiles.size() == 0)
	{
		FLogFile LogFile;
		LogFile.FilePath = "test";
		LogFile.FileContents = "test";
		OpenFiles.push_back(std::move(LogFile));
	}

	bool bExitApp = true;
	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

	// Fullscreen Dock
	{
		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetWorkPos());
		ImGui::SetNextWindowSize(viewport->GetWorkSize());
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", nullptr, window_flags);
		ImGui::PopStyleVar(3);

		// DockSpace
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

		// MainMenu
		if (ImGui::BeginMenuBar())
		{
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
			ImGui::EndMenuBar();
		}

		ImGui::End();
	}

	/*bool show_demo_window = true;
	ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Once);
	ImGui::ShowDemoWindow(&show_demo_window);*/

	for (const FLogFile& File : OpenFiles)
	{
		ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Once);
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