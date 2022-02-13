#pragma once
#include "imgui.h"

namespace narvalengine {
	/**
	 *  Global base definitions for ImGui dimensions.
	 */
	struct ImGuiStyleDefinitions {
		const ImVec2 windowPadding{ 15, 15 };
	};

	const inline ImGuiStyleDefinitions imGuiStyleDefs;
}

