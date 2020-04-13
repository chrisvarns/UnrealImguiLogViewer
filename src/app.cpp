#include "imgui/imgui.h"
#include "FileUtils.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

enum class EFilterType : int
{
	Include = 0,
	Exclude,
	MAX
};

const char* EFilterTypeStrings[(int)EFilterType::MAX + 1] = { "Include", "Exclude" };

const char* ToString(EFilterType Type)
{
	int StringIndex = (int)Type;
	if (StringIndex >= 0 || StringIndex < (int)EFilterType::MAX)
	{
		return EFilterTypeStrings[StringIndex];
	}
	return "Unknown";
}

struct FLineFilter
{
	EFilterType Type = EFilterType::Include;
	std::string Token;
	bool bCaseMatch = false;
};

struct FLogFile
{
public:
	std::string FilePath;
	std::vector<std::string> Lines;
	std::vector<FLineFilter> Filters;
};

static std::vector<FLogFile> OpenFiles;

/** Returns true if we should include the line */
bool DoFilterLine(const std::vector<FLineFilter>& Filters, const std::string Line)
{
	auto SearchPredCaseInvariant = [](char ch1, char ch2) { return toupper(ch1) == toupper(ch2); };

	bool bIncluded = false;
	bool bExcluded = false;
	bool bIncludeFilterEncountered = false;

	for (const FLineFilter& Filter : Filters)
	{
		bool bContains = Filter.bCaseMatch ?
			std::search(Line.begin(), Line.end(), Filter.Token.begin(), Filter.Token.end()) != Line.end() :
			std::search(Line.begin(), Line.end(), Filter.Token.begin(), Filter.Token.end(), SearchPredCaseInvariant) != Line.end();

		if (Filter.Type == EFilterType::Include)
		{
			bIncludeFilterEncountered = true;
			bIncluded |= bContains;
		}
		else if (Filter.Type == EFilterType::Exclude)
		{
			bExcluded |= bContains;
			break; // No need to continue if we're excluded
		}
	}

	return !bExcluded && (bIncluded || !bIncludeFilterEncountered);
}

bool RenderWindow()
{
	if (OpenFiles.size() == 0)
	{
		FLogFile LogFile;
		LogFile.FilePath = "test";
		std::string TestLine;
		for (int i = 0; i < 100; ++i)
		{
			TestLine += "Lorem ipsom etc ";
		}
		LogFile.Lines.push_back(TestLine);
		for (char a = '0'; a <= 'z'; ++a)
			LogFile.Lines.push_back(std::string(&a, 1));
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

	bool show_demo_window = true;
	ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Once);
	ImGui::ShowDemoWindow(&show_demo_window);

	for (FLogFile& File : OpenFiles)
	{
		static ImGuiTextFilter LineFilter;

		ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Once);
		if (ImGui::Begin(File.FilePath.c_str(), nullptr, ImGuiWindowFlags_None))
		{
			ImGui::Columns(2);
			// Text region
			{
				ImGui::PushTextWrapPos();
				for (std::string& Line : File.Lines)
				{
					if (DoFilterLine(File.Filters, Line))
					{
						ImGui::Text(Line.c_str());
					}
				}
				ImGui::PopTextWrapPos();
			}

			ImGui::NextColumn();

			// Config region
			{
				if (ImGui::Button("Add Filter"))
				{
					File.Filters.emplace_back(FLineFilter());
				}

				for (int LineFilterIdx = 0; LineFilterIdx < File.Filters.size(); ++LineFilterIdx)
				{
					FLineFilter& LineFilter = File.Filters[LineFilterIdx];
					ImGui::Spacing();
					ImGui::PushID(LineFilterIdx);
					ImGui::Combo("Filter Type", (int*)&LineFilter.Type, EFilterTypeStrings, int(EFilterType::MAX));
					static char Buf[1024];
					strncpy_s(Buf, LineFilter.Token.c_str(), LineFilter.Token.size());
					if (ImGui::InputText("Filter Token", Buf, 1024))
					{
						LineFilter.Token = Buf;
					}
					ImGui::Checkbox("Case Sensitive", &LineFilter.bCaseMatch);
					if (ImGui::Button("Remove Filter"))
					{
						File.Filters.erase(File.Filters.begin() + LineFilterIdx);
					}
					ImGui::PopID();
				}
			}
		}
		ImGui::End();
	}

	return bExitApp;
}

void OpenAdditionalFile(const std::string& FilePath)
{
	OpenFiles.push_back({ FilePath, FileUtils::ReadFileContents(FilePath) });
}