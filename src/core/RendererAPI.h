#pragma once
#include <stdint.h>
#include "core/Settings.h"

/**
*	Texture sampling flags
*	Flags layout:
*		   4096 2048 1024  512  256  128   64   32   16    8    4    2   1
*						   MAG  MAG  MIN   MIN  W    W     V    V    U   U
*/
#define NE_TEX_SAMPLER_U_CLAMP			0x00000001
#define NE_TEX_SAMPLER_U_MIRROR			0x00000002
#define NE_TEX_SAMPLER_U_BORDER			0x00000003
#define NE_TEX_SAMPLER_U_SHIFT			0x00000000
#define NE_TEX_SAMPLER_U_MASK			0x00000003

#define NE_TEX_SAMPLER_V_CLAMP			0x00000004
#define NE_TEX_SAMPLER_V_MIRROR			0x00000008
#define NE_TEX_SAMPLER_V_BORDER			0x0000000c
#define NE_TEX_SAMPLER_V_SHIFT			0x00000002
#define NE_TEX_SAMPLER_V_MASK			0x0000000c

#define NE_TEX_SAMPLER_W_CLAMP			0x00000010
#define NE_TEX_SAMPLER_W_MIRROR			0x00000020
#define NE_TEX_SAMPLER_W_BORDER			0x00000030
#define NE_TEX_SAMPLER_W_SHIFT			0x00000004
#define NE_TEX_SAMPLER_W_MASK			0x00000030

#define NE_TEX_SAMPLER_MIN_LINEAR		0x00000040
#define NE_TEX_SAMPLER_MIN_NEAREST		0x00000080
#define NE_TEX_SAMPLER_MIN_SHIFT		0x00000006
#define NE_TEX_SAMPLER_MIN_MASK			0x000000c0

#define NE_TEX_SAMPLER_MAG_LINEAR		0x00000100
#define NE_TEX_SAMPLER_MAG_NEAREST		0x00000200
#define NE_TEX_SAMPLER_MAG_SHIFT		0x00000008
#define NE_TEX_SAMPLER_MAG_MASK			0x00000300

#define NE_TEX_SAMPLER_MIN_MAG_LINEAR	(NE_TEX_SAMPLER_MIN_LINEAR | NE_TEX_SAMPLER_MAG_LINEAR)
#define NE_TEX_SAMPLER_MIN_MAG_NEAREST	(NE_TEX_SAMPLER_MIN_NEAREST | NE_TEX_SAMPLER_MAG_NEAREST)

#define NE_TEX_SAMPLER_UVW_CLAMP		(NE_TEX_SAMPLER_U_CLAMP | NE_TEX_SAMPLER_V_CLAMP | NE_TEX_SAMPLER_W_CLAMP)
#define NE_TEX_SAMPLER_UVW_MIRROR		(NE_TEX_SAMPLER_U_MIRROR | NE_TEX_SAMPLER_V_MIRROR | NE_TEX_SAMPLER_W_MIRROR)
#define NE_TEX_SAMPLER_UVW_BORDER		(NE_TEX_SAMPLER_U_BORDER | NE_TEX_SAMPLER_V_BORDER | NE_TEX_SAMPLER_W_BORDER)

#define NE_SHADER_TYPE_VERTEX			0x0000001
#define NE_SHADER_TYPE_FRAGMENT			0x0000002
#define NE_SHADER_TYPE_GEOMETRY			0x0000003

/**
*	Stencil
*/
#define NE_STENCIL_FUNC_MASK_SHIFT		8

/**
*	Mask value vary from 0 to 255 (8 bits)
* */
#define NE_STENCIL_FUNC_MASK_MASK		0x0000ff00
#define NE_STENCIL_FUNC_MASK(v) ( ((uint32_t)(v) << NE_STENCIL_FUNC_MASK_SHIFT) & NE_STENCIL_FUNC_MASK_MASK)

#define NE_STENCIL_NONE					0x00000000
#define NE_STENCIL_MASK					0xffffffff
#define NE_STENCIL_DEFAULT				NE_STENCIL_NONE

#define NE_STENCIL_TEST_LESS			0x00010000
#define NE_STENCIL_TEST_LESS_EQUAL		0x00020000
#define NE_STENCIL_TEST_EQUAL			0x00030000
#define NE_STENCIL_TEST_GREATER_EQUAL	0x00040000
#define NE_STENCIL_TEST_GREATER			0x00050000
#define NE_STENCIL_TEST_NOTEQUAL		0x00060000
#define NE_STENCIL_TEST_NEVER			0x00070000
#define NE_STENCIL_TEST_ALWAYS			0x00080000
#define NE_STENCIL_TEST_SHIFT			16        
#define NE_STENCIL_TEST_MASK			0x000f0000

