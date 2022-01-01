#pragma once
#include "primitives/Model.h"
#include "core/RendererAPI.h"
#include "utils/Utils.h"
#include "utils/MurmurHash3.h"
#include <queue>

namespace narvalengine {

	struct FrameBufferHandler {
		HandleID id = INVALID_HANDLE;
	};

	struct UniformHandler {
		HandleID id = INVALID_HANDLE;
	};

	struct ShaderHandler {
		HandleID id = INVALID_HANDLE;
	};

	struct ProgramHandler {
		HandleID id = INVALID_HANDLE;
	};

	struct IndexBufferHandler {
		HandleID id = INVALID_HANDLE;
	};

	struct VertexBufferHandler {
		HandleID id = INVALID_HANDLE;
	};

	struct TextureHandler {
		HandleID id = INVALID_HANDLE;
	};

	struct MaterialHandler {
		HandleID id = INVALID_HANDLE;
		TextureHandler textures[TextureName::TextureNameCount] = {
			INVALID_HANDLE, INVALID_HANDLE, INVALID_HANDLE, INVALID_HANDLE, INVALID_HANDLE , INVALID_HANDLE, INVALID_HANDLE, INVALID_HANDLE, INVALID_HANDLE };
	};

	struct MeshHandler {
		HandleID id = INVALID_HANDLE;
		VertexBufferHandler vertexBuffer;
		IndexBufferHandler indexBuffer;
		MaterialHandler material;
	};

	struct ModelHandler {
		HandleID id = INVALID_HANDLE;
		std::vector<MaterialHandler> materials;
		std::vector<MeshHandler> meshes;
	};

	struct Binding {
		enum Enum {
			Texture
		};

		HandleID texID;
		HandleID uniformID;
	};

	struct Attachment {
		TextureHandler texh;
	};

	struct FrameBufferClear {
		FrameBufferHandler fbh;
		float r, g, b, a;
	};

	struct ViewClear {
		uint32_t clearFlags;
		float r, g, b, a;
	};

	class RenderState {
	public:
		std::queue<UniformHandler> uniforms;
		std::queue<Binding> bindings;
		std::queue<Binding> toUnbinding;
		std::queue<ModelHandler> models;
		std::queue<MeshHandler> meshes;
		std::queue<VertexBufferHandler> vertexBuffers;
		std::queue<IndexBufferHandler> indexBuffers;
		FrameBufferHandler fbh = { INVALID_HANDLE };
		std::queue<FrameBufferClear> fbhClear;
		uint64_t stateFlags = NE_STATE_DEFAULT;
		uint32_t stencilFlags = NE_STENCIL_NONE;
		ViewClear viewClear = { NE_CLEAR_NONE, 0, 0, 0, 1 };

		void setTexture(TextureHandler texh, UniformHandler uh) {
			Binding b = { texh.id, uh.id };
			bindings.push(b);
			toUnbinding.push(b);
		}

		void setUniform(UniformHandler uh) {
			uniforms.push(uh);
		}

		void setVertexBuffer(VertexBufferHandler vh) {
			vertexBuffers.push(vh);
		}

		void setIndexBuffer(IndexBufferHandler ih) {
			indexBuffers.push(ih);
		}

		void setModel(ModelHandler mh) {
			models.push(mh);
		}

		void setMesh(MeshHandler mh) {
			 meshes.push(mh);
		}

		void setFrameBuffer(FrameBufferHandler fbh) {
			this->fbh = fbh;
		}

		void setFrameBufferClear(FrameBufferHandler fbh, float r, float g, float b, float a) {
			this->fbhClear.push({ fbh, r, g, b, a });
		}

		void setState(uint64_t flags) {
			stateFlags = flags;
		}

		void setStencil(uint32_t flags) {
			stencilFlags = flags;
		}

		void setClear(uint32_t flags, float r, float g, float b, float a) {
			viewClear = { flags, r, g, b, a };
		}

		void clear() {
			viewClear = { NE_CLEAR_NONE, 0, 0, 0, 1 };
			stateFlags = NE_STATE_DEFAULT;
			uniforms.swap(std::queue<UniformHandler>());
			bindings.swap(std::queue<Binding>());
			toUnbinding.swap(std::queue<Binding>());
			models.swap(std::queue<ModelHandler>());
			meshes.swap(std::queue<MeshHandler>());
			vertexBuffers.swap(std::queue<VertexBufferHandler>());
			indexBuffers.swap(std::queue<IndexBufferHandler>());
			fbh = { INVALID_HANDLE };
			fbhClear.swap(std::queue<FrameBufferClear>());
			stencilFlags = NE_STENCIL_NONE;
		}
	};

