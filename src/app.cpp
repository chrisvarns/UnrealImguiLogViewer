#include "imgui/imgui.h"
#include "FileUtils.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

enum class EFilterType : int
{
	TextInclude = 0,
	TextExclude,
	LogCategory,
	MAX
};

const char* EFilterTypeStrings[(int)EFilterType::MAX + 1] =
{
	"Text Include",
	"Text Exclude",
	"Log Category"
};

const char* ToString(EFilterType Type)
{
	int StringIndex = (int)Type;
	if (StringIndex >= 0 || StringIndex < (int)EFilterType::MAX)
	{
		return EFilterTypeStrings[StringIndex];
	}
	return "Unknown";
}

enum class ELogVerbosity : int
{
	Off = 0,
	Error,
	Warning,
	Log,
	Verbose,
	VeryVerbose,
	MAX
};

const char* ELogVerbosityStrings[(int)ELogVerbosity::MAX + 1] =
{
	"Off",
	"Error",
	"Warning",
	"Log",
	"Verbose",
	"VeryVerbose"
};

const char* ToString(ELogVerbosity Type)
{
	int StringIndex = (int)Type;
	if (StringIndex >= 0 || StringIndex < (int)ELogVerbosity::MAX)
	{
		return ELogVerbosityStrings[StringIndex];
	}
	return "Unknown";
}

struct FLineFilter
{
	EFilterType Type = EFilterType::TextInclude;
	struct
	{
		std::string Token;
		bool bCaseMatch = false;
	} TextData;
	struct
	{
		std::string Category;
		ELogVerbosity Verbosity = ELogVerbosity::Log;
	} LogCategoryData;
	bool bEnable = false;
};

bool Contains(const std::string& Haystack, const std::string& Needle)
{
	return std::search(Haystack.begin(), Haystack.end(), Needle.begin(), Needle.end()) != Haystack.end();
}

template<class TPred>
bool ContainsByPred(const std::string& Haystack, const std::string& Needle, TPred Pred)
{
	return std::search(Haystack.begin(), Haystack.end(), Needle.begin(), Needle.end(), Pred) != Haystack.end();
}

/** Returns true if we should include the line */
bool DoFilterLine(const std::vector<FLineFilter>& Filters, const std::string Line)
{
	auto SearchPredCaseInvariant = [](char ch1, char ch2) { return toupper(ch1) == toupper(ch2); };

	bool bIncluded = false;
	bool bExcluded = false;
	bool bIncludeFilterEncountered = false;

	for (const FLineFilter& Filter : Filters)
	{
		if (bExcluded) break;
		if (!Filter.bEnable) continue;

		if (Filter.Type == EFilterType::TextInclude || Filter.Type == EFilterType::TextExclude)
		{
			const auto& FilterData = Filter.TextData;
			bool bContains = FilterData.Token.empty() ? false :
				FilterData.bCaseMatch ? Contains(Line, FilterData.Token) : ContainsByPred(Line, FilterData.Token, SearchPredCaseInvariant);

			if (Filter.Type == EFilterType::TextInclude)
			{
				bIncludeFilterEncountered = true;
				bIncluded |= bContains;
			}
			else if (Filter.Type == EFilterType::TextExclude)
			{
				bExcluded |= bContains;
			}
		}
		else if (Filter.Type == EFilterType::LogCategory)
		{
			const auto& FilterData = Filter.LogCategoryData;

			if (FilterData.Verbosity != ELogVerbosity::VeryVerbose
				&& !FilterData.Category.empty()
				&& Contains(Line, FilterData.Category + ":"))
			{
				switch (FilterData.Verbosity)
				{
				case ELogVerbosity::Off:
					bExcluded = true;
					break;
				case ELogVerbosity::Error:
					bExcluded = !Contains(Line, FilterData.Category + ": Error:");
					break;
				case ELogVerbosity::Warning:
					bExcluded = !(Contains(Line, FilterData.Category + ": Error:")
						|| Contains(Line, FilterData.Category + ": Warning:"));
					break;
				case ELogVerbosity::Log:
					bExcluded = Contains(Line, FilterData.Category + ": VeryVerbose:")
						|| Contains(Line, FilterData.Category + ": Verbose:");
					break;
				case ELogVerbosity::Verbose:
					bExcluded = Contains(Line, FilterData.Category + ": VeryVerbose:");
					break;
				}
			}
		}
		else assert(false);
	}

	return !bExcluded && (bIncluded || !bIncludeFilterEncountered);
}

