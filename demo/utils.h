#pragma once

#include <cstdio>
#include <cstdlib>

#include "memory/allocator.h"

inline char* LoadFile(const char* filepath) {
	FILE *f = fopen(filepath, "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

	char *string = (char*) malloc(fsize + 1);
	fread(string, 1, fsize, f);
	fclose(f);

	string[fsize] = 0;

	return string;
}

inline char* LoadFile(const char* filepath, soul::memory::Allocator* allocator) {
	FILE* f = fopen(filepath, "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

	char* string = (char*)allocator->allocate(fsize + 1, alignof(char));
	fread(string, 1, fsize, f);
	fclose(f);

	string[fsize] = 0;

	return string;
}

// Returns the max number of levels for a texture of given dimensions
static inline uint8 MaxLevelCount(uint32 width, uint32 height) noexcept {
	return std::max(1, std::ilogbf(std::max(width, height)) + 1);
}

