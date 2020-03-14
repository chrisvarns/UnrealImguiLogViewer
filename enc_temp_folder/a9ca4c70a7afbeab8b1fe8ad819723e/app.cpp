#include "imgui.h"

void RenderWindow()
{
//	ImGui::ShowDemoWindow(&show_demo_window);

	ImGuiWindowFlags window_flags = 0
		//| ImGuiWindowFlags_NoTitleBar
		//| ImGuiWindowFlags_NoScrollbar
		;
	if (ImGui::Begin("Dear ImGui Demo", nullptr, window_flags))
	{

	}
	ImGui::End();
}