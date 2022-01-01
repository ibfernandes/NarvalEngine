#pragma once
#include <glm/glm.hpp>

namespace narvalengine {
	enum VolumetricMethod {
		RAY_MARCHING,
		LOBE_SAMPLING,
		MONTE_CARLO
	};

	enum RendererAPIName {
		OPENGL,
		CPU,
		VULKAN
	};

	struct SceneSettings {
		glm::ivec2 resolution{};
		int spp = -1;
		int bounces = -1;
		bool hdr = false;
	};

	/**
	 * All settings that must be set before the program initiates. Set once per program execution.
	 */
	struct Settings {
		RendererAPIName api = CPU;
		int apiVersionMajor = 0;
		int apiVersionMinor = 0;
		glm::ivec2 resolution{};
	};
}