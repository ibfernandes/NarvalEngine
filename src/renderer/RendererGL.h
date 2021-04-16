#pragma once
#include "materials/Texture.h"
#include "utils/Utils.h"
#include "core/Renderer.h"
#include <iostream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace narvalengine {
	static GLenum filterFlag(uint64_t flags, uint64_t mask, uint64_t shift) {
		return ((flags & mask) >> shift) - 1;
	}

	inline static int GLgetNumberOfChannels(GLenum texFormat) {
		if (texFormat == GL_RED) {
			return 1;
		}else if (texFormat == GL_RG) {
			return 2;
		}else if (texFormat == GL_RGB) {
			return 3;
		}else if (texFormat == GL_RGBA) {
			return 4;
		}else {
			return 0;
		}
	};

	//array[0] is the default!
	struct GLTexFormatInfo {
		GLenum internalFormat;
		GLint format;
		GLenum type;
	};

	static const GLenum GLShaderType[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_GEOMETRY_SHADER };
	static const GLenum GLTextureWrap[] = { GL_CLAMP_TO_EDGE, GL_REPEAT, GL_CLAMP_TO_BORDER };
	static const GLenum GLTextureFilter[] = { GL_LINEAR, GL_NEAREST };
	//Must closely match TexFormat from "Texture.h"
	static const GLTexFormatInfo GLTexFormat[] = { {}, {}, {GL_RGB32I, GL_RGB_INTEGER, GL_INT},
		{}, {GL_R32F, GL_RED, GL_FLOAT}, {},
		{GL_RGB32F, GL_RGB, GL_FLOAT}, {GL_RGBA32F, GL_RGBA, GL_FLOAT}, {}, {},
		{GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE}, {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE}, 
		{GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT}, 
		{GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8} };

	static const GLenum GLstencilTest[] ={
		GL_LESS,
		GL_LEQUAL,
		GL_EQUAL,
		GL_GEQUAL,
		GL_GREATER,
		GL_NOTEQUAL,
		GL_NEVER,
		GL_ALWAYS,
	};

	static const GLenum GLstencilOp[] = {
		GL_ZERO,
		GL_KEEP,
		GL_REPLACE,
		GL_INCR_WRAP,
		GL_INCR,
		GL_DECR_WRAP,
		GL_DECR,
		GL_INVERT,
	};

	static const GLenum GLrenderMode[] = {
		GL_TRIANGLES,
		GL_TRIANGLE_STRIP,
		GL_LINES,        
		GL_LINE_STRIP,   
		GL_POINTS       
	};

	static void checkCompileErrors(int object, std::string type) {
		int success;
		GLchar infoLog[512];

		if (type != "PROGRAM") {
			glGetShaderiv(object, GL_COMPILE_STATUS, &success);

			if (success == 0) {
				glGetShaderInfoLog(object, 512, NULL, infoLog);
				std::cout << infoLog << std::endl;
			}

		}
		else {
			glGetProgramiv(object, GL_LINK_STATUS, &success);

			if (success == 0) {
				glGetProgramInfoLog(object, 512, NULL, infoLog);
				std::cout << infoLog << std::endl;
			}

		}
	}

	struct ShaderGL {
		GLuint id;

		//TODO change source from strind to MemoryBuffer
		void compile(std::string source, uint8_t shaderType) {
			id = glCreateShader(GLShaderType[shaderType - 1]);
			const char* c_str = 0;

			glShaderSource(id, 1, &(c_str = source.c_str()), NULL);
			glCompileShader(id);
			checkCompileErrors(id, "Shader");
		};
	};

	struct UniformGL {
		MemoryBuffer mem;
		UniformType::Enum type;
		std::string name;
	};

	struct ProgramGL {
		GLuint id;

		void use() {
			glUseProgram(id);
		};

		void compile(ShaderGL vertexShader, ShaderGL fragmentShader) {
			id = glCreateProgram();
			glAttachShader(id, vertexShader.id);
			glAttachShader(id, fragmentShader.id);
			glLinkProgram(id);
			checkCompileErrors(id, "PROGRAM");
		}

		void setFloat(std::string name, float value) {
			glUniform1f(glGetUniformLocation(id, name.c_str()), value);
		}

		void setFloatArray(std::string name, int length, float* value) {
			glUniform1fv(glGetUniformLocation(id, name.c_str()), length, value);
		}

		void setInteger(std::string name, int value) {
			glUniform1i(glGetUniformLocation(id, name.c_str()), value);
		}

		void setIntegerArray(std::string name, int length, int* value) {
			glUniform1iv(glGetUniformLocation(id, name.c_str()), length, value);
		}

		void setVec2(std::string name, float x, float y) {
			glUniform2f(glGetUniformLocation(id, name.c_str()), x, y);
		}

		void setIntegerVec2(std::string name, glm::ivec2 vec) {
			glUniform2i(glGetUniformLocation(id, name.c_str()), vec.x, vec.y);
		}

		void setVec3(std::string name, float x, float y, float z) {
			glUniform3f(glGetUniformLocation(id, name.c_str()), x, y, z);
		}

		void setVec3(std::string name, glm::vec3 vec) {
			glUniform3f(glGetUniformLocation(id, name.c_str()), vec.x, vec.y, vec.z);
		}
		void setIntegerVec3(std::string name, glm::ivec3 vec) {
			glUniform3i(glGetUniformLocation(id, name.c_str()), vec.x, vec.y, vec.z);
		}

		void setVec4(std::string name, float x, float y, float z, float w) {
			glUniform4f(glGetUniformLocation(id, name.c_str()), x, y, z, w);
		}

		void setVec4(std::string name, glm::vec4 vec) {
			glUniform4f(glGetUniformLocation(id, name.c_str()), vec.x, vec.y, vec.z, vec.w);
		}

		void setMat4(std::string name, glm::mat4 mat) {
			if (glGetUniformLocation(id, name.c_str()) == -1) {
				std::cout << "ERROR: uniform " + name + " not found";
				exit(1);
			}

			glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
		}
	};

	struct IndexBufferGL {
		GLuint id;
		int size;

		void create(MemoryBuffer mem) {
			this->size = mem.size;
			glGenBuffers(1, &id);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, mem.size, mem.data, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	};

	struct VertexBufferGL {
		GLuint id;
		VertexLayout vertexLayout;

		void create(MemoryBuffer mem, VertexLayout vertexLayout) {
			this->vertexLayout = vertexLayout;
			glGenBuffers(1, &id);
			glBindBuffer(GL_ARRAY_BUFFER, id);
			glBufferData(GL_ARRAY_BUFFER, mem.size, mem.data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		void bindAttributesBegin() {

		}

		void bindAttributes() {
			int j = 0;
			for (int a = 0; a < vertexLayout.activeAttribs; a++) {
				glEnableVertexAttribArray(a);

				for (int i = j; i < VertexAttrib::Count; i++) {
					if (vertexLayout.qtt[i] > 0) {
						glVertexAttribPointer(a, vertexLayout.qtt[i], GL_FLOAT, GL_FALSE, vertexLayout.stride, (GLvoid*)(vertexLayout.offset[i]));
						j = i + 1;
						break;
					}
				}
			}

		}

		void bindAttributesEnd() {

		}
	};

	class TextureGL {
	public:
		MemoryBuffer mem;
		GLuint id;
		TextureLayout texLayout;
		GLTexFormatInfo glTexFormatInfo;
		int flags;
		GLenum target;
		int width, height, depth;

		void createTexture2D(int width, int height, TextureLayout texFormat, int flags, MemoryBuffer mem){
			//assert(width > 0 && height > 0);
			this->mem = mem;
			target = GL_TEXTURE_2D;

			glGenTextures(1, &(this->id));
			glBindTexture(GL_TEXTURE_2D, id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GLTextureWrap[filterFlag(flags, NE_TEX_SAMPLER_U_MASK, NE_TEX_SAMPLER_U_SHIFT)]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GLTextureWrap[filterFlag(flags, NE_TEX_SAMPLER_V_MASK, NE_TEX_SAMPLER_V_SHIFT)]);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GLTextureFilter[filterFlag(flags, NE_TEX_SAMPLER_MIN_MASK, NE_TEX_SAMPLER_MIN_SHIFT)]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GLTextureFilter[filterFlag(flags, NE_TEX_SAMPLER_MAG_MASK, NE_TEX_SAMPLER_MAG_SHIFT)]);

			glTexFormatInfo = GLTexFormat[texFormat];
			glTexImage2D(GL_TEXTURE_2D, 0, glTexFormatInfo.internalFormat, width, height, 0, glTexFormatInfo.format, glTexFormatInfo.type, mem.data);

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		void createTexture3D(int width, int height, int depth, TextureLayout texFormat, int flags, MemoryBuffer mem) {
			//assert(width > 0 && height > 0);
			this->mem = mem;
			target = GL_TEXTURE_3D;

			glGenTextures(1, &(this->id));
			glBindTexture(GL_TEXTURE_3D, id);

			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GLTextureWrap[filterFlag(flags, NE_TEX_SAMPLER_U_MASK, NE_TEX_SAMPLER_U_SHIFT)]);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GLTextureWrap[filterFlag(flags, NE_TEX_SAMPLER_V_MASK, NE_TEX_SAMPLER_V_SHIFT)]);

			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GLTextureFilter[filterFlag(flags, NE_TEX_SAMPLER_MIN_MASK, NE_TEX_SAMPLER_MIN_SHIFT)]);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GLTextureFilter[filterFlag(flags, NE_TEX_SAMPLER_MAG_MASK, NE_TEX_SAMPLER_MAG_SHIFT)]);

			glTexFormatInfo = GLTexFormat[texFormat];
			glTexImage3D(GL_TEXTURE_3D, 0, glTexFormatInfo.internalFormat, width, height, depth, 0, glTexFormatInfo.format, glTexFormatInfo.type, mem.data);

			glBindTexture(GL_TEXTURE_3D, 0);
		}

		void createTexture(int width, int height, int depth, TextureLayout texFormat, int flags, MemoryBuffer mem) {
			texLayout = texFormat;
			this->width = width;
			this->height = height;
			this->depth = depth;
			
			if (width > 0 && height > 0 && depth > 0) {
				target = GL_TEXTURE_3D;
				createTexture3D(width, height, depth, texFormat, flags, mem);
			}else if (width > 0 && height > 0) {
				target = GL_TEXTURE_2D;
				createTexture2D(width, height, texFormat, flags, mem);
			}else if (width > 0) {

			}
		}

		void updateTexture2D(int offsetX, int offsetY, int width, int height, MemoryBuffer mem) {
			glBindTexture(GL_TEXTURE_2D, id);
			if (target == GL_TEXTURE_2D)
				glTexSubImage2D(GL_TEXTURE_2D, 0, offsetX, offsetY, width, height,
					glTexFormatInfo.format, glTexFormatInfo.type, mem.data);
		}

		void updateTexture3D(int offsetX, int offsetY, int offsetZ, int width, int height, int depth, MemoryBuffer mem) {
			glBindTexture(GL_TEXTURE_3D, id);
			if (target == GL_TEXTURE_3D)
				glTexSubImage3D(GL_TEXTURE_3D, 0, offsetX, offsetY, offsetZ, width, height, depth,
					glTexFormatInfo.format, glTexFormatInfo.type, mem.data);
		}

		void updateTexture(int offsetX, int offsetY, int offsetZ, int width, int height, int depth, MemoryBuffer mem) {
			if (width > 0 && height > 0 && depth > 0) {
				updateTexture3D(offsetX, offsetY, offsetZ, width, height, depth, mem);
			}
			else if (width > 0 && height > 0) {
				updateTexture2D(offsetX, offsetY, width, height, mem);
			}
			else if (width > 0) {

			}
		}
	};

	class FrameBufferGL {
	public:
		GLuint id;
		int width, height;
		int attachmentsLength = 0;
		Attachment attachments[NE_MAX_FRAMEBUFFERS_ATTACHMENTS];

		void create(Attachment *attachments, int length);

		void create(int width, int height, TextureLayout texFormat, TextureLayout depthFormat) {
			glGenFramebuffers(1, &id);
			this->width = width;
			this->height = height;
		}

		void bind() {
			glBindFramebuffer(GL_FRAMEBUFFER, id);
		}

		void unbind() {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void clear(float r, float g, float b, float a) {
			glClearColor(r, g, b, a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}

		void clear(glm::vec4 color) {
			clear(color.x, color.y, color.z, color.a);
		}

		void clear() {
			clear(0, 0, 0, 0);
		}
	};

	static void setUniformGL(GLint uniformLoc, UniformType::Enum type, MemoryBuffer mem){
		switch (type) {
			case UniformType::Sampler:
				glUniform1i(uniformLoc, *(int*)(mem.data));
				break;
			case UniformType::Int:
				glUniform1i(uniformLoc, *(int*)(mem.data));
				break;
			case UniformType::Float:
				glUniform1f(uniformLoc, *(float*)(mem.data));
				break;
			case UniformType::Vec2:
				glUniform2f(uniformLoc, *(float*)(mem.data), *((float*)mem.data + 1));
				break;
			case UniformType::Vec3:
				glUniform3f(uniformLoc,  *(float*)(mem.data), *((float*)mem.data + 1), *((float*)mem.data+2));
				break;
			case UniformType::Vec4:
				glUniform4f(uniformLoc, *(float*)(mem.data), *((float*)mem.data + 1), *((float*)mem.data + 2), *((float*)mem.data + 3));
				break;
			case UniformType::Mat4:
				glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, (float*)mem.data);
				break;
		}
	}

	class RendererGL : public RendererInterface {
	public:
		VertexBufferGL vertexBuffers[NE_MAX_VERTEX_BUFFERS];
		IndexBufferGL indexBuffers[NE_MAX_INDEX_BUFFERS];
		TextureGL textures[NE_MAX_TEXTURES];
		FrameBufferGL frameBuffers[NE_MAX_FRAMEBUFFERS];
		ShaderGL shaders[NE_MAX_SHADERS];
		ProgramGL programs[NE_MAX_PROGRAMS];
		UniformGL uniforms[NE_MAX_UNIFORMS];
		GLuint vao = 0;
		Binding bindings[NE_MAX_STAGES];

		void init();

		void createIndexBuffer(IndexBufferHandler ibh, MemoryBuffer mem, VertexLayout vertexLayout) {
			IndexBufferGL ib;
			ib.create(mem);
			indexBuffers[ibh.id] = ib;
		}

		void createVertexBuffer(VertexBufferHandler vbh, MemoryBuffer mem, VertexLayout vertexLayout) {
			VertexBufferGL vb;
			vb.create(mem, vertexLayout);
			vertexBuffers[vbh.id] = vb;
		}

		//TODO map uh
		void createUniform(UniformHandler uh, const char* name, MemoryBuffer memBuffer, UniformType::Enum type, int flags) {
			UniformGL ugl;
			ugl.mem = memBuffer;
			ugl.type = type;
			ugl.name = name;
			uniforms[uh.id] = ugl;

			//uniforms.insert({ name, ugl });
		}

		void createTexture(TextureHandler texh, int width, int height, int depth, TextureLayout texFormat, MemoryBuffer memBuffer, int flags) {
			TextureGL tex;
			tex.createTexture(width, height, depth, texFormat, flags, memBuffer);
			textures[texh.id] = tex;
		}

		void createFrameBuffer(FrameBufferHandler fbh, int lengthAttach, Attachment *attachments) {
			FrameBufferGL fb;

			fb.create(attachments, lengthAttach);
			
			frameBuffers[fbh.id] = fb;
		}

		void createFrameBuffer(FrameBufferHandler fbh, int width, int height, TextureLayout texFormat, TextureLayout depthFormat) {
			FrameBufferGL fb;

			fb.create(width, height, texFormat, depthFormat);

			frameBuffers[fbh.id] = fb;
		}

		void createShader(ShaderHandler shaderh, std::string sourceCode, uint8_t shaderType) {
			ShaderGL shadergl;
			shadergl.compile(sourceCode, shaderType);
			shaders[shaderh.id] = shadergl;
		}

		void createProgram(ProgramHandler ph, ShaderHandler vertex, ShaderHandler fragment) {
			ProgramGL programgl;
			programgl.compile(shaders[vertex.id], shaders[fragment.id]);
			programs[ph.id] = programgl;
		}
		
		void updateUniform(UniformHandler uh, MemoryBuffer memBuffer) {
			uniforms[uh.id].mem = memBuffer;
			//GLint uniformLoc = glGetUniformLocation(program.id, name);
			//glUniform1f(uniformLoc, memBuffer.data);
		}

		void updateTexture(TextureHandler texHandler, int offsetX, int offsetY, int offsetZ, int width, int height, int depth, MemoryBuffer mem) {
			textures[texHandler.id].updateTexture(offsetX, offsetY, offsetZ, width, height, depth, mem);
		}

		void updateAllUniforms(GLint programID) {
			for (UniformGL u : uniforms) {
				GLint uniformLoc = glGetUniformLocation(programID, u.name.c_str());
				setUniformGL(uniformLoc, u.type, u.mem);
			}
		}

		void render(ProgramHandler program, RenderState *renderState) {
			ProgramGL currentProgram = programs[program.id];
			currentProgram.use();
			if (renderState->viewClear.clearFlags != NE_CLEAR_NONE) {
				ViewClear v = renderState->viewClear;
				glClearColor(v.r, v.g, v.b, v.a);
				GLbitfield clear = GL_NONE;
				if (v.clearFlags && NE_CLEAR_COLOR)
					clear |= GL_COLOR_BUFFER_BIT;
				if (v.clearFlags && NE_CLEAR_DEPTH)
					clear |= GL_DEPTH_BUFFER_BIT;
				if (v.clearFlags && NE_CLEAR_STENCIL)
					clear |= GL_STENCIL_BUFFER_BIT;
				glClear(clear);
			}

			if (renderState->stencilFlags != NE_STENCIL_DEFAULT) {
				glEnable(GL_STENCIL_TEST);
				GLint maskValue = (renderState->stencilFlags & NE_STENCIL_FUNC_MASK_MASK) >> NE_STENCIL_FUNC_MASK_SHIFT;

				GLenum sfail = GLstencilOp[filterFlag(renderState->stencilFlags, NE_STENCIL_OP_FAIL_S_MASK, NE_STENCIL_OP_FAIL_S_SHIFT)];
				GLenum dpfail = GLstencilOp[filterFlag(renderState->stencilFlags, NE_STENCIL_OP_FAIL_Z_MASK, NE_STENCIL_OP_FAIL_Z_SHIFT)];
				GLenum sdppass = GLstencilOp[filterFlag(renderState->stencilFlags, NE_STENCIL_OP_PASS_Z_MASK, NE_STENCIL_OP_PASS_Z_SHIFT)];

				GLenum test = GLstencilTest[filterFlag(renderState->stencilFlags, NE_STENCIL_TEST_MASK, NE_STENCIL_TEST_SHIFT)];

				glStencilOp(sfail, dpfail, sdppass);
				glStencilFunc(test, 1, 0xff); // all fragments should pass the stencil test
				glStencilMask(maskValue); // enable writing to the stencil buffer
			}

			while (!renderState->fbhClear.empty()) {
				FrameBufferClear fbhc = renderState->fbhClear.front();
				renderState->fbhClear.pop();

				frameBuffers[fbhc.fbh.id].bind();
				frameBuffers[fbhc.fbh.id].clear(fbhc.r, fbhc.g, fbhc.b, fbhc.a);
				frameBuffers[fbhc.fbh.id].unbind();
			}

			GLint glview[4];
			glGetIntegerv(GL_VIEWPORT, glview);
			if (renderState->fbh.id != INVALID_HANDLE) {
				frameBuffers[renderState->fbh.id].bind();
				glViewport(0, 0, frameBuffers[renderState->fbh.id].width, frameBuffers[renderState->fbh.id].height);
			}

			while (!renderState->uniforms.empty()) {
				UniformHandler uh = renderState->uniforms.front();
				renderState->uniforms.pop();

				std::string uniformName = uniforms[uh.id].name;
				GLint uniformLoc = glGetUniformLocation(currentProgram.id, uniformName.c_str());

				//TODO
				if (uniformLoc == -1)
					float error = 0;
				setUniformGL(uniformLoc, uniforms[uh.id].type, uniforms[uh.id].mem);
			}

			while (!renderState->bindings.empty()) {
				Binding b = renderState->bindings.front();
				int bindValue = *(int*)(uniforms[b.uniformID].mem.data);

				glActiveTexture(GL_TEXTURE0 + bindValue);
				glBindTexture(textures[b.texID].target, textures[b.texID].id);

				renderState->bindings.pop();
			}

			glLineWidth(4.0f);
			GLenum renderMode = GLrenderMode[filterFlag(renderState->stateFlags, NE_STATE_PRIMITIVE_TYPE_MASK, NE_STATE_PRIMITIVE_TYPE_SHIFT)];
			glBindVertexArray(vao);
			//TODO should unify models and vertexBuffers/ib into one render passage only.
			while (!renderState->vertexBuffers.empty()) {
				VertexBufferHandler vbh = renderState->vertexBuffers.front();
				IndexBufferHandler ibh = renderState->indexBuffers.front();
				renderState->vertexBuffers.pop();
				renderState->indexBuffers.pop();
			
				VertexBufferGL vb = vertexBuffers[vbh.id];
				IndexBufferGL ib = indexBuffers[ibh.id];
				glBindBuffer(GL_ARRAY_BUFFER, vb.id);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib.id);

				vb.bindAttributesBegin();
				vb.bindAttributes();
				vb.bindAttributesEnd();

				glDrawElements(renderMode, ib.size / 4, GL_UNSIGNED_INT, 0);

				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}

			while (!renderState->models.empty()) {
				ModelHandler model = renderState->models.front();
				renderState->models.pop();

				for (MeshHandler mesh : model.meshes) {
					VertexBufferGL vb = vertexBuffers[mesh.vertexBuffer.id];
					IndexBufferGL ib = indexBuffers[mesh.indexBuffer.id];
					glBindBuffer(GL_ARRAY_BUFFER, vb.id);
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib.id);

					vb.bindAttributesBegin();
					vb.bindAttributes();
					vb.bindAttributesEnd();

					glDrawElements(renderMode, ib.size / 4, GL_UNSIGNED_INT, 0);

					glBindBuffer(GL_ARRAY_BUFFER, 0);
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
					//glBindVertexArray(0);
				}
			}

			//TODO should either support only model or enforce mesh (the latter is better)
			while (!renderState->meshes.empty()) {
				MeshHandler mesh = renderState->meshes.front();
				renderState->meshes.pop();

				VertexBufferGL vb = vertexBuffers[mesh.vertexBuffer.id];
				IndexBufferGL ib = indexBuffers[mesh.indexBuffer.id];
				glBindBuffer(GL_ARRAY_BUFFER, vb.id);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib.id);

				vb.bindAttributesBegin();
				vb.bindAttributes();
				vb.bindAttributesEnd();

				glDrawElements(renderMode, ib.size / 4, GL_UNSIGNED_INT, 0);

				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}

			while (!renderState->toUnbinding.empty()) {
				Binding b = renderState->toUnbinding.front();
				int bindValue = *(int*)(uniforms[b.uniformID].mem.data);

				glActiveTexture(GL_TEXTURE0 + bindValue);
				glBindTexture(textures[b.texID].target, 0);

				renderState->toUnbinding.pop();
			}

			if (renderState->fbh.id != INVALID_HANDLE) {
				glViewport(glview[0], glview[1], glview[2], glview[3]);
				frameBuffers[renderState->fbh.id].unbind();
			}

			//Reset to default after draw call
			if (renderState->stencilFlags != NE_STENCIL_DEFAULT) {
				renderState->stencilFlags = NE_STENCIL_DEFAULT;
				glDisable(GL_STENCIL_TEST);
			}
		}
	};
};