#include "Tools/InputViewTool.h"

#include <imgui.h>
#include <ImGui/imgui_stdlib.h>
#include "Input/Input.h"
#include "Tools/ToolManager.h"

using namespace Ball;

void InputViewTool::Init()
{
	m_Name = "Input Viewer";
	m_ToolCatagory = Ball::ToolCatagory::ENGINE;
	m_ToolInterfaceType = Ball::ToolInterfaceType::WINDOW;
}
/// <summary>
/// Small struct to inject into popup window
/// </summary>
struct RebindInfo
{
	JoyStick* joyStick;
	KeyCode* key;
};

std::vector<const char*> GenerateKeyNames()
{
	auto m_Keys = std::vector<const char*>(Ball::InputInternal::m_KeyCodeNameLookup.size());

	int i = 0;
	for (auto& keycodeInfo : Ball::InputInternal::m_KeyCodeNameLookup)
	{
		m_Keys[i] = keycodeInfo.second;
		i++;
	}

	return m_Keys;
}

const std::vector<const char*>& GetKeyNames()
{
	static std::vector<const char*> keyNames = GenerateKeyNames();

	return keyNames;
}

bool ContainsIgnoreCase(const std::string& str, const std::string& sub)
{
	if (sub.empty())
		return true;
	// Convert both strings to lowercase
	std::string strLower = str;
	std::string subLower = sub;
	std::transform(strLower.begin(), strLower.end(), strLower.begin(), [](unsigned char c) { return std::tolower(c); });
	std::transform(subLower.begin(), subLower.end(), subLower.begin(), [](unsigned char c) { return std::tolower(c); });

	// Check if strLower contains subLower
	return strLower.find(subLower) != std::string::npos;
}

static RebindInfo s_RebindInfo;
static std::string s_KeySearch = std::string();

void DrawRebindableButton(KeyCode& key, const char* ID)
{
	// TODO replace this with a button so we can change input mapping
	if (ImGui::Button((std::string(Utilities::ToString(key)) + "##" + ID).c_str()))
	{
		s_KeySearch = std::string();
		s_RebindInfo.key = &key;
		s_RebindInfo.joyStick = nullptr;
		ImGui::OpenPopup("KeyRebindModal");
	}
}

void DrawRebindableButton(JoyStick& key, const char* ID)
{
	// TODO replace this with a button so we can change input mapping
	if (ImGui::Button((std::string(Utilities::ToString(key)) + "##" + ID).c_str()))
	{
		s_KeySearch = std::string();
		s_RebindInfo.key = nullptr;
		s_RebindInfo.joyStick = &key;
		ImGui::OpenPopup("JoyStickRebindModal");
	}
}

void DrawKeyRebindMenu()
{
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
		ImGui::SetKeyboardFocusHere(0);

	ImGui::InputText("SearchFilter", &s_KeySearch);

	for (auto& keyCodeNameLookup : Ball::InputInternal::m_KeyCodeNameLookup)
	{
		if (!s_KeySearch.empty() && !ContainsIgnoreCase(std::string(keyCodeNameLookup.second), s_KeySearch))
			continue;

		if (ImGui::Button(keyCodeNameLookup.second))
		{
			(*s_RebindInfo.key) = keyCodeNameLookup.first;
			ImGui::CloseCurrentPopup();
			break;
		}
	}
}

void DrawJoyStickRebindMenu()
{
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
		ImGui::SetKeyboardFocusHere(0);

	ImGui::InputText("SearchFilter", &s_KeySearch);

	for (auto& keyCodeNameLookup : Ball::InputInternal::m_JoystickNameLookup)
	{
		if (!s_KeySearch.empty() && !ContainsIgnoreCase(std::string(keyCodeNameLookup.second), s_KeySearch))
			continue;

		if (ImGui::Button(keyCodeNameLookup.second))
		{
			(*s_RebindInfo.joyStick) = keyCodeNameLookup.first;
			ImGui::CloseCurrentPopup();
			break;
		}
	}
}