struct FLogFile
{
public:
	std::string FilePath;
	std::string FileContent;
	std::vector<FLineFilter> Filters;
	bool bInitialColumnWidthSet = false;
	bool bDisplayStrDirty = true;

	const std::string& GetDisplayStr()
	{
		if (bDisplayStrDirty)
		{
			std::stringstream DisplayStream;
			std::stringstream Input(FileContent);
			std::string Line;
			while (std::getline(Input, Line))
			{
				if (DoFilterLine(Filters, Line))
				{
					DisplayStream.write(Line.c_str(), Line.size());
					DisplayStream.put('\n');
				}
			}
			DisplayStr = DisplayStream.str();
			bDisplayStrDirty = false;
		}
		return DisplayStr;
	}

private:
	std::string DisplayStr;
};

static std::vector<FLogFile> OpenFiles;

bool ImGuiInputText(const char* Label, std::string& InOutText)
{
	static char Buf[1024];
	strncpy_s(Buf, InOutText.c_str(), InOutText.size());
	if (ImGui::InputText("Token", Buf, 1024))
	{
		InOutText = Buf;
		return true;
	}
	return false;
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
		LogFile.FileContent += TestLine;
		for (char a = '0'; a <= 'z'; ++a)
			LogFile.FileContent += a;
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
			if (!File.bInitialColumnWidthSet)
			{
				ImGui::SetColumnWidth(0, ImGui::GetWindowContentRegionWidth() * 0.85f);
				File.bInitialColumnWidthSet = true;
			}
			// Text region
			{
				ImGui::PushTextWrapPos();
				
				//for (int i = 0; i < File.Lines.size(); ++i)
				//{
				//	if (File.LineIncluded[i])
				//	{
				//		ImGui::Text(File.Lines[i].c_str());
				//	}
				//}
				ImGui::TextUnformatted(File.GetDisplayStr().c_str());
				ImGui::PopTextWrapPos();
			}

			ImGui::NextColumn();

			// Config region
			{
				if (ImGui::Button("Add Filter"))
				{
					File.Filters.emplace_back(FLineFilter());
				}

				bool bFiltersDirty = false;
				for (int LineFilterIdx = 0; LineFilterIdx < File.Filters.size(); ++LineFilterIdx)
				{
					FLineFilter& LineFilter = File.Filters[LineFilterIdx];
					ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
					ImGui::PushID(LineFilterIdx);

					bool bFilterDirty = false;
					bFilterDirty |= ImGui::Combo("Type", (int*)&LineFilter.Type, EFilterTypeStrings, int(EFilterType::MAX));
					if (LineFilter.Type == EFilterType::TextInclude || LineFilter.Type == EFilterType::TextExclude)
					{
						auto& FilterData = LineFilter.TextData;
						bFilterDirty |= ImGuiInputText("Token", FilterData.Token);
						bFilterDirty |= ImGui::Checkbox("Case Sensitive", &FilterData.bCaseMatch);
						//ImGui::SameLine();
						//bFilterDirty |= ImGui::Checkbox("Regex", &LineFilter.bRegex);
					}
					else if (LineFilter.Type == EFilterType::LogCategory)
					{
						auto& FilterData = LineFilter.LogCategoryData;
						bFilterDirty |= ImGuiInputText("Category", FilterData.Category);
						bFilterDirty |= ImGui::Combo("Verbosity", (int*)&FilterData.Verbosity, ELogVerbosityStrings, int(ELogVerbosity::MAX));
					}

					bool bEnableChanged = ImGui::Checkbox("Enable", &LineFilter.bEnable);
					ImGui::SameLine();
					if (ImGui::Button("Remove Filter"))
					{
						File.Filters.erase(File.Filters.begin() + LineFilterIdx);
						bFilterDirty = true;
						--LineFilterIdx;
					}

					File.bDisplayStrDirty = bEnableChanged || (LineFilter.bEnable && bFilterDirty);

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
	FLogFile LogFile;
	LogFile.FilePath = FilePath;
	LogFile.FileContent = FileUtils::ReadFileContents(FilePath);
	OpenFiles.push_back(std::move(LogFile));
}