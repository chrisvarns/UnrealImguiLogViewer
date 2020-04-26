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
bool DoFilterLine(const std::vector<FLineFilter>& Filters, const std::string& Line)
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

enum class ELogLineType
{
	Normal,
	Warning,
	Error
};

struct FLogLineMetadata
{
	FLogLineMetadata(const std::string& InText);
	bool bContainsTimestamp = false;
	ELogLineType LineType = ELogLineType::Normal;

	static const int TimestampStartIdx = 0;
	static const int TimestampEndIdx = TimestampStartIdx + 24;
	static const int FrameStartIdx = TimestampEndIdx + 1;
	static const int FrameEndIdx = FrameStartIdx + 4;
};

FLogLineMetadata::FLogLineMetadata(const std::string& Text)
{
	if (Text[TimestampStartIdx] == '[' && Text[TimestampEndIdx] == ']' && Text[FrameStartIdx] == '[' && Text[FrameEndIdx] == ']') bContainsTimestamp = true;
	if (Contains(Text, "Error")) LineType = ELogLineType::Error;
	else if (Contains(Text, "Warning")) LineType = ELogLineType::Warning;
}

typedef std::vector<int> FDisplayLines;

struct FLogFile
{
public:
	FLogFile(const std::string& FilePath, std::vector<std::string>&& InLines);
	std::string FilePath;
	std::vector<std::string> Lines;
	std::vector<FLogLineMetadata> LineMetadatas;
	std::vector<FLineFilter> Filters;
	mutable bool bDisplayTextDirty = true;

	const FDisplayLines& GetDisplayLines() const
	{
		if (bDisplayTextDirty)
		{
			DisplayLines.clear();
			DisplayLines.reserve(Lines.size());

			std::string Line;
			for (int LineIdx = 0; LineIdx < Lines.size(); ++LineIdx)
			{
				const std::string& Line = Lines[LineIdx];
				if (DoFilterLine(Filters, Line))
				{
					DisplayLines.emplace_back(LineIdx);
				}
			}
			bDisplayTextDirty = false;
		}
		return DisplayLines;
	}

private:
	mutable FDisplayLines DisplayLines;
};

FLogFile::FLogFile(const std::string& FilePath, std::vector<std::string>&& InLines)
	: FilePath(FilePath)
	, Lines(std::move(InLines))
{
	LineMetadatas.reserve(Lines.size());
	for(int LineIdx = 0; LineIdx < Lines.size(); ++LineIdx)
	{
		const std::string& Line = Lines[LineIdx];
		LineMetadatas.emplace_back(FLogLineMetadata(Line));
	}
}

static std::vector<FLogFile> OpenFiles;
static ImVec4 TextColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
static ImVec4 TextColor_Warning = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
static ImVec4 TextColor_Error = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
static bool bWordWrap = true;
static bool bDisplayTimestamps = true;

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

void RenderTextWindow(const FLogFile& LogFile)
{
	const FDisplayLines& DisplayLines = LogFile.GetDisplayLines();
	// Get width of the line number section
	int NumLineNumChars = 1;
	{
		size_t BiggestLine = DisplayLines[DisplayLines.size()-1];
		while (BiggestLine /= 10) ++NumLineNumChars;
	}

	if (bWordWrap)
	{
		ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionWidth());
	}
	ImGuiListClipper Clipper((int)DisplayLines.size());
	while (Clipper.Step())
	{
		for (int ClipperIdx = Clipper.DisplayStart; ClipperIdx < Clipper.DisplayEnd; ++ClipperIdx)
		{
			int LineNumber = DisplayLines[ClipperIdx];
			const std::string& LogLine = LogFile.Lines[LineNumber];
			const FLogLineMetadata& LogLineMetadata = LogFile.LineMetadatas[LineNumber];

			ImVec4 TextStyleColor;
			switch (LogLineMetadata.LineType)
			{
			case ELogLineType::Warning: TextStyleColor = TextColor_Warning; break;
			case ELogLineType::Error: TextStyleColor = TextColor_Error; break;
			default: TextStyleColor = TextColor; break;
			}
			ImGui::PushStyleColor(ImGuiCol_Text, TextStyleColor);
			ImGui::Text("%d", LineNumber + 1);
			ImGui::SameLine(NumLineNumChars * ImGui::GetFontSize());

			const char* TextPtr = LogLine.c_str();
			TextPtr += !bDisplayTimestamps && LogLineMetadata.bContainsTimestamp ? FLogLineMetadata::FrameEndIdx+1 : 0;
			ImGui::TextUnformatted(TextPtr);

			ImGui::PopStyleColor();

			// Content menu
			{
				ImGui::PushID(ClipperIdx);
				if (ImGui::BeginPopupContextItem("DisplayText context menu"))
				{
					if (ImGui::Selectable("Copy")) ImGui::SetClipboardText(LogLine.c_str());
					ImGui::EndPopup();
				}
				ImGui::PopID();
			}
		}
	}
	if (bWordWrap)
	{
		ImGui::PopTextWrapPos();
	}
}

namespace App
{

bool RenderWindow()
{
	// Create test file
	if (OpenFiles.size() == 0)
	{
		std::vector<std::string> Lines;
		std::string TestLine;
		for (int i = 0; i < 100; ++i)
		{
			TestLine += "Lorem ipsom etc ";
		}
		Lines.emplace_back(std::move(TestLine));
		for (char a = '0'; a <= 'z'; ++a)
			Lines.emplace_back(&a);

		OpenFiles.emplace_back(FLogFile("test", std::move(Lines)));
	}

	bool bAppContinue = true;
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
					bAppContinue = false;
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Options"))
			{
				ImGui::ColorEdit3("Text Color", &TextColor.x, ImGuiColorEditFlags_None);
				ImGui::ColorEdit3("Text Color (Warning)", &TextColor_Warning.x, ImGuiColorEditFlags_None);
				ImGui::ColorEdit3("Text Color (Error)", &TextColor_Error.x, ImGuiColorEditFlags_None);
				ImGui::Checkbox("Word Wrap", &bWordWrap);
				ImGui::Checkbox("Display Timestamps", &bDisplayTimestamps);
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
				RenderTextWindow(File);
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

	return bAppContinue;
}

void OpenAdditionalFile(const std::string& FilePath)
{
	OpenFiles.emplace_back(FLogFile(FilePath, FileUtils::ReadFileContents(FilePath)));
}

void Startup(int argc, char** argv)
{
	for (int i = 1; i < argc; ++i)
	{
		OpenAdditionalFile(argv[i]);
	}
}

} // namespace App