#pragma once
#include <glm/glm.hpp>

namespace narvalengine {
	enum VolumetricMethod {
		RAY_MARCHING,
		LOBE_SAMPLING,
		MONTE_CARLO
	};

	enum RenderingMode {
		OFFLINE_RENDERING_MODE,
		REALTIME_RENDERING_MODE,
		COMPARE_RENDERING_MODE
	};

	struct Settings {
		glm::ivec2 resolution;
		int spp;
		int bounces;
		RenderingMode renderMode;
		bool hdr;
	};
}