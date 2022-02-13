#pragma once
#include <glad/glad.h>
#include "core/RendererAPI.h"
#include "utils/Utils.h"
#include <glm/glm.hpp>
#include "stb_image_write.h"
#include "tinyexr.h"
#include "utils/Math.h"
#include <glog/logging.h>
#include <magic_enum.hpp>
#include "utils/StringID.h"

namespace narvalengine {
	/**
	 * Returns the number of channels in a given {@code TextureLayout}.
	 * 
	 * @param texFormat.
	 * @return number of channels in {@param texFormat}. Returns 0 if the current {@code TextureLayout} has not been mapped yet.
	 */
	inline static uint8_t getNumberOfChannels(TextureLayout texFormat) {
		if (texFormat == R32I || texFormat == R32F || texFormat == R8) {
			return 1;
		} else if (texFormat == RG32I || texFormat == RG32F || texFormat == RG8) {
			return 2;
		} else if (texFormat == RGB32I || texFormat == RGB32F || texFormat == RGB8) {
			return 3;
		}else if (texFormat == RGBA32I || texFormat == RGBA32F || texFormat == RGBA8) {
			return 4;
		}else {
			return 0;
		}
	};

	/**
	 * Saves the image at {@code mem} to {@code path}. Currently supports EXR and PNG file formats.
	 * 
	 * @param mem
	 * @param width
	 * @param height
	 * @param format
	 * @param fileFormat
	 * @param path
	 */
	inline void saveImage(MemoryBuffer mem, int width, int height, TextureLayout format, ImageFileFormat fileFormat, const char* path) {
		int channels = getNumberOfChannels(format);
		const char* err = nullptr;

		if (fileFormat == ImageFileFormat::PNG) {
			if (format == TextureLayout::RGB32F) {
				int sizeBytes = width * height * channels * sizeof(char);
				MemoryBuffer converted = { new uint8_t[sizeBytes], sizeBytes };
				float* startPoint = (float*)mem.data;

				for (int i = 0; i < width * height * channels; i++) {
					float v = startPoint[i];
					v = glm::clamp(v, 0.0f, 1.0f);
					((uint8_t*)(converted.data))[i] = (uint8_t)(v * 255);
				}

				stbi_write_png(path, width, height, channels, converted.data, width * channels);
				memBufferFree(&converted);
			}else 
				DLOG(WARNING) << "The TextureLayout " << magic_enum::enum_name(format) << " has no mapping for the ImageFileFormat " << magic_enum::enum_name(fileFormat) << ". No Operation was performed when trying to save the image " << std::string(path) << ".";
			
		}else if (fileFormat == ImageFileFormat::EXR) {
			int r = SaveEXR((float*)mem.data, width, height, channels, 0, path, &err);
			if (r != TINYEXR_SUCCESS) {
				if (err) {
					LOG(ERROR) << std::string(err);
					FreeEXRErrorMessage(err);
				}
			}
		}else 
			DLOG(WARNING) << "The ImageFileFormat " << magic_enum::enum_name(fileFormat) << " has no mapping. No Operation was performed when trying to save the image " << std::string(path) << ".";
	}

	class Texture {
	public:
		uint32_t width = 0, height = 0, depth = 0;
		/**
		 * nPixels = width * height * depth.
		 */
		int nPixels = 0;
		TextureLayout texFormat;
		TextureName textureName;
		TextureChannelFormat textureChannelFormat;
		int samplerFlags;
		MemoryBuffer mem;
		StringID resourceID = NE_INVALID_STRING_ID;

		Texture();
		Texture(int width, int height, TextureLayout texFormat, int samplerFlags, MemoryBuffer memBuffer);
		Texture(int width, int height, int depth, TextureLayout texFormat, int samplerFlags, MemoryBuffer memBuffer);
		glm::ivec3 getResolution();

		/**
		 * Wraps the texture coordinates {@code u}, {@code v} and {@code w} according to the texture sampling flags set in this {@code samplerFlags}.
		 * 
		 * @param u
		 * @param v
		 * @param w
		 */
		void wrapTextureCoordinates(float &u, float &v, float &w);

		/**
		 * Samples directly at the {@code index}.
		 * 
		 * @param index in the range [0, {@code width*height*depth}].
		 * @return sampled color;
		 */
		glm::vec4 sampleAtIndex(uint32_t index);
		
		/**
		 * Samples a pixel from this texture using the UVW texture coordinates {@code u}, {@code v} and {@code w}.
		 * 
		 * @param u.
		 * @param v.
		 * @param w.
		 * @return sampled color.
		 */
		glm::vec4 sample(float u, float v = 0, float w = 0);
	};
}