	class RendererInterface {
	public:
		virtual void init() = 0;
		virtual void createIndexBuffer(IndexBufferHandler ibh, MemoryBuffer mem, VertexLayout vertexLayout) = 0;
		virtual void createVertexBuffer(VertexBufferHandler vbh, MemoryBuffer mem, VertexLayout vertexLayout) = 0;
		virtual void createUniform(UniformHandler uh, const char* name, MemoryBuffer memBuffer, UniformType::Enum type, int flags) = 0;
		virtual void readTexture(TextureHandler texh, void *data, int mip) = 0;
		virtual void createTexture(TextureHandler texh, int width, int height, int depth, TextureLayout texFormat, MemoryBuffer memBuffer, int flags) = 0;
		virtual void createFrameBuffer(FrameBufferHandler fbh, int len, Attachment *attachments) = 0;
		virtual void createFrameBuffer(FrameBufferHandler fbh, int width, int height, TextureLayout texFormat, TextureLayout depthFormat) = 0;
		virtual void createShader(ShaderHandler shaderh, std::string sourceCode, uint8_t shaderType) = 0;
		virtual void createProgram (ProgramHandler ph, ShaderHandler vertex, ShaderHandler fragment) = 0;
		virtual void updateUniform(UniformHandler uh, MemoryBuffer memBuffer) = 0;
		virtual void updateTexture(TextureHandler texHandler, int offsetX, int offsetY, int offsetZ, int width, int height, int depth, MemoryBuffer mem) = 0;
		virtual void render(ProgramHandler program, RenderState *renderState) = 0;
	};

	/**
	 * Handles the current Renderer Context, i.e., all the currently allocated handlers and which API is in use.
	 */
	class RendererContext {
		public:
			HandleAllocator *vertexBufferHandleAllocator;
			HandleAllocator *indexBufferHandleAllocator;
			HandleAllocator *textureHandleAllocator;
			HandleAllocator *materialHandleAllocator;
			HandleAllocator *frameBufferHandleAllocator;
			HandleAllocator *shaderHandleAllocator;
			HandleAllocator *programHandleAllocator;
			HandleAllocator *uniformHandleAllocator;
			HandleAllocator *meshHandleAllocator;
			HandleAllocator *modelHandleAllocator;
			MurmurHash3 hasher;
			std::map <uint32_t, uint16_t> uniformMap;
			/**
			 * Pointer to an instance of the current selected API for rendering.
			 */
			RendererInterface* renderer;
			RenderState renderState;

			void setRenderer(RendererInterface* renderer) {
				this->renderer = renderer;
			}

			~RendererContext();
			RendererContext();

			IndexBufferHandler createIndexBuffer(MemoryBuffer mem, VertexLayout vertexLayout) {
				IndexBufferHandler ibh = { indexBufferHandleAllocator->alloc() };

				renderer->createIndexBuffer(ibh, mem, vertexLayout);

				return ibh;
			}

			VertexBufferHandler createVertexBuffer(MemoryBuffer mem, VertexLayout vertexLayout) {
				VertexBufferHandler vbh = { vertexBufferHandleAllocator->alloc() };

				renderer->createVertexBuffer(vbh, mem, vertexLayout);

				return vbh;
			}

			UniformHandler createUniform(const char* name, MemoryBuffer memBuffer, UniformType::Enum type, int flags) {
				UniformHandler uh = { uniformHandleAllocator->alloc() };

				uint32_t hash = hasher.hash(name, std::string(name).length());

				renderer->createUniform(uh, name, memBuffer, type, flags);

				return uh;
			}

			TextureHandler createTexture(int width, int height, int depth, TextureLayout texFormat, MemoryBuffer memBuffer, int flags) {
				TextureHandler texh = { textureHandleAllocator->alloc() };

				renderer->createTexture(texh, width, height, depth, texFormat, memBuffer, flags);

				return texh;
			}

			TextureHandler createTexture(Texture* texture) {
				return createTexture(texture->width, texture->height, texture->depth, texture->texFormat, texture->mem, texture->samplerFlags);
			}

			FrameBufferHandler createFrameBuffer(int length, Attachment *attachments) {
				FrameBufferHandler fbh = { frameBufferHandleAllocator->alloc() };

				renderer->createFrameBuffer(fbh, length, attachments);

				return fbh;
			}

