#pragma once

#include "runtime/runtime.h"

inline char* LoadFile(const char* filepath) {
	FILE* f = fopen(filepath, "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

	char* string = (char*)Soul::Runtime::Allocate(fsize + 1, alignof(char));
	fread(string, 1, fsize, f);
	fclose(f);

	string[fsize] = 0;

	return string;
}