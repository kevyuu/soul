#pragma once

#ifndef SOUL_RENDER_INTERN_UTIL_H
#define SOUL_RENDER_INTERN_UTIL_H

#include "render/type.h"
#include "core/debug.h"
#include "extern/glad.h"

#include <cstdio>

namespace Soul {
    namespace RenderUtil{

        static char* LoadFile(const char* filepath) {
            FILE *f = fopen(filepath, "rb");
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);

            char *string = (char*) malloc(fsize + 1);
            fread(string, fsize, 1, f);
            fclose(f);

            string[fsize] = 0;
            return string;
        }

		inline bool GLIsErrorCheckPass() {
			bool isError = false;
			GLenum err;
			while ((err = glGetError()) != GL_NO_ERROR) {
				SOUL_LOG(SOUL_LOG_VERBOSE_ERROR, "OpenGL Error | Error Code : %d", err);
				isError = true;
			}

			return !isError;
		}

		inline void GLErrorCheck(const char* message) {
			GLenum err;
			while ((err = glGetError()) != GL_NO_ERROR) {
				SOUL_ASSERT(0, err == GL_NO_ERROR, "OpenGL error | error_code = %d", err);
			}
		}

        inline GLuint GLProgramCreate(const char* shaderFile) {

            char* shaderCode = nullptr;
            if (shaderFile) shaderCode = LoadFile(shaderFile);

            int success = 1;
            char infoLog[512];

			GLuint shaderHandle = glCreateProgram();
			bool isComputeShader = strstr(shaderCode, "#ifdef COMPUTE_SHADER");

			if (isComputeShader) {

				const char* computePrefix = "#version 450 core\n#define COMPUTE_SHADER\n\0";
				const char* computeCode[] = { computePrefix, shaderCode };
				const GLint computeCodeLength[] = { (GLint)strlen(computePrefix), (GLint)strlen(shaderCode) };
				
				GLuint computeHandle = 0;
				computeHandle = glCreateShader(GL_COMPUTE_SHADER);
				glShaderSource(computeHandle, 2, computeCode, computeCodeLength);
				glCompileShader(computeHandle);
				glGetShaderiv(computeHandle, GL_COMPILE_STATUS, &success);
				glGetShaderInfoLog(computeHandle, 512, NULL, infoLog);
				SOUL_ASSERT(0, success, "Compute shader compilation failed| info = %s", infoLog);

				shaderHandle = glCreateProgram();
				glAttachShader(shaderHandle, computeHandle);
				glLinkProgram(shaderHandle);
				glGetProgramiv(shaderHandle, GL_LINK_STATUS, &success);
				glGetProgramInfoLog(shaderHandle, 512, NULL, infoLog);
				SOUL_ASSERT(0, success, "Program linking failed| info = %s", infoLog);

				glDeleteShader(computeHandle);

			}
			else {
				GLuint vertexHandle;
				vertexHandle = glCreateShader(GL_VERTEX_SHADER);
				const char* vertexPrefix = "#version 450 core\n#define VERTEX_SHADER\n\0";
				const char* vertexCode[] = { vertexPrefix, shaderCode };
				const GLint vertexCodeLength[] = { (GLint)strlen(vertexPrefix), (GLint)strlen(shaderCode) };
				glShaderSource(vertexHandle, 2, vertexCode, vertexCodeLength);
				glCompileShader(vertexHandle);
				glGetShaderiv(vertexHandle, GL_COMPILE_STATUS, &success);
				glGetShaderInfoLog(vertexHandle, 512, NULL, infoLog);
				SOUL_ASSERT(0, success, "Vertex shader compilation failed| info = %s", infoLog);

				bool isGeometryShaderExist = strstr(shaderCode, "#ifdef GEOMETRY_SHADER");
				GLuint geometryHandle = 0;
				if (isGeometryShaderExist) {
					const char* geometryPrefix = "#version 450 core\n#define GEOMETRY_SHADER\n\0";
					const char* geometryCode[] = { geometryPrefix, shaderCode };
					const GLint geometryCodeLength[] = { (GLint)strlen(geometryPrefix), (GLint)strlen(shaderCode) };
					geometryHandle = glCreateShader(GL_GEOMETRY_SHADER);
					glShaderSource(geometryHandle, 2, geometryCode, geometryCodeLength);
					glCompileShader(geometryHandle);
					glGetShaderiv(geometryHandle, GL_COMPILE_STATUS, &success);
					glGetShaderInfoLog(geometryHandle, 512, NULL, infoLog);
					SOUL_ASSERT(0, success, "Geometry shader compilation failed| info = %s", infoLog);
				}

				GLuint fragmentHandle;
				fragmentHandle = glCreateShader(GL_FRAGMENT_SHADER);
				const char* fragmentPrefix = "#version 450 core\n#define FRAGMENT_SHADER\n\0";
				const char* fragmentCode[] = { fragmentPrefix, shaderCode };
				const GLint fragmentCodeLength[] = { (GLint)strlen(fragmentPrefix), (GLint)strlen(shaderCode) };
				glShaderSource(fragmentHandle, 2, fragmentCode, fragmentCodeLength);
				glCompileShader(fragmentHandle);
				glGetShaderiv(fragmentHandle, GL_COMPILE_STATUS, &success);
				glGetShaderInfoLog(fragmentHandle, 512, NULL, infoLog);
				SOUL_ASSERT(0, success, "Fragment shader compilation failed| info = %s", infoLog);
				
				glAttachShader(shaderHandle, vertexHandle);
				if (isGeometryShaderExist) glAttachShader(shaderHandle, geometryHandle);
				glAttachShader(shaderHandle, fragmentHandle);
				glLinkProgram(shaderHandle);
				glGetProgramiv(shaderHandle, GL_LINK_STATUS, &success);
				glGetProgramInfoLog(shaderHandle, 512, NULL, infoLog);
				SOUL_ASSERT(0, success, "Program linking failed| info = %s", infoLog);

				glDeleteShader(vertexHandle);
				if (isGeometryShaderExist) glDeleteShader(geometryHandle);
				glDeleteShader(fragmentHandle);
			}

            free(shaderCode);
			SOUL_ASSERT(0, RenderUtil::GLIsErrorCheckPass(), "GL Program Creation Failed");
            return shaderHandle;

        }

    }
}

#endif