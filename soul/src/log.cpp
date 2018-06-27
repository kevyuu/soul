#include "core/log.h"
#include <iostream>

namespace Soul {

	void LogInit() {

	}

	void Log(const char* file, const char* line, const char* message) {
		std::cout << file << "::" << line << "::" << message << std::endl;
	}
}