#include "imgui/imgui.h"
#include "FileUtils.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define STRNCPY strncpy_s
#else
#define STRNCPY strncpy
#endif

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

struct FDisplayLine
{
	int LineNumber;
	std::string Text;
};

typedef std::vector<FDisplayLine> FDisplayText;

struct FLogFile
{
public:
	std::string FilePath;
	std::vector<std::string> FileContents;
	std::vector<FLineFilter> Filters;
	bool bDisplayTextDirty = true;
	int NumFileContentLines = -1;

	const FDisplayText& GetDisplayText()
	{
		if (bDisplayTextDirty)
		{
			DisplayText.clear();
			DisplayText.reserve(NumFileContentLines > 0 ? NumFileContentLines : 16384);

			std::string Line;
			int LineNumber = 1;
			for (const std::string& Line : FileContents)
			{
				if (DoFilterLine(Filters, Line))
				{
					FDisplayLine DisplayLine;
					DisplayLine.LineNumber = LineNumber;
					DisplayLine.Text = std::string(Line.c_str(), Line.size());
					DisplayText.emplace_back(std::move(DisplayLine));
				}
				++LineNumber;
			}
			NumFileContentLines = LineNumber + 1;
			bDisplayTextDirty = false;
		}
		return DisplayText;
	}

private:
	FDisplayText DisplayText;
};

static std::vector<FLogFile> OpenFiles;
static ImVec4 TextColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
static bool bWordWrap = true;

bool InputTextBox(const char* Label, std::string& InOutText)
{
	static char Buf[1024];
	STRNCPY(Buf, InOutText.c_str(), InOutText.size());
	if (ImGui::InputText(Label, Buf, 1024))
	{
		InOutText = Buf;
		return true;
	}
	return false;
}

void RenderTextWindow(const FDisplayText& DisplayText)
{
	// Get width of the line number section
	int NumLineNumChars = 1;
	{
		size_t NumLines = DisplayText[DisplayText.size()-1].LineNumber;
		while (NumLines /= 10) ++NumLineNumChars;
	}

	if (bWordWrap)
	{
		ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionWidth());
	}
	ImGui::PushStyleColor(ImGuiCol_Text, TextColor);
	ImGuiListClipper Clipper((int)DisplayText.size());
	while (Clipper.Step())
	{
		for (int i = Clipper.DisplayStart; i < Clipper.DisplayEnd; ++i)
		{
			ImGui::Text("%d", DisplayText[i].LineNumber);
			ImGui::SameLine(NumLineNumChars * ImGui::GetFontSize());
			ImGui::TextUnformatted(DisplayText[i].Text.c_str());
			ImGui::PushID(i);
			if (ImGui::BeginPopupContextItem("DisplayText context menu"))
			{
				if (ImGui::Selectable("Copy")) ImGui::SetClipboardText(DisplayText[i].Text.c_str());
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
	}
	ImGui::PopStyleColor();
	if (bWordWrap)
	{
		ImGui::PopTextWrapPos();
	}
}

namespace App
{

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
		LogFile.FileContents.emplace_back(std::move(TestLine));
		for (char a = '0'; a <= 'z'; ++a)
			LogFile.FileContents.emplace_back(&a);
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
			if (ImGui::BeginMenu("Options"))
			{
				ImGui::ColorEdit3("Text Color", &TextColor.x, ImGuiColorEditFlags_None);
				ImGui::Checkbox("Word Wrap", &bWordWrap);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::End();
	}

	static bool show_demo_window = true;
	ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Once);
	ImGui::ShowDemoWindow(&show_demo_window);

	for (FLogFile& File : OpenFiles)
	{
		ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Once);

		if (ImGui::Begin(File.FilePath.c_str(), nullptr, ImGuiWindowFlags_None))
		{
			if (ImGui::BeginChild("TextRegion", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.85f, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
			{
				RenderTextWindow(File.GetDisplayText());
			}
			ImGui::EndChild();

			ImGui::SameLine();

			if (ImGui::BeginChild("ConfigRegion"))
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
						bFilterDirty |= InputTextBox("Token", FilterData.Token);
						bFilterDirty |= ImGui::Checkbox("Case Sensitive", &FilterData.bCaseMatch);
						//ImGui::SameLine();
						//bFilterDirty |= ImGui::Checkbox("Regex", &LineFilter.bRegex);
					}
					else if (LineFilter.Type == EFilterType::LogCategory)
					{
						auto& FilterData = LineFilter.LogCategoryData;
						bFilterDirty |= InputTextBox("Category", FilterData.Category);
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

					File.bDisplayTextDirty |= bEnableChanged || (LineFilter.bEnable && bFilterDirty);

					ImGui::PopID();
				}
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	return bExitApp;
}

void OpenAdditionalFile(const std::string& FilePath)
{
	FLogFile LogFile;
	LogFile.FilePath = FilePath;
	LogFile.FileContents = FileUtils::ReadFileContents(FilePath);
	OpenFiles.push_back(std::move(LogFile));
}

void Startup(int argc, char** argv)
{
	for (int i = 1; i < argc; ++i)
	{
		OpenAdditionalFile(argv[i]);
	}
}

} // namespace App