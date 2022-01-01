#include "core/Renderer.h"

namespace narvalengine {
	RendererContext::~RendererContext() {
		delete vertexBufferHandleAllocator;
		delete indexBufferHandleAllocator;
		delete textureHandleAllocator;
		delete frameBufferHandleAllocator;
		delete shaderHandleAllocator;
		delete programHandleAllocator;
		delete uniformHandleAllocator;
		delete meshHandleAllocator;
		delete modelHandleAllocator;
		delete materialHandleAllocator;
	}

	RendererContext::RendererContext() {
		uint8_t* vbAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_VERTEX_BUFFERS * sizeof(HandleID));
		uint8_t* ibAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_INDEX_BUFFERS * sizeof(HandleID));
		uint8_t* framebufferAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_FRAMEBUFFERS * sizeof(HandleID));
		uint8_t* texAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_TEXTURES * sizeof(HandleID));
		uint8_t* shaderAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_SHADERS * sizeof(HandleID));
		uint8_t* programAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_PROGRAMS * sizeof(HandleID));
		uint8_t* uniformAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_UNIFORMS * sizeof(HandleID));
		uint8_t* meshAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_MESHS * sizeof(HandleID));
		uint8_t* modelAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_MODELS * sizeof(HandleID));
		uint8_t* materialAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_MATERIALS * sizeof(HandleID));

		vertexBufferHandleAllocator = createHandleAllocator(vbAlloc, NE_MAX_VERTEX_BUFFERS);
		indexBufferHandleAllocator = createHandleAllocator(ibAlloc, NE_MAX_INDEX_BUFFERS);
		textureHandleAllocator = createHandleAllocator(texAlloc, NE_MAX_TEXTURES);
		frameBufferHandleAllocator = createHandleAllocator(framebufferAlloc, NE_MAX_FRAMEBUFFERS);
		shaderHandleAllocator = createHandleAllocator(shaderAlloc, NE_MAX_SHADERS);
		programHandleAllocator = createHandleAllocator(programAlloc, NE_MAX_PROGRAMS);
		uniformHandleAllocator = createHandleAllocator(uniformAlloc, NE_MAX_UNIFORMS);
		meshHandleAllocator = createHandleAllocator(meshAlloc, NE_MAX_MESHS);
		modelHandleAllocator = createHandleAllocator(modelAlloc, NE_MAX_MODELS);
		materialHandleAllocator = createHandleAllocator(materialAlloc, NE_MAX_MATERIALS);
	}
}
