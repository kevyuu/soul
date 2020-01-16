#include "render/intern/asset.h"
#include "render/data.h"
#include "render/intern/glext.h"
#include "core/dev_util.h"

namespace Soul {
	namespace Render {

		void ShadowMapRP::init(Database &database) {

			SOUL_ASSERT(0, GLExt::IsErrorCheckPass());
			program = GLExt::ProgramCreate(RenderAsset::ShaderFile::shadowMap);

			modelLoc = glGetUniformLocation(program, "model");
			shadowMatrixLoc = glGetUniformLocation(program, "shadowMatrix");

			SOUL_ASSERT(0, GLExt::IsErrorCheckPass());

		}

		static void _calculateViewport(const ShadowAtlas& shadowAtlas, const ShadowKey& shadowKey, int* viewportLeft, int* viewportBottom, int* viewportWidth)
		{
			int quadrant = shadowKey.quadrant;
			int subdiv = shadowKey.subdiv;
			int subdivCount = shadowAtlas.subdivSqrtCount[quadrant] * shadowAtlas.subdivSqrtCount[quadrant];
			int atlasReso = shadowAtlas.resolution;
			int subdivReso = atlasReso / (2 * shadowAtlas.subdivSqrtCount[quadrant]);
			int xSubdiv = subdiv % shadowAtlas.subdivSqrtCount[quadrant];
			int ySubdiv = subdiv / shadowAtlas.subdivSqrtCount[quadrant];

			*viewportBottom = (quadrant / 2) * 0.5f * atlasReso + ySubdiv * subdivReso;
			*viewportLeft = (quadrant % 2) * 0.5f * atlasReso + xSubdiv * subdivReso;
			*viewportWidth = subdivReso;
		}

		void ShadowMapRP::execute(Database &database) {
			SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

			int resolution = database.shadowAtlas.resolution;

			glViewport(0, 0, resolution, resolution);
			glBindFramebuffer(GL_FRAMEBUFFER, database.shadowAtlas.framebuffer);
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glDisable(GL_DITHER);
			glDepthMask(GL_TRUE);
			glColorMask(0, 0, 0, 0);
			glClearDepth(1.0f);
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glEnable(GL_SCISSOR_TEST);

			glUseProgram(program);

			for (int i = 0; i < database.dirLightCount; i++) {
				const DirLight& light = database.dirLights[i];
				int quadrant = light.shadowKey.quadrant;
				int subdiv = light.shadowKey.subdiv;
				int subdivCount = database.shadowAtlas.subdivSqrtCount[quadrant] * database.shadowAtlas.subdivSqrtCount[quadrant];
				int atlasReso = database.shadowAtlas.resolution;
				int subdivReso = atlasReso / (2 * database.shadowAtlas.subdivSqrtCount[quadrant]);
				int xSubdiv = subdiv % database.shadowAtlas.subdivSqrtCount[quadrant];
				int ySubdiv = subdiv / database.shadowAtlas.subdivSqrtCount[quadrant];

				float bottomSubdivViewport = (quadrant / 2) * 0.5f * atlasReso + ySubdiv * subdivReso;
				float leftSubdivViewport = (quadrant % 2) * 0.5f * atlasReso + xSubdiv * subdivReso;

				for (int j = 0; j < 4; j++) {

					int splitReso = subdivReso / 2;
					int xSplit = j % 2;
					int ySplit = j / 2;

					float bottomSplitViewport = bottomSubdivViewport + (ySplit * splitReso);
					float leftSplitViewport = leftSubdivViewport + (xSplit * splitReso);
					glUniformMatrix4fv(shadowMatrixLoc, 1, GL_TRUE, (const GLfloat*)light.shadowMatrixes[j].elem);
					glScissor(leftSplitViewport, bottomSplitViewport, splitReso, splitReso);

					for (int k = 0; k < database.meshBuffer.size(); k++) {
						const Mesh& mesh = database.meshBuffer.get(k);
						glUniformMatrix4fv(modelLoc, 1, GL_TRUE, (const GLfloat*)mesh.transform.elem);	
						glBindVertexArray(mesh.vaoHandle);
						glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
					}
				}
			}

			for (auto iterator = database.pointLights.begin(); iterator != database.pointLights.end(); iterator.next())
			{
				const PointLight& light = database.pointLights.iter(iterator);
				for (int i = 0; i < 6; i++)
				{
					int viewportBottom, viewportLeft, viewportWidth;
					_calculateViewport(database.shadowAtlas, light.shadowKeys[i], &viewportLeft, &viewportBottom, &viewportWidth);
					for (int j = 0; j < database.meshBuffer.size(); j++)
					{
						const Mesh& mesh = database.meshBuffer[j];
						glUniformMatrix4fv(modelLoc, 1, GL_TRUE, (const GLfloat*)mesh.transform.elem);
						glUniformMatrix4fv(shadowMatrixLoc, 1, GL_TRUE, (const GLfloat*)light.shadowMatrixes[i].elem);
						glBindVertexArray(mesh.vaoHandle);
						glScissor(viewportLeft, viewportBottom, viewportWidth, viewportWidth);
						glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
					}
				}

			}

			for (auto iterator = database.spotLights.begin(); iterator != database.spotLights.end(); iterator.next())
			{
				const SpotLight& light = database.spotLights.iter(iterator);
				int viewportBottom, viewportLeft, viewportWidth;
				_calculateViewport(database.shadowAtlas, light.shadowKey, &viewportLeft, &viewportBottom, &viewportWidth);

				for (int j = 0; j < database.meshBuffer.size(); j++)
				{
					const Mesh& mesh = database.meshBuffer[j];
					glUniformMatrix4fv(modelLoc, 1, GL_TRUE, (const GLfloat*)mesh.transform.elem);
					glUniformMatrix4fv(shadowMatrixLoc, 1, GL_TRUE, (const GLfloat*)light.shadowMatrix.elem);
					glBindVertexArray(mesh.vaoHandle);
					glScissor(viewportLeft, viewportBottom, viewportWidth, viewportWidth);
					glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
				}
			}

			glDisable(GL_SCISSOR_TEST);
			glBindVertexArray(0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glUseProgram(0);

			GLExt::ErrorCheck("ShadowMapRP::execute");

			SOUL_PROFILE_RANGE_POP();
		}

		void ShadowMapRP::shutdown(Database &database) {
			glDeleteProgram(program);
		}

	}
}