#define NE_STENCIL_OP_FAIL_S_ZERO		0x00100000
#define NE_STENCIL_OP_FAIL_S_KEEP		0x00200000
#define NE_STENCIL_OP_FAIL_S_REPLACE	0x00300000
#define NE_STENCIL_OP_FAIL_S_INCR		0x00400000
#define NE_STENCIL_OP_FAIL_S_INCR_CLAMP	0x00500000
#define NE_STENCIL_OP_FAIL_S_DECR		0x00600000
#define NE_STENCIL_OP_FAIL_S_DECR_CLAMP	0x00700000
#define NE_STENCIL_OP_FAIL_S_INVERT		0x00800000
#define NE_STENCIL_OP_FAIL_S_SHIFT		20         
#define NE_STENCIL_OP_FAIL_S_MASK		0x00f00000
		
#define NE_STENCIL_OP_FAIL_Z_ZERO		0x01000000
#define NE_STENCIL_OP_FAIL_Z_KEEP		0x02000000
#define NE_STENCIL_OP_FAIL_Z_REPLACE	0x03000000
#define NE_STENCIL_OP_FAIL_Z_INCR		0x04000000
#define NE_STENCIL_OP_FAIL_Z_INCR_CLAMP	0x05000000
#define NE_STENCIL_OP_FAIL_Z_DECR		0x06000000
#define NE_STENCIL_OP_FAIL_Z_DECR_CLAMP	0x07000000
#define NE_STENCIL_OP_FAIL_Z_INVERT		0x08000000
#define NE_STENCIL_OP_FAIL_Z_SHIFT		24        
#define NE_STENCIL_OP_FAIL_Z_MASK		0x0f000000
		
#define NE_STENCIL_OP_PASS_Z_ZERO		0x10000000
#define NE_STENCIL_OP_PASS_Z_KEEP		0x20000000
#define NE_STENCIL_OP_PASS_Z_REPLACE	0x30000000
#define NE_STENCIL_OP_PASS_Z_INCR		0x40000000
#define NE_STENCIL_OP_PASS_Z_INCR_CLAMP	0x50000000
#define NE_STENCIL_OP_PASS_Z_DECR		0x60000000
#define NE_STENCIL_OP_PASS_Z_DECR_CLAMP	0x70000000
#define NE_STENCIL_OP_PASS_Z_INVERT		0x80000000
#define NE_STENCIL_OP_PASS_Z_SHIFT		28                   
#define NE_STENCIL_OP_PASS_Z_MASK		0xf0000000 

/**
*	Clear
*/
#define NE_CLEAR_NONE                 0x0000
#define NE_CLEAR_COLOR                0x0001
#define NE_CLEAR_DEPTH                0x0002
#define NE_CLEAR_STENCIL              0x0004
#define NE_CLEAR_ALL	              NE_CLEAR_COLOR | NE_CLEAR_DEPTH | NE_CLEAR_STENCIL

/**
*	Primitive type
*/
#define NE_STATE_PRIMITIVE_TYPE_TRI		      0x0001000000000000 //Triangle Strip
#define NE_STATE_PRIMITIVE_TYPE_TRISTRIP      0x0002000000000000 //Triangle Strip
#define NE_STATE_PRIMITIVE_TYPE_LINES         0x0003000000000000
#define NE_STATE_PRIMITIVE_TYPE_LINESTRIP     0x0004000000000000
#define NE_STATE_PRIMITIVE_TYPE_POINTS        0x0005000000000000
#define NE_STATE_PRIMITIVE_TYPE_SHIFT         48                 
#define NE_STATE_PRIMITIVE_TYPE_MASK          0x0007000000000000 

#define NE_STATE_DEFAULT NE_STATE_PRIMITIVE_TYPE_TRI

#define INVALID_HANDLE 2048
#define NE_MAX_TEXTURES 2048
#define NE_MAX_FRAMEBUFFERS 8
#define NE_MAX_FRAMEBUFFERS_ATTACHMENTS 2
#define NE_MAX_VERTEX_BUFFERS 2048
#define NE_MAX_INDEX_BUFFERS 2048
#define NE_MAX_UNIFORMS 2048
#define NE_MAX_SHADERS 32
#define NE_MAX_PROGRAMS 32
#define NE_MAX_MESHS 1024
#define NE_MAX_MATERIALS 1024
#define NE_MAX_MODELS 1024
#define NE_MAX_STAGES	8

namespace narvalengine {
	typedef uint16_t HandleID;

	enum ImageFileFormat {
		PNG,
		EXR
	};

	/**
	 *	Defines the interpretation given for each Vertex Attribute.
	 */
	struct VertexAttrib{
		enum Enum {
			Position,
			Normal,
			Tangent,
			Color0,
			TexCoord0,
			Slot0,
			Slot1,
			Slot2,
			Slot3,
			Count
		};
	};

	struct VertexAttribType {
		enum Enum {
			Float,
			Int,
			Count
		};
	};

	struct UniformType {
		enum Enum {
			Sampler,
			Mat4,
			Vec2,
			Vec3,
			Vec4,
			Float,
			Int,
			Bool,
			Count
		};
	};

