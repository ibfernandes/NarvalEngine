#pragma once
#include "primitives/Model.h"
#include "core/RendererAPI.h"
#include "utils/Utils.h"
#include "utils/MurmurHash3.h"
#include <queue>

namespace narvalengine {

	struct FrameBufferHandler {
		uint16_t id;
	};

	struct UniformHandler {
		uint16_t id;
	};

	struct ShaderHandler {
		uint16_t id;
	};

	struct ProgramHandler {
		uint16_t id;
	};

	struct IndexBufferHandler {
		uint16_t id;
	};

	struct VertexBufferHandler {
		uint16_t id;
	};

	struct TextureHandler {
		uint16_t id;
	};

	struct MaterialHandler {
		uint16_t id;
		TextureHandler textures[TextureName::TextureNameCount] = {
			INVALID_HANDLE, INVALID_HANDLE, INVALID_HANDLE, INVALID_HANDLE, INVALID_HANDLE , INVALID_HANDLE, INVALID_HANDLE, INVALID_HANDLE, INVALID_HANDLE };
	};

	struct MeshHandler {
		uint16_t id;
		VertexBufferHandler vertexBuffer;
		IndexBufferHandler indexBuffer;
		MaterialHandler material;
		//std::vector<TextureHandler> textures;
	};

	struct ModelHandler {
		uint16_t id;
		std::vector<MaterialHandler> materials;
		std::vector<MeshHandler> meshes;
	};

	struct Binding {
		enum Enum {
			Texture
		};

		uint16_t texID;
		uint16_t uniformID;
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
		virtual void createIndexBuffer(IndexBufferHandler ibh, MemoryBuffer mem, VertexLayout vertexLayout) = 0;
		virtual void createVertexBuffer(VertexBufferHandler vbh, MemoryBuffer mem, VertexLayout vertexLayout) = 0;
		virtual void createUniform(UniformHandler uh, const char* name, MemoryBuffer memBuffer, UniformType::Enum type, int flags) = 0;
		virtual void createTexture(TextureHandler texh, int width, int height, int depth, TextureLayout texFormat, MemoryBuffer memBuffer, int flags) = 0;
		virtual void createFrameBuffer(FrameBufferHandler fbh, int len, Attachment *attachments) = 0;
		virtual void createFrameBuffer(FrameBufferHandler fbh, int width, int height, TextureLayout texFormat, TextureLayout depthFormat) = 0;
		virtual void createShader(ShaderHandler shaderh, std::string sourceCode, uint8_t shaderType) = 0;
		virtual void createProgram (ProgramHandler ph, ShaderHandler vertex, ShaderHandler fragment) = 0;
		virtual void updateUniform(UniformHandler uh, MemoryBuffer memBuffer) = 0;
		virtual void updateTexture(TextureHandler texHandler, int offsetX, int offsetY, int offsetZ, int width, int height, int depth, MemoryBuffer mem) = 0;
		virtual void render(ProgramHandler program, RenderState *renderState) = 0;
	};

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
			//Maps a hash code to a handleAllocatorID
			//TODO: proper hashmap to handle collisions
			MurmurHash3 hasher;
			std::map <uint32_t, uint16_t> uniformMap;
			RendererInterface* renderer;
			RenderState renderState;

			RendererContext() {
				uint8_t *vbAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_VERTEX_BUFFERS * sizeof(uint16_t));
				uint8_t *ibAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_INDEX_BUFFERS * sizeof(uint16_t));
				uint8_t *framebufferAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_FRAMEBUFFERS * sizeof(uint16_t));
				uint8_t *texAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_TEXTURES * sizeof(uint16_t));
				uint8_t *shaderAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_SHADERS * sizeof(uint16_t));
				uint8_t *programAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_PROGRAMS * sizeof(uint16_t));
				uint8_t *uniformAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_UNIFORMS * sizeof(uint16_t));
				uint8_t *meshAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_MESHS * sizeof(uint16_t));
				uint8_t *modelAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_MODELS * sizeof(uint16_t));
				uint8_t *materialAlloc = memAlloc(sizeof(HandleAllocator) + 2 * NE_MAX_MATERIALS * sizeof(uint16_t));

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
				//uniformMap.insert({hash, uh.id});

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

				//renderer->createFrameBuffer(fbh, width, height, texFormat, depthFormat);
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
				memvb.size = mesh->vertexDataPointerLength * 4; //TODO 4 bytes for a float ( should Enum this)

				MemoryBuffer memib;
				memib.data = (uint8_t*)(mesh->vertexIndicesPointer);
				memib.size = mesh->vertexIndicesPointerLength * 4; //TODO 4 bytes for a float ( should Enum this)

				meshHandler.vertexBuffer = createVertexBuffer(memvb, mesh->vertexLayout);
				meshHandler.indexBuffer = createIndexBuffer(memib, mesh->vertexLayout);

				if (mh.id != INVALID_HANDLE)
					meshHandler.material = mh;
				else {
					//no material associated with this mesh.
				}

				/*for (TextureInfo ti : mesh->textures) {
					Texture* t = ResourceManager::getSelf()->getTexture(ti.texID);
					if (t == nullptr)
						continue;
					meshHandler.textures.push_back(createTexture(t));
				}*/

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

