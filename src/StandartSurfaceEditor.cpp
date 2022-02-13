#include "StandartSurfaceEditor.h"
#include "core/Engine3D.h"

namespace narvalengine {
	void StandartSurfaceEditor::init() {
		renderCtx = &Engine3D::getSelf()->renderer;

		ResourceManager::getSelf()->loadShader("standartSurface", "shaders/uber.vert", "shaders/uber.frag", "");
		Shader* shader = ResourceManager::getSelf()->getShader("standartSurface");
		ShaderHandler shvertex = renderCtx->createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx->createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		programHandler = renderCtx->createProgram(shvertex, shfragment);

		uniforms[0] = renderCtx->createUniform("model", { model, sizeof(model) }, UniformType::Mat4, 0);
		uniforms[1] = renderCtx->createUniform("cam", { cam, sizeof(cam) }, UniformType::Mat4, 0);
		uniforms[2] = renderCtx->createUniform("proj", { proj, sizeof(proj) }, UniformType::Mat4, 0);
		uniforms[3] = renderCtx->createUniform("lightCam", { lightView, sizeof(lightView) }, UniformType::Mat4, 0);
		uniforms[4] = renderCtx->createUniform("lightProj", { lightProjection, sizeof(lightProjection) }, UniformType::Mat4, 0);

		uniforms[5] = renderCtx->createUniform("material.diffuse", { &textureIds[0], sizeof(int)}, UniformType::Sampler, 0);
		uniforms[6] = renderCtx->createUniform("material.metallic", { &textureIds[1], sizeof(int)}, UniformType::Sampler, 0);
		uniforms[7] = renderCtx->createUniform("material.normal", { &textureIds[3], sizeof(int)}, UniformType::Sampler, 0);
		uniforms[8] = renderCtx->createUniform("material.roughness", { &textureIds[4], sizeof(int)}, UniformType::Sampler, 0);
		uniforms[9] = renderCtx->createUniform("material.subsurface", { &textureIds[5], sizeof(int)}, UniformType::Sampler, 0);
		uniforms[10] = renderCtx->createUniform("material.ao", { &textureIds[6], sizeof(int)}, UniformType::Sampler, 0);

		uniforms[11] = renderCtx->createUniform("materialIsSet.diffuse", { &isMaterialSet[0], sizeof(bool)}, UniformType::Bool, 0);
		uniforms[12] = renderCtx->createUniform("materialIsSet.metallic", { &isMaterialSet[1], sizeof(bool)}, UniformType::Bool, 0);
		uniforms[13] = renderCtx->createUniform("materialIsSet.specular", { &isMaterialSet[2], sizeof(bool)}, UniformType::Bool, 0);
		uniforms[14] = renderCtx->createUniform("materialIsSet.normal", { &isMaterialSet[3], sizeof(bool)}, UniformType::Bool, 0);
		uniforms[15] = renderCtx->createUniform("materialIsSet.roughness", { &isMaterialSet[4], sizeof(bool)}, UniformType::Bool, 0);
		uniforms[16] = renderCtx->createUniform("materialIsSet.subsurface", { &isMaterialSet[5], sizeof(bool)}, UniformType::Bool, 0);
		uniforms[17] = renderCtx->createUniform("materialIsSet.ao", { &isMaterialSet[6], sizeof(bool)}, UniformType::Bool, 0);
		uniforms[18] = renderCtx->createUniform("materialIsSet.isLight", { &isMaterialSet[6], sizeof(bool)}, UniformType::Bool, 0);
		
		uniforms[19] = renderCtx->createUniform("cameraPosition", { cameraPosition, sizeof(glm::vec3)}, UniformType::Vec3, 0);
		uniforms[20] = renderCtx->createUniform("shadowMap", { &shadowMapId, sizeof(int)}, UniformType::Sampler, 0);
		uniforms[21] = renderCtx->createUniform("time", { &time, sizeof(float)}, UniformType::Float, 0);
		uniforms[22] = renderCtx->createUniform("sheenColor", { &sheenColor, sizeof(glm::vec3)}, UniformType::Vec3, 0);
		uniforms[23] = renderCtx->createUniform("numberOfLights", { numberOfActiveLights, sizeof(int)}, UniformType::Int, 0);
		uniforms[24] = renderCtx->createUniform("preIntegratedSSSBRDFLUT", { &textureIds[7], sizeof(int)}, UniformType::Sampler, 0);

		lightUniforms[0] = renderCtx->createUniform("lightPoints[0].position", { lightPos[0], sizeof(glm::vec3)}, UniformType::Vec3, 0);
		lightUniforms[1] = renderCtx->createUniform("lightPoints[0].color", { defaultLightColor, sizeof(glm::vec3) }, UniformType::Vec3, 0);
		
		preIntegratedSSSBRDFLUTTex = renderCtx->createTexture(ResourceManager::getSelf()->getTexture("skinLUT"));
	}

	void StandartSurfaceEditor::setAllUniforms(const uint32_t nLights) {
		for (int i = 0; i < maxUniforms; i++) 
			if (isHandleValid(uniforms[i].id))
				renderCtx->setUniform(uniforms[i]);
			else
				break;

		for (int i = 0; i < nLights * lightsOffset; i++) 
			if (isHandleValid(lightUniforms[i].id))
				renderCtx->setUniform(lightUniforms[i]);
			else
				break;
	}
}
