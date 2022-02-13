#pragma once
#include "core/Renderer.h"

namespace narvalengine {
	class StandartSurfaceEditor {
	public:
		RendererContext* renderCtx;
		
		//Shader Parameters
		ProgramHandler programHandler;
		static const int maxUniforms = 100;
		UniformHandler uniforms[maxUniforms];
		UniformHandler lightUniforms[maxUniforms];
		//Number of varaible members in the light struct.
		const int lightsOffset = 2;
		bool isMaterialSet[7]{};
		int textureIds[8] = { 0, 1, 2, 3, 4, 5, 6, 8 };
		
		int shadowMapId = 7;
		float time = 0;
		glm::vec3 sheenColor = glm::vec3(1, 1, 1);

		TextureHandler preIntegratedSSSBRDFLUTTex;

		//References
		glm::mat4* model;
		glm::mat4* cam;
		glm::mat4* proj;
		glm::mat4* lightView;
		glm::mat4* lightProjection;
		glm::vec3* cameraPosition;
		int *numberOfActiveLights;
		glm::vec3* lightPos[10];
		glm::vec4* defaultLightColor;
		glm::ivec2* renderResolution;

		void init();
		void setAllUniforms(const uint32_t nLights);
	};
}