void InputViewTool::Draw()
{
	ImGui::Begin(m_Name.c_str(), &m_Open);

	ImGui::BeginTabBar("InputTabBar");

	if (ImGui::BeginTabItem("InputBindings"))
	{
		DrawInputBindings();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Input Debug"))
	{
		DrawDebugInput();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Raw KeyState"))
	{
		DrawRawKeyState();
		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();
	ImGui::End();
}
void InputViewTool::Serialize(Ball::SerializeArchive& archive)
{
	ASSERT_MSG(LOG_INPUT, false, "Serialization for InputViewTool has not been implemented (yet).");
}

void InputViewTool::DrawInputBindings()
{
	auto& input = GetInput();

	if (ImGui::TreeNode("Actions"))
	{
		for (auto& actionPair : input.m_Actions)
		{
			if (ImGui::TreeNode((actionPair.first).c_str()))
			{
				ImGui::Indent(1);
				auto& action = actionPair.second;

				ImGui::Text("State: %s", Ball::Utilities::ToString(input.GetActionRaw(actionPair.first)));

				ImGui::Spacing();

				ImGui::BeginGroup();
				ImGui::Text("Bound keys:");
				ImGui::SameLine();
				for (int i = 0; i < action.m_BoundKeyCount; ++i)
				{
					ImGui::SameLine();

					DrawRebindableButton(action.m_Keys[i], actionPair.first.c_str());

					if (i + 1 >= action.m_BoundKeyCount)
					{
						ImGui::SameLine();
					}
				}

				ImGui::EndGroup();

				if (ImGui::BeginPopup(
						"KeyRebindModal",
						ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
				{
					DrawKeyRebindMenu();

					ImGui::EndPopup();
				}

				ImGui::Indent(-1);
				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Axes"))
	{
		for (auto& axisPair : input.m_Axes)
		{
			if (ImGui::TreeNode((axisPair.first).c_str()))
			{
				auto& Axis = axisPair.second;

				ImGui::BeginGroup();
				ImGui::Text("Bound keys:");

				for (int i = 0; i < sizeof(Axis.m_Keys) / sizeof(Axis.m_Keys[0]); ++i)
				{
					DrawRebindableButton(Axis.m_Keys[i], Axis.m_Name.c_str());

					if (i % 2 == 0)
					{
						ImGui::SameLine();
					}
				}
				ImGui::EndGroup();

				DrawRebindableButton(Axis.m_JoyStick, Axis.m_Name.c_str());

				if (ImGui::BeginPopup(
						"KeyRebindModal",
						ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
				{
					DrawKeyRebindMenu();

					ImGui::EndPopup();
				}

				if (ImGui::BeginPopup(
						"JoyStickRebindModal",
						ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
				{
					DrawJoyStickRebindMenu();

					ImGui::EndPopup();
				}

				ImGui::Text("Value: %f", input.GetAxis(axisPair.first));
				ImGui::InputFloat(("Deadzone##" + axisPair.first).c_str(), &Axis.m_Deadzone);
				ImGui::InputFloat(("Magnitude##" + axisPair.first).c_str(), &Axis.m_Magnitude);

				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}
}

void InputViewTool::DrawRawKeyState()
{
	auto& input = GetInput();

	static struct
	{
		KeyCode key;

		std::vector<KeyState> history;
	} keyHistory;

	if (ImGui::TreeNode("JoySticks"))
	{
		static bool showActiveOnly = false;

		ImGui::Checkbox("ShowActive only", &showActiveOnly);

		for (const auto& keyCodeNameLookup : input.m_AxisValues)
		{
			if (showActiveOnly)
				if (keyCodeNameLookup.second == 0.f)
					continue;

			ImGui::Text("%s: ", Utilities::ToString(keyCodeNameLookup.first));
			ImGui::SameLine();
			ImGui::Text("%f", keyCodeNameLookup.second);
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("RawKeys"))
	{
		static bool showActiveOnly = true;

		ImGui::Checkbox("ShowActive only", &showActiveOnly);

		for (const auto& keyCodeNameLookup : input.m_KeyStates)
		{
			if (showActiveOnly)
				if (keyCodeNameLookup.second == KeyState::NONE)
					continue;

			ImGui::Text("%s", Utilities::ToString(keyCodeNameLookup.first));
			ImGui::SameLine();
			ImGui::TextDisabled("%s", Ball::Utilities::ToString(keyCodeNameLookup.second));

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				if (keyHistory.key != keyCodeNameLookup.first || keyHistory.key == KeyCode::NONE)
				{
					keyHistory.key = keyCodeNameLookup.first;
					keyHistory.history.clear();
				}
				else
				{
					do
					{
						if (keyHistory.history.size() > 0)
						{
							auto last = keyHistory.history[keyHistory.history.size() - 1];
							if (last == keyCodeNameLookup.second)
								continue;
						}

						keyHistory.history.emplace_back(keyCodeNameLookup.second);
					} while (false);
				}

				{
					ImGui::BeginTooltip();
					for (auto& keyState : keyHistory.history)
					{
						ImGui::Text("%s", Utilities::ToString(keyState));
					}
					ImGui::EndTooltip();
				}
			}
		}

		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Raw mouse input"))
	{
		glm::vec2 mousePos = GetInput().GetMousePosition();
		ImGui::Text("Mouse x position: %f", mousePos.x);
		ImGui::Text("Mouse y position: %f", mousePos.y);

		std::string mouseState;
		if (GetInput().GetCursorState() == CursorState::NONE)
			mouseState = "None";
		else if (GetInput().GetCursorState() == CursorState::LOCKED)
			mouseState = "Locked";
		else if (GetInput().GetCursorState() == CursorState::INVISIBLE)
			mouseState = "Invisible";
		else
			mouseState = "Undefined";
		ImGui::Text("Mouse state: %s", mouseState.c_str());

		ImGui::TreePop();
	}
}

void InputViewTool::DrawDebugInput()
{
	auto& input = Ball::GetInput();

	{
		static const char* CursorStateStrings[] = {"NONE", "LOCKED", "INVISIBLE"};
		auto state = (int)input.GetCursorState();
		if (ImGui::ListBox("CursorState", &state, CursorStateStrings, 3))
		{
			input.SetCursorState((CursorState)state);
		}
	}

	ImGui::NewLine();
	ImGui::NewLine();

	ImGui::Separator();
	ImGui::NewLine();

	{
		if (ImGui::SliderFloat("lowFreq Motor", &input.m_Vibrations[0], 0, 1))
			GetInput().SetControllerVibration(MOTORLOWFREQ, input.m_Vibrations[0]);

		if (ImGui::SliderFloat("highFreq Motor", &input.m_Vibrations[1], 0, 1))
			GetInput().SetControllerVibration(MOTORHIGHFREQ, input.m_Vibrations[1]);
	}
}