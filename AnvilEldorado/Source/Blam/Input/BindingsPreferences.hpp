#pragma once
#include "KeyCode.hpp"
#include "MouseButton.hpp"
#include "GameAction.hpp"

namespace Blam::Input
{
	struct BindingsPreferences
	{
		KeyCode PrimaryKeys[eGameAction_KeyboardMouseCount];
		MouseButton PrimaryMouseButtons[eGameAction_KeyboardMouseCount];
		KeyCode SecondaryKeys[eGameAction_KeyboardMouseCount];
		MouseButton SecondaryMouseButtons[eGameAction_KeyboardMouseCount];
	};
	static_assert(sizeof(BindingsPreferences) == 0x17C, "Invalid BindingsPreferences size");
}