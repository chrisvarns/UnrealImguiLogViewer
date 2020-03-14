#include "imgui.h"

using namespace ImGui;

bool RenderWindow()
{
	//	ImGui::ShowDemoWindow(&show_demo_window);

	bool return_value = true;
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
				return_value = false;
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
	return return_value;
}