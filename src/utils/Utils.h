#pragma once
#include <stdint.h> 
#include <stdio.h>
#include <cstring>
#include "utils/MurmurHash3.h"

namespace narvalengine {
	/**
	 * Struct responsible for allocating handles. Used for managing renderer resources.
	 * The memory layout is defined as:
	 * Struct			| Dense					| Sparse
	 * HandleAllocator	| 0 ... nHandles		| 0 ... nHandles
	 * 
	 * Where:
	 * Dense contains actual handles in use.
	 * Sparse contains indices for handles in use.
	 */
	struct HandleAllocator {

		HandleAllocator() {};

		HandleAllocator(uint16_t maxHandles) {
			this->maxHandles = maxHandles;
			reset();
		}

		void reset() {
			uint16_t* dense = getDensePtr();
			for (uint16_t i = 0; i < maxHandles; i++)
				dense[i] = i;
		}

		uint16_t alloc() {
			if (nHandles < maxHandles) {
				uint16_t* dense = getDensePtr();
				uint16_t* sparse = getSparsePtr();

				uint16_t  handle = dense[nHandles];
				sparse[handle] = nHandles;

				nHandles++;
				return handle;
			}

			return INVALID_HANDLE;
		}

		void free(uint16_t handle) {
			uint16_t* dense = getDensePtr();
			uint16_t* sparse = getSparsePtr();

			uint16_t index = sparse[handle];
			nHandles--;

			uint16_t temp = dense[nHandles];
			dense[nHandles] = handle;
			sparse[temp] = index;
			dense[index] = temp;
		}

		bool isValid(uint16_t handle) {
			uint16_t* dense = getDensePtr();
			uint16_t* sparse = getSparsePtr();

			uint16_t  index = sparse[handle];

			return index < nHandles&& dense[index] == handle;
		}

		uint16_t getHandleAt(uint16_t handle) {
			return getDensePtr()[handle];
		}

		uint16_t* getDensePtr() {
			uint8_t* ptr = (uint8_t*)this + sizeof(this);
			return (uint16_t*)ptr;
		}

		uint16_t* getSparsePtr() {
			return &getDensePtr()[maxHandles];
		}

		~HandleAllocator() {
			delete getDensePtr();
		}

		uint16_t nHandles = 0;
		uint16_t maxHandles;
	};

	struct MemoryBuffer {
		void* data = nullptr;
		uint32_t size = 0;
	};

	/**
	 * Creates a Handle Allocator at the specified {@code mem} address.
	 * 
	 * @param mem in which to allocate this HandleAllocator.
	 * @param maxHandles maximum capacity of handlers permitted by this allocator.
	 * @return pointer to this allocator.
	 */
	static HandleAllocator* createHandleAllocator(uint8_t* mem, uint16_t maxHandles) {
		 return new (mem) HandleAllocator(maxHandles);
	}

	/**
	 * Creates a MemoryBuffer with size N in bytes.
	 * 
	 * @param size in bytes.
	 * @return allocated memor
	 */
	static uint8_t* memAlloc(uint32_t size) {
		return new uint8_t[size];
	}

	/**
	 * Creates a MemoryBuffer with size n in bytes.
	 * 
	 * @param size in bytes.
	 * @return memoryBuffer created.
	 */
	static MemoryBuffer* memBufferAlloc(uint32_t size) {
		return new MemoryBuffer{new uint8_t[size], size};
	}

	/**
	 * Frees {@code mem.data}
	 * 
	 * @param mem
	 */
	static void memBufferFree(MemoryBuffer *mem) {
		mem->size = -1;
		free(mem->data);
	}

	/**
	 * Copies to {@code destination.data} from {@code source} with size in bytes.
	 *
	 * @param destination destination data.
	 * @param source data.
	 * @param size of {@code source} in bytes.
	 */
	template <typename T>
	static void memCopy(MemoryBuffer* destination, T *source, int size){
		//assert(size < destination->size / sizeof(T));
		memcpy(destination->data, source, size);
	}
};