			FrameBufferHandler createFrameBuffer(int width, int height, TextureLayout texFormat, TextureLayout depthFormat) {
				FrameBufferHandler fbh = { frameBufferHandleAllocator->alloc() };
				MemoryBuffer texData = {};
				TextureHandler th = createTexture(width, height, 0, texFormat, texData, 0);
				Attachment tex = {th};

				renderer->createFrameBuffer(fbh, 1, &tex);
				return fbh;
			}

			ShaderHandler createShader(std::string sourceCode, uint8_t shaderType) {
				ShaderHandler sh = { shaderHandleAllocator->alloc() };

				renderer->createShader(sh, sourceCode, shaderType);

				return sh;
			}

			ProgramHandler createProgram(ShaderHandler vertex, ShaderHandler fragment) {
				ProgramHandler ph = { programHandleAllocator->alloc() };

				renderer->createProgram(ph, vertex, fragment);

				return ph;
			}

			MeshHandler createMesh(Mesh* mesh, MaterialHandler mh = { INVALID_HANDLE }) {
				MeshHandler meshHandler = { meshHandleAllocator->alloc() };
				VertexBufferHandler vbh;
				IndexBufferHandler ibh;

				MemoryBuffer memvb;
				memvb.data = (uint8_t*)(mesh->vertexDataPointer);
				memvb.size = mesh->vertexDataPointerLength * sizeof(float);

				MemoryBuffer memib;
				memib.data = (uint8_t*)(mesh->vertexIndicesPointer);
				memib.size = mesh->vertexIndicesPointerLength * sizeof(float);

				meshHandler.vertexBuffer = createVertexBuffer(memvb, mesh->vertexLayout);
				meshHandler.indexBuffer = createIndexBuffer(memib, mesh->vertexLayout);

				if (mh.id != INVALID_HANDLE)
					meshHandler.material = mh;
				else {
					//no material associated with this mesh.
				}

				return meshHandler;
			}

			MaterialHandler createMaterial(Material* material) {
				MaterialHandler matHandler = { materialHandleAllocator->alloc() };

				for (Texture* t : material->textures)
					if(t!=nullptr)
						matHandler.textures[ctz(t->textureName)] = createTexture(t);

				return matHandler;
			}

			ModelHandler createModel(Model *model) {
				ModelHandler modelhandler = { modelHandleAllocator->alloc() };

				for (Material *m : model->materials)
					modelhandler.materials.push_back(createMaterial(m));

				for (Mesh m : model->meshes)
					if(m.modelMaterialIndex < 0)
						modelhandler.meshes.push_back(createMesh(&m));
					else
						modelhandler.meshes.push_back(createMesh(&m, modelhandler.materials.at(m.modelMaterialIndex)));

				return modelhandler;
			}

			void updateUniform(UniformHandler uh, MemoryBuffer memBuffer) {
				renderer->updateUniform(uh, memBuffer);
			}

			void readTexture(TextureHandler texHandler, void *data, int mip = 0) {
				renderer->readTexture(texHandler, data, mip);
			}

			void updateTexture(TextureHandler texHandler, int offsetX, int offsetY, int offsetZ, int width, int height, int depth, MemoryBuffer mem) {
				renderer->updateTexture(texHandler, offsetX, offsetY, offsetZ, width, height, depth, mem);
			}

			void setUniform(UniformHandler uh) {
				renderState.setUniform(uh);
			}

			void setTexture(TextureHandler texh, UniformHandler uh) {
				renderState.setTexture(texh, uh);
			}

			void setModel(ModelHandler modelh) {
				renderState.setModel(modelh);
			}

			void setMesh(MeshHandler meshH) {
				renderState.setMesh(meshH);
			}

			void setVertexBuffer(VertexBufferHandler vh) {
				renderState.setVertexBuffer(vh);
			}

			void setIndexBuffer(IndexBufferHandler ih) {
				renderState.setIndexBuffer(ih);
			}

			void setFrameBuffer(FrameBufferHandler fbh) {
				renderState.setFrameBuffer(fbh);
			}

			void setFrameBufferClear(FrameBufferHandler fbh, float r, float g, float b, float a) {
				renderState.setFrameBufferClear(fbh, r, g, b, a);
			}

			void setStencil(uint32_t flags) {
				renderState.setStencil(flags);
			}

			void setState(uint64_t flags) {
				renderState.setState(flags);
			}

			void setClear(uint32_t flags, float r, float g, float b, float a) {
				renderState.setClear(flags, r, g, b, a);
			}

			void render(ProgramHandler ph) {
				renderer->render(ph, &renderState);
				renderState.clear();
			}
	};
}

