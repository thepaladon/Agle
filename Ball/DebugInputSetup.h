#pragma once

#include "Engine.h"
#include "Input/Input.h"

inline void DebugInputSetup()
{
#ifndef SHIPPING
	// The f12 on your browser toggle
	Ball::GetInput().CreateAction("InspectUI").AddKeyBind(Ball::KEY_F11);

	// Debug tools controls
	auto& buttonPressDL = Ball::GetInput().CreateAction("On_Off_Debug_Lines");
	buttonPressDL.AddKeyBind(Ball::KEY_L);
	buttonPressDL.AddKeyBind(Ball::GAMEPAD_L3);

	// Controls for testing the level switching
	auto& switchLevelFileButton = Ball::GetInput().CreateAction("SwitchLevelFileTest");
	switchLevelFileButton.AddKeyBind(Ball::KEY_N);
	switchLevelFileButton.AddKeyBind(Ball::GAMEPAD_L1);

	auto& switchLevelTypeButton = Ball::GetInput().CreateAction("SwitchLevelTypeTest");
	switchLevelTypeButton.AddKeyBind(Ball::KEY_M);
	switchLevelTypeButton.AddKeyBind(Ball::GAMEPAD_R1);

	// Toolmanager controls
	auto& toggleToolmanager = Ball::GetInput().CreateAction("ToolOverlay");
	toggleToolmanager.AddKeyBind(Ball::KEY_F1);
	toggleToolmanager.AddKeyBind(Ball::GAMEPAD_START);

	// Freecam toggle
	Ball::GetInput().CreateAction("FreeCameraToggle").AddKeyBind(Ball::KEY_F2);

#endif

	auto& screenshot = Ball::GetInput().CreateAction("TakeScreenshot");
	screenshot.AddKeyBind(Ball::KEY_PRINTSCREEN);

	Ball::GetInput().CreateAction("Escape").AddKeyBind(Ball::KEY_ESCAPE).AddKeyBind(Ball::GAMEPAD_START);
}