#pragma once

#include "render/data.h"
#include "core/debug.h"
#include "extern/glad.h"
#include "core/array.h"
#include "render/intern/asset.h"

#include <cstdio>

namespace Soul {
	namespace Render {
		namespace GLExt {

			static char* _LoadFile(const char* filepath) {
				FILE *f = fopen(filepath, "rb");
				fseek(f, 0, SEEK_END);
				long fsize = ftell(f);
				fseek(f, 0, SEEK_SET);

				char *string = (char*)malloc(fsize + 1);
				fread(string, fsize, 1, f);
				fclose(f);

				string[fsize] = 0;
				return string;
			}

			bool IsErrorCheckPass();
			void ErrorCheck(const char* message);

			GLuint ProgramCreate(const char* shaderFile);
			void ProgramDelete(GLuint* programHandle);

			void TextureDelete(GLuint* textureHandle);

			void FramebufferDelete(GLuint* framebufferHandle);

			void UBOBind(GLuint shader, const char* uboName, const int bindPoint);

		}
	}
}