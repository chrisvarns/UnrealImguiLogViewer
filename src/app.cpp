#include "imgui.h"

using namespace ImGui;

bool RenderWindow()
{
	//	ImGui::ShowDemoWindow(&show_demo_window);

	bool exit_app = true;
	// MainMenu
	{
		BeginMainMenuBar();
		if (BeginMenu("File"))
		{
			if (MenuItem("Open"))
			{
				// Open sesame
			}
			if (MenuItem("Exit"))
			{
				exit_app = false;
			}
			EndMenu();
		}
		EndMainMenuBar();
	}

	ImGuiWindowFlags window_flags = 0
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_MenuBar
		//| ImGuiWindowFlags_NoScrollbar
		;
	if (ImGui::Begin("Unreal Log Viewer", nullptr, window_flags))
	{
	}

	ImGui::End();
	return exit_app;
}