#pragma once
#include "imgui.h"

namespace narvalengine {
	/**
	 * Palette of colors defining the overall feel and aesthetic of the user interface (UI).
	 * The default is white color.
	 */
	struct Palette {
		ImVec4 text = ImVec4(53 / 255.0f, 53 / 255.0f, 53 / 255.0f, 1.0f);
		ImVec4 textLight = ImVec4(73 / 255.0f, 73 / 255.0f, 73 / 255.0f, 1.0f);

		ImVec4 iceGrey[6] = {
			ImVec4(243 / 255.0f, 247 / 255.0f, 248 / 255.0f, 1.0f),
			ImVec4(223 / 255.0f, 227 / 255.0f, 228 / 255.0f, 1.0f),
			ImVec4(203 / 255.0f, 207 / 255.0f, 208 / 255.0f, 1.0f),
			ImVec4(183 / 255.0f, 187 / 255.0f, 188 / 255.0f, 1.0f),
			ImVec4(153 / 255.0f, 157 / 255.0f, 158 / 255.0f, 1.0f),
			ImVec4(133 / 255.0f, 137 / 255.0f, 138 / 255.0f, 1.0f)
		};

		ImVec4 blueGrey[3] = {
			ImVec4(183 / 255.0f, 195 / 255.0f, 203 / 255.0f, 1.0f),
			ImVec4(112 / 255.0f, 135 / 255.0f, 151 / 255.0f, 1.0f),
			ImVec4(40 / 255.0f, 75 / 255.0f, 99 / 255.0f, 1.0f)
		};

		ImVec4 green[3] = {
			ImVec4(114 / 255.0f, 179 / 255.0f, 0 / 255.0f, 1.0f),
			ImVec4(94 / 255.0f, 159 / 255.0f, 0 / 255.0f, 1.0f),
			ImVec4(74 / 255.0f, 139 / 255.0f, 0 / 255.0f, 1.0f)
		};

		ImVec4 blue[3] = {
			ImVec4(64 / 255.0f, 133 / 255.0f, 237 / 255.0f, 1.0f),
			ImVec4(44 / 255.0f, 113 / 255.0f, 217 / 255.0f, 1.0f),
			ImVec4(24 / 255.0f, 93 / 255.0f, 197 / 255.0f, 1.0f)
		};

		ImVec4 red[3] = {
			ImVec4(184 / 255.0f, 56 / 255.0f, 79 / 255.0f, 1.0f),
			ImVec4(164 / 255.0f, 36 / 255.0f, 59 / 255.0f, 1.0f),
			ImVec4(144 / 255.0f, 16 / 255.0f, 39 / 255.0f, 1.0f)
		};

		ImVec4 yellow[3] = {
			ImVec4(237 / 255.0f, 193 / 255.0f, 40 / 255.0f, 1.0f),
			ImVec4(237 / 255.0f, 193 / 255.0f, 40 / 255.0f, 1.0f),
			ImVec4(237 / 255.0f, 193 / 255.0f, 40 / 255.0f, 1.0f)
		};

		Palette(bool darkMode = false) {
			if (darkMode) {
				text = ImVec4(214 / 255.0f, 218 / 255.0f, 219 / 255.0f, 1.0f);
				textLight = ImVec4(232 / 255.0f, 235 / 255.0f, 236 / 255.0f, 1.0f);
				iceGrey[0] = ImVec4(77 / 255.0f, 77 / 255.0f, 77 / 255.0f, 1.0f);
				iceGrey[1] = ImVec4(63 / 255.0f, 64 / 255.0f, 64 / 255.0f, 1.0f);
				iceGrey[2] = ImVec4(54 / 255.0f, 55 / 255.0f, 55 / 255.0f, 1.0f);
				iceGrey[3] = ImVec4(43 / 255.0f, 44 / 255.0f, 44 / 255.0f, 1.0f);
				iceGrey[4] = ImVec4(32 / 255.0f, 33 / 255.0f, 33 / 255.0f, 1.0f);
				iceGrey[5] = ImVec4(20 / 255.0f, 21 / 255.0f, 21 / 255.0f, 1.0f);
			}
		}
	};

	const inline Palette gUIPalette(false);

	/**
	 * Takes a normalized color in the range [0, 1] and converts to integer 32 bits representation in RGBA format.
	 * 
	 * @param color to be converted.
	 * @return color as an 32-bit integer.
	 */
	inline int colorToInt32(ImVec4 color) {
		return IM_COL32(color.x * 255, color.y * 255, color.z * 255, color.w * 255);
	}
};