#pragma once
#include <glm/glm.hpp>

enum RenderingMode {
	OFFLINE_RENDERING_MODE,
	REALTIME_RENDERING_MODE
};

struct Settings {
	glm::ivec2 resolution;
	int spp;
	int bounces;
	RenderingMode renderMode;
};