	enum TextureLayout {
		R32I,
		RG32I,
		RGB32I,
		RGBA32I,
		R32F,
		RG32F,
		RGB32F,
		RGBA32F,
		R8,
		RG8,
		RGB8,
		RGBA8,

		D24,
		D24S8,

		Count
	};	

	/**
	 * Checks if a handle is valid.
	 * 
	 * @param handleId
	 * @return true if valid. False otherwise.
	 */
	static bool isHandleValid(HandleID handleId) {
		if (handleId < 0 || handleId >= INVALID_HANDLE)
			return false;
		else
			return true;
	}

	static bool isDepth(TextureLayout tl) {
		if (tl == TextureLayout::D24 || tl == TextureLayout::D24S8)
			return true;
		else 
			return false;
	}

	enum TextureName {
		ALBEDO = 1 << 1,
		ROUGHNESS = 1 << 2,
		METALLIC = 1 << 3,
		EMISSION = 1 << 4,
		NORMAL_MAP = 1 << 5,
		AO = 1 << 6,
		TEX_1 = 1 << 7,
		TEX_2 = 1 << 8,
		TEX_3 = 1 << 9,
		TextureNameCount = 9
	};

	enum TextureChannelFormat {
		RGB_ALBEDO,
		RGB_ALBEDO_A_ROUGHNESS,
		RGB_ALBEDO_A_METALLIC,
		R_METALLIC
	};

	static const uint8_t vertexAttribTypeSizeCPU[VertexAttribType::Count][4] = {
		{sizeof(float), sizeof(float)*2, sizeof(float)*3, sizeof(float)*4} // Float
	};

	static const uint8_t vertexAttribTypeSizeGL[VertexAttribType::Count][4] = {
		{4, 8, 12, 16} // Float
	};

	static const uint8_t (*vertexAttribTypeSize[])[VertexAttribType::Count][4] = {
		&vertexAttribTypeSizeGL,
		& vertexAttribTypeSizeCPU
	};

	/**
	* Struct responsible for configuring the memory layout of a Vertex Buffer.
	* 
	*  Example of a vertex packed with Position, composed of 3 floats, and UV coordinates, composed of 2 floats.
	*  Assuming floats of 4 bytes.
	* 
	*   Total Stride: 20 bytes
	*   |-----------------|
	*	Offsets:
	*	0 bytes		12 bytes
	*	|---------| |-----|
	*   Size in bytes:
	*   12 bytes	8 bytes
	*   |---------| |-----|
	*	1.0	0.0	1.0	0.0 0.0 ...
	*/
	struct VertexLayout {
		/**
		*	Stride size defined in bytes.
		*/
		int stride = 0;
		/**
		*	Offset for each VertexAttrib type. Also defined in bytes.
		*/
		int offset[VertexAttrib::Count]{};
		/**
		*	Name of each attribute stored.
		*/
		int attribute[VertexAttrib::Count]{};
		/**
		*	Type of each attribute stored.
		*/
		VertexAttribType::Enum attributeType[VertexAttrib::Count]{};
		/** 
		*	Number of elements for this VertexAttrib with VertexAttribType per each element.
		*/
		int qtt[VertexAttrib::Count]{};
		/**
		 *	Counter of how many attributes are active.
		 */
		int activeAttribs = 0;

		void init() {
			stride = 0;
			activeAttribs = 0;
			for (int i = 0; i < VertexAttrib::Count; i++) {
				offset[i] = 0;
				attribute[i] = 0;
				qtt[i] = 0;
			}
		}

		/**
		 * Adds a vertex attribute name with type and n elements and calculates the stride and offset.
		 * 
		 * @param vertAttrib the vertex attribute name.
		 * @param vertAttribType the vertex attribute type.
		 * @param nElements number of elements of type VertexAttribType with semantic meaning of VertexAttrib.
		 */
		void add(VertexAttrib::Enum vertAttrib, VertexAttribType::Enum vertAttribType, int nElements) {
			qtt[vertAttrib] = nElements;
			offset[vertAttrib] = stride;
			attributeType[vertAttrib] = vertAttribType;
			attribute[vertAttrib] = vertAttrib;
			stride += (*vertexAttribTypeSize[0])[vertAttribType][nElements -1];
			activeAttribs++;
		}

		/**
		 * Verifies if this VertexLayout contains vertAttrib.
		 * 
		 * @param vertAttrib to test.
		 * @return true if this VertexLayout contains vertAttrib. False otherwise.
		 */
		bool contains(VertexAttrib::Enum vertAttrib) {
			if (qtt[vertAttrib] > 0)
				return true;
			else
				return false;
		}

		/**
		 * Returns the size in bytes of this Vertex Attribute Type.
		 * 
		 * @param vertAttrib
		 * @return the size in bytes.
		 */
		uint8_t getVertexAttribTypeSize(VertexAttrib::Enum vertAttrib, RendererAPIName apiName) {
			return (*vertexAttribTypeSize[apiName])[attributeType[vertAttrib]][0];
		}

		void end() {

		}
	};
}