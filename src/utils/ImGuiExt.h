#pragma once
#include "imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"
#include "addons/ImGuizmo/ImGuizmo.h"
#include <glm/glm.hpp>
#include <string>

/**
 * ImGui and Addons extensions.
 */
namespace ImGuiExt{
	struct vec_t;
	/**
	 * Extension supporting change of text color for the selected item.
	 */
	bool ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_items, ImVec4 activeTextColor);
    bool ListBox(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int height_in_items, ImVec4 activeTextColor);
	bool CollapsingHeader(const char* label, ImVec4 shadowColor, ImGuiTreeNodeFlags flags = 0);
	void TextVec3(glm::vec3 v, std::string name = "");
	ImVec4 add(ImVec4 vec4, float v);
};
