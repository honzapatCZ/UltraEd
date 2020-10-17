#pragma once

#include <windows.h>
#include <functional>
#include <memory>
#include <string>
#include <ImGui/imgui.h>
#include <tuple>
#include <ImGui\imgui_internal.h>
namespace UltraEd {
	class Gui_Widgets
	{
	public:
		static void ToggleButton(const char* str_id, bool* v);
	};
}

