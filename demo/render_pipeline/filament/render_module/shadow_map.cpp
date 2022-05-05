#include "shadow_map.h"
#include "../renderer.h"
#include "runtime/scope_allocator.h"

namespace soul_fila
{

	static constexpr bool USE_DEPTH_CLAMP = false;

	static float TexelSizeWorldSpace(const Mat3f& worldToShadowTexture, uint16 shadowDimension) noexcept {
		// The Jacobian of the transformation from texture-to-world is the matrix itself for
		// orthographic projections. We just need to inverse worldToShadowTexture,
		// which is guaranteed to be orthographic.
		// The two first columns give us the how a texel maps in world-space.
		const float ures = 1.0f / float(shadowDimension);
		const float vres = 1.0f / float(shadowDimension);
		const Mat3f shadowTextureToWorld = mat3Inverse(worldToShadowTexture);
		const Vec3f Jx = shadowTextureToWorld.columns(0);
		const Vec3f Jy = shadowTextureToWorld.columns(1);
		const float s = std::max(length(Jx) * ures, length(Jy) * vres);
		return s;
	}

	static soul::Mat4f ComputeVSMLightViewMatrix(const soul::Mat4f& lightSpacePcf,
		const soul::Mat4f& Mv, float zfar) noexcept {
		// The lightSpacePcf matrix transforms coordinates from world space into (u, v, z) coordinates,
		// where (u, v) are used to access the shadow map, and z is the (non linear) PCF comparison
		// value [0, 1].
		//
		// For VSM, we want to leave the z coordinate in linear light space, normalized between [0, 1]
		// (the normalization factor is therefore -1/zfar).
		//
		// When sampling a VSM shadow map, the shader follows suit, and doesn't divide by w for the z
		// coordinate. See getters.fs.
		soul::Mat4f lightSpaceVsm(lightSpacePcf);
		lightSpaceVsm.rows[2] = Mv.rows[2] * (-1.0f / zfar);
		return lightSpaceVsm;
	}

	struct FrustumVertices
	{
		Vec3f vertices[8];
	};

	static FrustumVertices ComputeCameraFrustumVertices(const CameraInfo& cameraInfo, Vec2f csNearFar) noexcept
	{
		FrustumVertices result;
		Mat4f wsMat = cameraInfo.model* mat4Inverse(cameraInfo.projection);
		float near = csNearFar.x;
		float far = csNearFar.y;
		Vec3f csVertices[8] = {
			{ -1, -1,  far },
			{  1, -1,  far },
			{ -1,  1,  far },
			{  1,  1,  far },
			{ -1, -1,  near },
			{  1, -1,  near },
			{ -1,  1,  near },
			{  1,  1,  near },
		};

		std::transform(csVertices, csVertices + std::size(csVertices), result.vertices, [&wsMat](Vec3f csVertex)
		{
			Vec4f result = wsMat * Vec4f(csVertex, 1.0f);
			return Vec3f(result.x, result.y, result.z) * (1 / result.w);
		});
		return result;
	}

	struct FrustumBoxIntersection
	{
		Vec3f vertices[64];
		soul_size count = 0.0f;

		void add(const Vec3f& point)
		{
			vertices[count] = point;
			count++;
		}
	};

	static FrustumBoxIntersection ComputeFrustumBoxIntersection(const FrustumVertices& frustumVertices, const soul::AABB& box) noexcept
	{
		FrustumBoxIntersection result;

		/*
		 * Clip the world-space view volume (frustum) to the world-space scene volume (AABB),
		 * the result is guaranteed to be a convex-hull and is returned as an array of point.
		 *
		 * Algorithm:
		 * a) keep the view frustum vertices that are inside the scene's AABB
		 * b) keep the scene's AABB that are inside the view frustum
		 * c) keep intersection of AABB edges with view frustum planes
		 * d) keep intersection of view frustum edges with AABB planes
		 *
		 */

		AABB::Corners boxCorners = box.getCorners();

		// a) Keep the frustum's vertices that are known to be inside the scene's box
		for (const Vec3f& vertex : frustumVertices.vertices)
		{
			if (box.isInside(vertex))
			{
				result.add(vertex);
			}
		}

		const bool someFrustumVerticesAreInTheBox = result.count > 0;
		constexpr const float EPSILON = 1.0f / 8192.0f; // ~0.012 mm

		// at this point if we have 8 vertices, we can skip the rest
		if (result.count < 8) {
			Frustum frustum(frustumVertices.vertices);
			const Frustum::Planes& wsFrustumPlanes = frustum.planes;

			// b) add the scene's vertices that are known to be inside the view frustum
			//
			// We calculate the distance of the point in the plane's normal direction and substract it with the distance of the plane
			// If the result is negative for all planes it means the corner is inside the frustum
			//
			// We need to handle the case where a corner of the box lies exactly on a plane of
			// the frustum. This actually happens often due to fitting light-space
			// We fudge the distance to the plane by a small amount.

			for (Vec3f p : boxCorners.vertices) {
				const Plane& leftPlane = wsFrustumPlanes[Frustum::Side::LEFT];
				const Plane& rightPlane = wsFrustumPlanes[Frustum::Side::RIGHT];
				const Plane& botPlane = wsFrustumPlanes[Frustum::Side::BOTTOM];
				const Plane& topPlane = wsFrustumPlanes[Frustum::Side::TOP];
				const Plane& farPlane = wsFrustumPlanes[Frustum::Side::FAR];
				const Plane& nearPlane = wsFrustumPlanes[Frustum::Side::NEAR];

				float l = dot(leftPlane.normal, p) - leftPlane.d;
				float r = dot(rightPlane.normal, p) - rightPlane.d;
				float b = dot(botPlane.normal, p) - botPlane.d;
				float t = dot(topPlane.normal, p) - topPlane.d;
				float f = dot(farPlane.normal, p) - farPlane.d;
				float n = dot(nearPlane.normal, p) - nearPlane.d;
				if ((l <= EPSILON) && (r <= EPSILON) &&
					(b <= EPSILON) && (t <= EPSILON) &&
					(f <= EPSILON) && (n <= EPSILON)) {
					result.add(p);
				}
			}

			/*
			 * It's not enough here to have all 8 vertices, consider this:
			 *
			 *                     +
			 *                   / |
			 *                 /   |
			 *    +---------C/--B  |
			 *    |       A/    |  |
			 *    |       |     |  |
			 *    |       A\    |  |
			 *    +----------\--B  |
			 *                 \   |
			 *                   \ |
			 *                     +
			 *
			 * A vertices will be selected by step (a)
			 * B vertices will be selected by step (b)
			 *
			 * if we stop here, the segment (A,B) is inside the intersection of the box and the
			 * frustum.  We do need step (c) and (d) to compute the actual intersection C.
			 *
			 * However, a special case is if all the vertices of the box are inside the frustum.
			 */

			if (someFrustumVerticesAreInTheBox || result.count < 8) {

				auto addSegmentQuadIntersection = [&result](const Vec3f* segmentVertices, const Vec3f* quadVertices)
				{
					static constexpr const Vec2ui8 sBoxSegments[12] = {
					{ 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 },
					{ 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 },
					{ 0, 4 }, { 1, 5 }, { 3, 7 }, { 2, 6 },
					};

					static constexpr const Vec4ui8 sBoxQuads[6] = {
					{ 2, 0, 1, 3 },  // far
					{ 6, 4, 5, 7 },  // near
					{ 2, 0, 4, 6 },  // left
					{ 3, 1, 5, 7 },  // right
					{ 0, 4, 5, 1 },  // bottom
					{ 2, 6, 7, 3 },  // top
					};

					for (const Vec2ui8 segment : sBoxSegments) {
						const Vec3f s0{ segmentVertices[segment.x] };
						const Vec3f s1{ segmentVertices[segment.y] };
						// each segment should only intersect with 2 quads at most
						size_t maxVertexCount = result.count + 2;
						for (size_t j = 0; j < 6 && result.count < maxVertexCount; ++j) {
							const Vec4ui8 quad = sBoxQuads[j];
							const Vec3f t0{ quadVertices[quad.x] };
							const Vec3f t1{ quadVertices [quad.y] };
							const Vec3f t2{ quadVertices[quad.z] };
							const Vec3f t3{ quadVertices[quad.w] };
							IntersectPointResult intersectRes = IntersectSegmentQuad(s0, s1, t0, t1, t2, t3);
							if (intersectRes.intersect) result.add(intersectRes.point);
						}
					}
				};

				// c) intersect scene's volume edges with frustum planes
				addSegmentQuadIntersection(boxCorners.vertices, frustumVertices.vertices);

				// d) intersect frustum edges with the scene's volume planes
				addSegmentQuadIntersection(frustumVertices.vertices, boxCorners.vertices);
			}
		}

		return result;
	}

	static soul::AABB ComputeAABB(const Mat4f& transform, const BoundingSphere& sphere) noexcept
	{
		Vec3f position = project(transform, sphere.position);
		Vec3f extent(sphere.radius, sphere.radius, sphere.radius);
		return AABB(position - extent, position + extent);
	}

	static soul::AABB ComputeAABB(const Mat4f& transform, const Vec3f* vertices, soul_size count) noexcept
	{
		soul::AABB result;
		std::for_each(vertices, vertices + count, [transform, &result](Vec3f vertex)
		{
			const Vec3f tVertex = project(transform,vertex);
			result.min = componentMin(result.min, tVertex);
			result.max = componentMax(result.max, tVertex);
		});
		return result;
	}

	soul::Mat4f ShadowMap::getTextureCoordsMapping()
	{
		soul::Vec4f mtRows[4] = {
			Vec4f(0.5f, 0.0f, 0.0f, 0.5f),
			Vec4f(0.0f, -0.5f, 0.0f, 0.5f),
			Vec4f(0.0f, 0.0f, -0.5f, 0.5f),
			Vec4f(0.0f, 0.0f, 0.0f, 1.0f)
		};
		const soul::Mat4f mt = mat4FromRows(mtRows);

		const float v = (float)shadowMapInfo.textureDimension / (float)shadowMapInfo.atlasDimension;
		const float mvVals[16] = {
			v, 0.0f, 0.0f, 0.0f,
			0.0f, v, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
		const soul::Mat4f mv = mat4(mvVals);

		// apply the 1-texel border viewport transform
		const float o = 1.0f / float(shadowMapInfo.atlasDimension);
		const float s = 1.0f - 2.0f * (1.0f / float(shadowMapInfo.textureDimension));
		const float mbVals[16] = {
			s,    0.0f, 0.0f, o,
			0.0f, s,    0.0f, o,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
		const soul::Mat4f mb = mat4(mbVals);

		return mb * mv * mt;
	}


	ShadowMap ShadowMap::Create(const ShadowMapInfo& shadowMapInfo, const LightInfo& lightInfo, const soul_fila::CameraInfo& viewCamera, const SceneInfo& sceneInfo, Vec2f csNearFar)
	{
		ShadowMap shadowMap;
		shadowMap.shadowMapInfo = shadowMapInfo;

		const ShadowParams& params = lightInfo.shadowParams;
		shadowMap.depthBias.constant = -params.options.polygonOffsetConstant;
		shadowMap.depthBias.slope = -params.options.polygonOffsetSlope;

		soul::Mat4f projection = viewCamera.cullingProjection;
		if (params.options.shadowFar > 0.0f)
		{
			float n = viewCamera.zn;
			float f = params.options.shadowFar;
			SOUL_ASSERT(0, projection.elem[3][3] != 1.0f, "Do not support orthographic camera view yet");
			projection = mat4Perspective(projection, n, f);
		}

		soul_fila::CameraInfo cameraInfo = viewCamera;
		cameraInfo.projection = projection;

		switch (lightInfo.lightType.type)
		{
		case LightRadiationType::SUN:
		case LightRadiationType::DIRECTIONAL:
			shadowMap.computeShadowCameraDirectional(lightInfo.direction, cameraInfo, lightInfo.shadowParams, sceneInfo, csNearFar);
			break;
		case LightRadiationType::FOCUSED_SPOT:
		case LightRadiationType::SPOT:
			SOUL_NOT_IMPLEMENTED();
			break;
		case LightRadiationType::POINT:
			break;
		case LightRadiationType::COUNT:
		default:
			SOUL_NOT_IMPLEMENTED();
			break;
		}

		return shadowMap;
	}

	void ShadowMap::computeShadowCameraDirectional(const Vec3f& dir, const CameraInfo& camera, const ShadowParams& params, const SceneInfo& cascadeParams, Vec2f csNearFar) noexcept
	{
		const soul::Mat4f lightViewMatrixOrigin = mat4View(Vec3f(0, 0, 0), dir, Vec3f(0.0f, 1.0f, 0.0f));
		const AABB wsShadowCastersVolume = cascadeParams.wsShadowCastersVolume;
		const AABB wsShadowReceiversVolume = cascadeParams.wsShadowReceiversVolume;

		if (wsShadowCastersVolume.isEmpty() || wsShadowReceiversVolume.isEmpty())
		{
			hasVisibleShadow = false;
			return;
		}

		FrustumVertices wsViewFrustumVertices = ComputeCameraFrustumVertices(camera, csNearFar);
		FrustumBoxIntersection wsClippedShadowReceiverVolume = ComputeFrustumBoxIntersection(wsViewFrustumVertices, wsShadowReceiversVolume);

		AABB lsLightFrustumBounds;
		if (!USE_DEPTH_CLAMP)
		{
			lsLightFrustumBounds.max.z = cascadeParams.lsNearFar.x;
		}
		for (soul_size i = 0; i < wsClippedShadowReceiverVolume.count; i++)
		{
			Vec3f v = lightViewMatrixOrigin * wsClippedShadowReceiverVolume.vertices[i];
			lsLightFrustumBounds.min.z = std::min(lsLightFrustumBounds.min.z, v.z);
			if (USE_DEPTH_CLAMP)
			{
				lsLightFrustumBounds.max.z = std::max(lsLightFrustumBounds.max.z, v.z);
			}
		}
		lsLightFrustumBounds.min.z = std::max(lsLightFrustumBounds.min.z, cascadeParams.lsNearFar.y);

		Vec3f position = dir * -lsLightFrustumBounds.max.z;
		Vec3f target = position + dir;
		const soul::Mat4f lightViewMatrix = mat4View(position, target, Vec3f(0.0f, 1.0f, 0.0f));
		znear = 0.0f;
		zfar = lsLightFrustumBounds.max.z - lsLightFrustumBounds.min.z;
		if(SOUL_UNLIKELY(znear >= zfar))
		{
			hasVisibleShadow = false;
			return;
		}

		soul::BoundingSphere viewVolumeBoundingSphere;
		if (params.options.stable)
		{
			// In stable mode, the light frustum size must be fixed, so we can choose either the
			// whole view frustum, or the whole scene bounding volume. We simply pick whichever is
			// is smaller.

			auto computeBoundingSphere = [](Vec3f* vertices, soul_size count) -> BoundingSphere
			{
				BoundingSphere result;
				for (size_t i = 0; i < count; i++) {
					result.position += vertices[i];
				}
				result.position *= 1.0f / float(count);
				for (size_t i = 0; i < count; i++) {
					result.radius = std::max(result.radius, length(vertices[i] - result.position));
				}
				result.radius = std::sqrt(result.radius);
				return result;
			};

			BoundingSphere shadowReceiverVolumeBoundingSphere = computeBoundingSphere(wsShadowReceiversVolume.getCorners().vertices, 8);
			viewVolumeBoundingSphere = computeBoundingSphere(wsViewFrustumVertices.vertices, 8);
			if (shadowReceiverVolumeBoundingSphere.radius < viewVolumeBoundingSphere.radius)
			{
				viewVolumeBoundingSphere.radius = 0.0f;
				std::copy_n(wsShadowReceiversVolume.getCorners().vertices, 8, wsClippedShadowReceiverVolume.vertices);
				wsClippedShadowReceiverVolume.count = 8;
			}
		}

		hasVisibleShadow = wsClippedShadowReceiverVolume.count >= 2;
		if (hasVisibleShadow)
		{
			auto directionLightFrustum = [](float near, float far) -> soul::Mat4f
			{
				const float d = far - near;
				soul::Mat4f m = mat4Identity();
				m.elem[2][2] = -2 / d;
				m.elem[2][3] = -(far + near) / d;
				return m;
			};
			Mat4f Mp = directionLightFrustum(znear, zfar);
			Mat4f MpMv = Mp * lightViewMatrix;

			AABB bounds;
			if (params.options.stable && viewVolumeBoundingSphere.radius > 0.0f)
			{
				bounds = ComputeAABB(lightViewMatrix, viewVolumeBoundingSphere);
			} else
			{
				bounds = ComputeAABB(MpMv, wsClippedShadowReceiverVolume.vertices, wsClippedShadowReceiverVolume.count);
			}
			lsLightFrustumBounds.min.xy = bounds.min.xy;
			lsLightFrustumBounds.max.xy = bounds.max.xy;

			if (SOUL_UNLIKELY(lsLightFrustumBounds.min.x >= lsLightFrustumBounds.max.x) ||
				(lsLightFrustumBounds.min.y >= lsLightFrustumBounds.max.y))
			{
				hasVisibleShadow = false;
				return;
			}

			// compute focus scale and offset
			Vec2f s = 2.0f / Vec2f(lsLightFrustumBounds.max.xy - lsLightFrustumBounds.min.xy);
			Vec2f o = -s * Vec2f(lsLightFrustumBounds.max.xy + lsLightFrustumBounds.min.xy) * 0.5f;

			if (params.options.stable)
			{
				// snap to texel increment
				auto fmod = [](Vec2f vec1, Vec2f vec2) -> Vec2f {
					auto mod = [](float x, float y) -> float { return std::fmod(x, y); };
					return Vec2f(std::fmod(vec1.x, vec2.x), mod(vec1.y, vec2.y));
				};

				// This snaps the shadow map bounds to texels.
				// The 2.0 comes from Mv having a NDC in the range -1,1 (so a range of 2).
				float csTexelUnit = 2.0f / float(shadowMapInfo.shadowDimension);
				const Vec2f r = Vec2f(csTexelUnit, csTexelUnit);
				o -= fmod(o, r);

				// This offsets the texture coordinates so it has a fixed offset w.r.t the world
				const Vec2f lsOrigin = (lightViewMatrix * camera.worldOrigin.columns(3)).xy * s;
				o -= fmod(lsOrigin, r);
			}

			const float fData[16] = {
				s.x, 0.0f, 0.0f, o.x,
				0.0f, s.y, 0.0f, o.y,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			};
			const Mat4f F = mat4(fData);

			

			const Mat4f S = F * MpMv;

			// We apply the constant bias in world space (as opposed to light-space) to account
			// for perspective and lispsm shadow maps. This also allows us to do this at zero-cost
			// by baking it in the shadow-map itself.
			const float constantBias = shadowMapInfo.vsm ? 0.0f : params.options.constantBias;
			const soul::Mat4f b = mat4Translate(dir * constantBias);

			renderProjectionMatrix = F * Mp;
			renderViewMatrix = lightViewMatrix * b;

			const Mat4f MbMt = getTextureCoordsMapping();
			const Mat4f St = MbMt * S;
			if (shadowMapInfo.vsm)
			{
				sampleMatrix = ComputeVSMLightViewMatrix(sampleMatrix, lightViewMatrix, zfar);
			}  else
			{
				sampleMatrix = St;
			}
			texelSizeWs = TexelSizeWorldSpace(mat3UpperLeft(St), shadowMapInfo.shadowDimension);

		}

	}

	void ShadowMapGenPass::calculateTextureRequirements(const Scene& scene, const Lights& lights)
	{
		const EntityID* entities = lights.data<LightsIdx::ENTITY_ID>();
		uint8 layer = 0;
		uint32 maxDimension = 0;
		for (soul_size lightIdx = 0; lightIdx < lights.size(); lightIdx++)
		{
			const LightComponent& lightComp = scene.get_light_component(entities[lightIdx]);
			if (!lightComp.lightType.shadowCaster)
			{
				continue;
			}
			if (lightComp.lightType.type == LightRadiationType::POINT)
			{
				// we don't support point light yet
				continue;
			}
			maxDimension = std::max(maxDimension, lightComp.shadowParams.options.mapSize);
			layer++;
		}

		// Generate mipmaps for VSM when anisotropy is enabled or when requested
		const bool useMipmapping = shadowType == ShadowType::VSM && ((vsmOptions.anisotropy > 0) || vsmOptions.mipmapping);

		uint8 mipLevels = 1u;
		if (useMipmapping)
		{
			// Limit the lowest mipmap level to 256x256.
			// This avoids artifacts on high derivative tangent surfaces.
			int lowMipmapLevel = 7;
			mipLevels = soul::cast<uint8>(std::max(1, std::max(1, std::ilogbf(float(maxDimension)) + 1) - lowMipmapLevel));
		}

		textureRequirements = {
			soul::cast<uint16>(maxDimension),
			layer,
			mipLevels
		};
	}

	ShadowMapGenPass::ShadowTechniqueFlags ShadowMapGenPass::prepare(const Scene& scene, const CameraInfo& cameraInfo, Renderables& renderables, Lights& lights, FrameUBO& frameUBO)
	{
		calculateTextureRequirements(scene, lights);
		ShadowTechniqueFlags shadowTechnique = 0;
		shadowTechnique |= prepareCascadeShadowMaps(scene, cameraInfo, renderables, lights, frameUBO);

		frameUBO.vsmExponent = vsmOptions.exponent;
		frameUBO.vsmDepthScale = vsmOptions.minVarianceScale;
		frameUBO.vsmLightBleedReduction = vsmOptions.lightBleedReduction;
		return shadowTechnique;
	}

	static Vec2f ComputeNearFar(const Mat4f& view, const Vec3f* vertices, soul_size count)
	{
		Vec2f nearFar(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
		for (soul_size vertexIdx = 0; vertexIdx < count; vertexIdx++)
		{
			Vec3f vsVertex = view * vertices[vertexIdx];
			nearFar.x = std::max(nearFar.x, vsVertex.z);
			nearFar.y = std::min(nearFar.y, vsVertex.z);
		}
		return nearFar;
	}

	static Vec2f ComputeNearFar(const Mat4f& view, const AABB& aabb)
	{
		AABB::Corners corners = aabb.getCorners();
		return ComputeNearFar(view, corners.vertices, AABB::Corners::COUNT);
	}

	static ShadowMap::SceneInfo ComputeSceneInfo(Vec3f dir, const Renderables& renderables, const CameraInfo& cameraInfo, uint8 visibleLayers)
	{
		ShadowMap::SceneInfo sceneInfo;
		Mat4f lightView = mat4View(Vec3f(), dir, Vec3f(0.0f, 1.0f, 0.0f));

		sceneInfo.lsNearFar = Vec2f(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
		sceneInfo.vsNearFar = Vec2f(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());

		const Vec3f* const SOUL_RESTRICT worldAABBCenter = renderables.data<RenderablesIdx::WORLD_AABB_CENTER>();
		const Vec3f* const SOUL_RESTRICT worldAABBExtent = renderables.data<RenderablesIdx::WORLD_AABB_EXTENT>();
		const uint8* const SOUL_RESTRICT layers = renderables.data<RenderablesIdx::LAYERS>();
		const Visibility* const SOUL_RESTRICT visibility = renderables.data<RenderablesIdx::VISIBILITY_STATE>();
		size_t c = renderables.size();
		for (size_t i = 0; i < c; i++) {
			if (layers[i] & visibleLayers) {
				const soul::AABB aabb{ worldAABBCenter[i] - worldAABBExtent[i],
								 worldAABBCenter[i] + worldAABBExtent[i] };
				if (visibility[i].castShadows) {
					sceneInfo.wsShadowCastersVolume.min =
						componentMin(sceneInfo.wsShadowCastersVolume.min, aabb.min);
					sceneInfo.wsShadowCastersVolume.max =
						componentMax(sceneInfo.wsShadowCastersVolume.max, aabb.max);
					Vec2f nf = ComputeNearFar(lightView, aabb);
					sceneInfo.lsNearFar.x = std::max(sceneInfo.lsNearFar.x, nf.x);  // near
					sceneInfo.lsNearFar.y = std::min(sceneInfo.lsNearFar.y, nf.y);  // far
				}
				if (visibility[i].receiveShadows) {
					sceneInfo.wsShadowReceiversVolume.min =
						componentMin(sceneInfo.wsShadowReceiversVolume.min, aabb.min);
					sceneInfo.wsShadowReceiversVolume.max =
						componentMax(sceneInfo.wsShadowReceiversVolume.max, aabb.max);
					Vec2f nf = ComputeNearFar(cameraInfo.view, aabb);
					sceneInfo.vsNearFar.x = std::max(sceneInfo.vsNearFar.x, nf.x);
					sceneInfo.vsNearFar.y = std::min(sceneInfo.vsNearFar.y, nf.y);
				}
			}
		}

		return sceneInfo;
	}

	ShadowMapGenPass::ShadowTechniqueFlags ShadowMapGenPass::prepareCascadeShadowMaps(const Scene& scene, const CameraInfo& cameraInfo, Renderables& renderables, Lights& lights, FrameUBO& frameUBO)
	{
		cascadeShadowMaps.clear();
		EntityID entityID = lights.elementAt<LightsIdx::ENTITY_ID>(0);
		if (entityID == ENTITY_ID_NULL) return 0;
		const LightComponent& lightComp = scene.get_light_component(entityID);
		if (!lightComp.lightType.shadowCaster) return 0;

		Vec3f direction = lights.elementAt<LightsIdx::DIRECTION>(0);

		ShadowMap::SceneInfo sceneInfo = ComputeSceneInfo(direction, renderables, cameraInfo, scene.get_visible_layers());

		ShadowMap::ShadowMapInfo shadowMapInfo;
		shadowMapInfo.zResolution = textureZResolution;
		shadowMapInfo.atlasDimension = textureRequirements.size;
		shadowMapInfo.textureDimension = uint16(lightComp.shadowParams.options.mapSize);
		shadowMapInfo.shadowDimension = shadowMapInfo.textureDimension - 2;
		shadowMapInfo.vsm = shadowType == ShadowType::VSM;

		ShadowMap::LightInfo lightInfo;
		lightInfo.lightType = lightComp.lightType;
		lightInfo.shadowParams = lightComp.shadowParams;
		lightInfo.direction = direction;
		
		ShadowMap shadowMap = ShadowMap::Create(shadowMapInfo, lightInfo, cameraInfo, sceneInfo, Vec2f(-1.0f, 1.0f));
		frameUBO.shadowBias = { 0.0f, lightComp.shadowParams.options.normalBias * shadowMap.texelSizeWs, 0.0f };

		Frustum shadowFrustum(shadowMap.renderProjectionMatrix * shadowMap.renderViewMatrix);
		Cull(renderables, shadowFrustum, VISIBLE_DIR_SHADOW_RENDERABLE_BIT);

		// Adjust the near and far planes to tightly bound the scene.
		float vsNear = -cameraInfo.zn;
		float vsFar = -cameraInfo.zf;
		vsNear = std::min(vsNear, sceneInfo.vsNearFar.x);
		vsFar = std::max(vsFar, sceneInfo.vsNearFar.y);

		const ShadowOptions& options = lightComp.shadowParams.options;
		const size_t cascadeCount = lightComp.shadowParams.options.shadowCascades;

		// We divide the camera frustum into N cascades. This gives us N + 1 split positions.
		// The first split position is the near plane; the last split position is the far plane.
		std::array<float, CascadeSplits::SPLIT_COUNT> splitPercentages{};
		splitPercentages[cascadeCount] = 1.0f;
		for (size_t i = 1; i < cascadeCount; i++) {
			splitPercentages[i] = options.cascadeSplitPositions[i - 1];
		}

		CascadeSplits::Params p;
		p.proj = cameraInfo.cullingProjection;
		p.near = vsNear;
		p.far = vsFar;
		p.cascadeCount = cascadeCount;
		p.splitPositions = splitPercentages;
		cascadeSplits = CascadeSplits(p);

		// The split positions uniform is a float4. To save space, we chop off the first split position
		// (which is the near plane, and doesn't need to be communicated to the shaders).
		static_assert(CONFIG_MAX_SHADOW_CASCADES <= 5,
			"At most, a float4 can fit 4 split positions for 5 shadow cascades");
		
		Vec4f wsSplitPositionUniform(-std::numeric_limits<float>::infinity());
		std::copy(cascadeSplits.beginWs() + 1, cascadeSplits.endWs(), &wsSplitPositionUniform.mem[0]);
		frameUBO.cascadeSplits = wsSplitPositionUniform;

		float csSplitPosition[CONFIG_MAX_SHADOW_CASCADES + 1];
		std::copy(cascadeSplits.beginCs(), cascadeSplits.endCs(), csSplitPosition);

		ShadowTechniqueFlags shadowTechnique = 0;
		uint32_t directionalShadowsMask = 0;
		uint32_t cascadeHasVisibleShadows = 0;
		float screenSpaceShadowDistance = 0.0f;
		for (size_t i = 0, c = cascadeCount; i < c; i++) {
			Vec2f csNearFar = { csSplitPosition[i], csSplitPosition[i + 1] };
			ShadowMap cascadeShadowMap = ShadowMap::Create(shadowMapInfo, lightInfo, cameraInfo, sceneInfo, csNearFar);
			if (cascadeShadowMap.hasVisibleShadow) {
				frameUBO.lightFromWorldMatrix[i] = cascadeShadowMap.sampleMatrix;
				shadowTechnique |= SHADOW_TECHNIQUE_SHADOW_MAP_BIT;
				cascadeHasVisibleShadows |= 0x1u << i;
			}
			cascadeShadowMaps.add(cascadeShadowMap);
		}

		// screen-space contact shadows for the directional light
		screenSpaceShadowDistance = options.maxShadowDistance;
		if (options.screenSpaceContactShadows) {
			shadowTechnique |= SHADOW_TECHNIQUE_SCREEN_SPACE_BIT;
		}
		directionalShadowsMask |= std::min(uint8_t(255u), options.stepCount) << 8u;

		if (shadowTechnique & SHADOW_TECHNIQUE_SHADOW_MAP_BIT) {
			directionalShadowsMask |= 0x1u;
		}
		if (shadowTechnique & SHADOW_TECHNIQUE_SCREEN_SPACE_BIT) {
			directionalShadowsMask |= 0x2u;
		}

		frameUBO.directionalShadows = directionalShadowsMask;
		frameUBO.ssContactShadowDistance = screenSpaceShadowDistance;
		
		uint32_t cascades = 0;
		cascades |= uint32_t(cascadeCount);
		cascades |= cascadeHasVisibleShadows << 8u;
		frameUBO.cascades = cascades;

		return shadowTechnique;
	}

	ShadowMapGenPass::CascadeSplits::CascadeSplits(Params const& params) noexcept
		: mSplitsWs(), mSplitsCs(), mSplitCount(params.cascadeCount + 1) {
		for (size_t s = 0; s < mSplitCount; s++) {
			mSplitsWs[s] = params.near + (params.far - params.near) * params.splitPositions[s];
			Vec4f p = params.proj * Vec4f(0.0f, 0.0f, mSplitsWs[s], 1.0f);
			mSplitsCs[s] = p.z / p.w;
		}
	}

	ShadowMapGenPass::Output ShadowMapGenPass::computeRenderGraph(soul::gpu::RenderGraph& renderGraph, const ShadowMapGenPass::Input& input, const RenderData& renderData, const Scene& scene)
	{
		struct Parameter
		{
			soul::gpu::BufferNodeID frameUBO;
			soul::gpu::BufferNodeID objectsUBO;
			soul::gpu::BufferNodeID bonesUBO;
			soul::gpu::BufferNodeID materialsUBO;
		};

		Array<FrameUBO> frameUBOs;
		frameUBOs.resize(cascadeShadowMaps.size());
		for (soul_size shadowMapIdx = 0; shadowMapIdx < cascadeShadowMaps.size(); shadowMapIdx++)
		{
			FrameUBO& frameUBO = frameUBOs[shadowMapIdx];
			const ShadowMap& shadowMap = cascadeShadowMaps[shadowMapIdx];

			frameUBO = renderData.frameUBO;

			const Mat4f viewFromWorld(shadowMap.renderViewMatrix);
			const Mat4f worldFromView(mat4Inverse(shadowMap.renderViewMatrix));
			const Mat4f projectionMatrix(shadowMap.renderProjectionMatrix);

			const Mat4f clipFromView(projectionMatrix);
			const Mat4f viewFromClip(mat4Inverse(clipFromView));
			const Mat4f clipFromWorld(clipFromView * viewFromWorld);
			const Mat4f worldFromClip(worldFromView* viewFromClip);

			frameUBO.viewFromWorldMatrix = viewFromWorld;    // view
			frameUBO.worldFromViewMatrix = worldFromView;    // model
			frameUBO.clipFromViewMatrix = clipFromView;      // projection
			frameUBO.viewFromClipMatrix = viewFromClip;      // 1/projection
			frameUBO.clipFromWorldMatrix = clipFromWorld;    // projection * view
			frameUBO.worldFromClipMatrix = worldFromClip;    // 1/(projection * view)
			frameUBO.cameraPosition = worldFromView.columns(3).xyz;
			frameUBO.worldOffset = Vec3f(0.0f, 0.0f, 0.0f);
			frameUBO.cameraFar = shadowMap.zfar;

			float res = shadowMap.shadowMapInfo.shadowDimension;
			frameUBO.resolution = Vec4f(res, res, 1 / res, 1 / res);
			frameUBO.origin = Vec2f(1.0f, 1.0f);

			frameUBO.vsmExponent = vsmOptions.exponent;
			frameUBO.vsmDepthScale = vsmOptions.minVarianceScale * 0.01f * vsmOptions.exponent;
			frameUBO.vsmLightBleedReduction = vsmOptions.lightBleedReduction;

			frameUBOs[shadowMapIdx] = frameUBO;
		}

		soul::gpu::BufferDesc frameUBODesc;
		frameUBODesc.typeSize = sizeof(FrameUBO);
		frameUBODesc.typeAlignment = alignof(FrameUBO);
		frameUBODesc.count = frameUBOs.size();
		frameUBODesc.usageFlags = { gpu::BufferUsage::UNIFORM };
		frameUBODesc.queueFlags = { gpu::QueueType::GRAPHIC };
		soul::gpu::BufferID frameGPUBuffer = gpuSystem->create_buffer(frameUBODesc, frameUBOs.data());
		gpuSystem->destroy_buffer(frameGPUBuffer);
		soul::gpu::BufferNodeID frameUniformBuffer = renderGraph.import_buffer("Frame Uniform Buffer", frameGPUBuffer);

		Parameter inputParam;
		inputParam.frameUBO = frameUniformBuffer;
		inputParam.objectsUBO = input.objectsUb;
		inputParam.bonesUBO = input.bonesUb;
		inputParam.materialsUBO = input.materialsUb;

		const auto shadowMapDesc = gpu::RGTextureDesc::create_d2_array(gpu::TextureFormat::DEPTH16,
			textureRequirements.levels,
			Vec2ui32(textureRequirements.size, textureRequirements.size),
			textureRequirements.layers,
			true);
		gpu::TextureNodeID shadowMapNode = renderGraph.create_texture("Shadowmap", shadowMapDesc);

		for (soul_size shadowMapIdx = 0; shadowMapIdx < cascadeShadowMaps.size(); shadowMapIdx++)
		{
			const gpu::DepthStencilAttachmentDesc depth_attachment_desc = {
				.nodeID = shadowMapNode,
				.view = gpu::SubresourceIndex(0, soul::cast<uint8>(shadowMapIdx)),
				.depthWriteEnable = true,
				.clear = true
			};

			const ShadowMap& shadowMap = cascadeShadowMaps[shadowMapIdx];
			SOUL_ASSERT(0, !shadowMap.shadowMapInfo.vsm, "");
			const auto& node = renderGraph.add_graphic_pass<Parameter>("Shadow Map Pass",
				gpu::RGRenderTargetDesc(
					Vec2ui32(shadowMap.shadowMapInfo.atlasDimension, shadowMap.shadowMapInfo.atlasDimension),
					depth_attachment_desc
				),
				[this, shadowMapIdx, &inputParam, dimension = shadowMap.shadowMapInfo.atlasDimension](gpu::RGShaderPassDependencyBuilder& builder, Parameter& params)
				{
					params.frameUBO = builder.add_shader_buffer(inputParam.frameUBO, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }, gpu::ShaderBufferReadUsage::UNIFORM);
					params.bonesUBO = builder.add_shader_buffer(inputParam.bonesUBO, { gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT }, gpu::ShaderBufferReadUsage::UNIFORM);
					params.objectsUBO = builder.add_shader_buffer(inputParam.objectsUBO, { gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT }, soul::gpu::ShaderBufferReadUsage::UNIFORM);
					params.materialsUBO = builder.add_shader_buffer(inputParam.materialsUBO, { gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT }, soul::gpu::ShaderBufferReadUsage::UNIFORM);

				},
				[this, shadowMapIdx, &renderData, &shadowMap, &scene](const Parameter& params, soul::gpu::RenderGraphRegistry& registry, soul::gpu::GraphicCommandList& command_list)
				{
					const ShadowMap::ShadowMapInfo& shadow_map_info = shadowMap.shadowMapInfo;

					const Mat4f& model = shadowMap.sampleMatrix;
					Vec3f camera_forward = unit(soul::Vec3f(model.columns(2).xyz) * -1.0f);
					Vec3f camera_position = model.columns(3).xyz;

					const Renderables& renderables = renderData.renderables;
					auto const* const SOUL_RESTRICT soa_world_aabb_center = renderables.data<RenderablesIdx::WORLD_AABB_CENTER>();
					auto const* const SOUL_RESTRICT soa_reversed_winding = renderables.data<RenderablesIdx::REVERSED_WINDING_ORDER>();
					auto const* const SOUL_RESTRICT soa_visibility = renderables.data<RenderablesIdx::VISIBILITY_STATE>();
					auto const* const SOUL_RESTRICT soa_primitives = renderables.data<RenderablesIdx::PRIMITIVES>();
					auto const* const SOUL_RESTRICT soa_primitive_count = renderables.data<RenderablesIdx::SUMMED_PRIMITIVE_COUNT>();

					const auto range = renderData.directionalShadowCasters;
					soul::runtime::ScopeAllocator<> scopeAllocator("Shadow map draw items");
					Array<DrawItem> draw_items(&scopeAllocator);
					const soul_size draw_item_count = soa_primitive_count[range.last] - soa_primitive_count[range.first];
					draw_items.resize(draw_item_count);

					for (soul_size renderable_idx : range)
					{

						float distance = dot(soa_world_aabb_center[renderable_idx], camera_forward) - dot(camera_position, camera_forward);
						distance = -distance;
						const uint32_t distanceBits = reinterpret_cast<uint32_t&>(distance);

						auto variant = GPUProgramVariant(GPUProgramVariant::DEPTH_VARIANT);
						variant.setSkinning(soa_visibility[renderable_idx].skinning || soa_visibility[renderable_idx].morphing);
						// calculate the per-primitive face winding order inversion
						const bool inverse_front_faces = soa_reversed_winding[renderable_idx];

						DrawItem item;
						item.key = to_underlying(Pass::DEPTH);
						item.key |= to_underlying(CustomCommand::PASS);
						item.key |= MakeField(soa_visibility[renderable_idx].priority, PRIORITY_MASK, PRIORITY_SHIFT);
						item.key |= MakeField(distanceBits, DISTANCE_BITS_MASK, DISTANCE_BITS_SHIFT);
						item.index = soul::cast<uint32>(renderable_idx);
						item.rasterState = {};
						item.rasterState.colorWrite = false;
						item.rasterState.depthWrite = true;
						item.rasterState.depthFunc = RasterState::DepthFunc::GREATER_OR_EQUAL;
						item.rasterState.inverseFrontFaces = inverse_front_faces;

						const soul_size offset = soa_primitive_count[renderable_idx] - soa_primitive_count[range.first];

						DrawItem* curr = &draw_items[offset];
						for (const Primitive& primitive : *soa_primitives[renderable_idx])
						{
							const Material& material = scene.materials()[primitive.materialID.id];
							*curr = item;
							curr->primitive = &primitive;
							curr->material = &material;
							curr->programID = programRegistry->getProgram(material.programSetID, variant);
							++curr;
						}
					}

					std::sort(draw_items.begin(), draw_items.end());
					DrawItem const* const last = std::partition_point(draw_items.begin(), draw_items.end(),
						[](DrawItem const& c) {
							return c.key != to_underlying(Pass::SENTINEL);
						});
					draw_items.resize(last - draw_items.begin());

					gpu::GraphicPipelineStateDesc pipeline_base_desc;
					pipeline_base_desc.viewport = { 1, 1, shadow_map_info.shadowDimension, shadow_map_info.shadowDimension };
					pipeline_base_desc.scissor = { false, 1, 1, shadow_map_info.shadowDimension, shadow_map_info.shadowDimension };
					pipeline_base_desc.colorAttachmentCount = 1;
					pipeline_base_desc.depthStencilAttachment = { true, true, gpu::CompareOp::GREATER_OR_EQUAL };

					const gpu::SamplerDesc samplerDesc = gpu::SamplerDesc::SameFilterWrap(gpu::TextureFilter::LINEAR, gpu::TextureWrap::REPEAT);
					const gpu::SamplerID sampler_id = gpuSystem->request_sampler(samplerDesc);
					const gpu::Descriptor set0_descriptors[] = {
						gpu::Descriptor::Uniform(registry.get_buffer(params.frameUBO), soul::cast<uint32>(shadowMapIdx), {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT}),
					};
					gpu::ShaderArgSetID set0 = registry.get_shader_arg_set(0, { std::size(set0_descriptors), set0_descriptors });

					auto get_material_gpu_texture = [&scene, stubTexture = renderData.stubTexture](TextureID sceneTextureID)->soul::gpu::TextureID
					{
						return sceneTextureID.is_null() ? stubTexture : scene.textures()[sceneTextureID.id].gpuHandle;
					};

					using DrawCommand = gpu::RenderCommandDrawPrimitive;
					command_list.push<DrawCommand>(draw_items.size(), [&, sampler_id, set0](soul_size command_idx) {
						SOUL_PROFILE_ZONE_WITH_NAME("Build Command Shadow Map");
						const DrawItem& draw_item = draw_items[command_idx];
						const Primitive& primitive = *draw_item.primitive;
						const Material& material = *draw_item.material;
						gpu::GraphicPipelineStateDesc pipeline_desc = pipeline_base_desc;
						DrawItem::ToPipelineStateDesc(draw_item, pipeline_desc);

						const gpu::Descriptor set1_descriptors[] = {
							gpu::Descriptor::Uniform(registry.get_buffer(params.materialsUBO), soul::cast<uint32>(primitive.materialID.id), {gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT })
						};
						const gpu::ShaderArgSetID set1 = registry.get_shader_arg_set(1, { std::size(set1_descriptors), set1_descriptors });

						gpu::Descriptor set2_descriptors[] = {
							gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.baseColorTexture), sampler_id, {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT}),
							gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.metallicRoughnessTexture), sampler_id, {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT}),
							gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.normalTexture), sampler_id, {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT}),
							gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.occlusionTexture), sampler_id, {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT}),
							gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.emissiveTexture), sampler_id, {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT}),
							gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.clearCoatTexture), sampler_id, {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT}),
							gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.clearCoatRoughnessTexture), sampler_id, {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT}),
							gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.clearCoatNormalTexture), sampler_id, {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT}),
							gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.sheenColorTexture), sampler_id, {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT}),
							gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.sheenRoughnessTexture), sampler_id, {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT}),
							gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.transmissionTexture), sampler_id, {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT}),
							gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.volumeThicknessTexture), sampler_id, {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT})
						};
						const gpu::ShaderArgSetID set2 = registry.get_shader_arg_set(2, { std::size(set2_descriptors), set2_descriptors});

						soul::Array<gpu::Descriptor> set3_descriptors;
						set3_descriptors.reserve(gpu::MAX_BINDING_PER_SET);
						set3_descriptors.add(gpu::Descriptor::Uniform(registry.get_buffer(params.objectsUBO), draw_item.index, { gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT }));

						const SkinID skin_id = renderables.elementAt<RenderablesIdx::SKIN_ID>(draw_item.index);
						if (Visibility visibility = renderables.elementAt<RenderablesIdx::VISIBILITY_STATE>(draw_item.index); visibility.skinning || visibility.morphing) {
							const uint32 skin_index = skin_id.is_null() ? 0 : soul::cast<uint32>(skin_id.id);
							set3_descriptors.add(gpu::Descriptor::Uniform(registry.get_buffer(params.bonesUBO), skin_index, { gpu::ShaderStage::VERTEX }));
						}

						gpu::ShaderArgSetID set3 = registry.get_shader_arg_set(3, { soul::cast<uint32>(set3_descriptors.size()), set3_descriptors.data() });

						DrawCommand command = {
							.pipelineStateID = registry.get_pipeline_state(pipeline_desc),
							.shaderArgSetIDs = { set0, set1, set2, set3 },
							.indexBufferID = primitive.indexBuffer
						};
						for (uint32 attrib_idx = 0; attrib_idx < to_underlying(VertexAttribute::COUNT); attrib_idx++) {
							Attribute attribute = primitive.attributes[attrib_idx];
							if (attribute.buffer == Attribute::BUFFER_UNUSED) {
								attribute = primitive.attributes[0];
							}
							command.vertexBufferIDs[attrib_idx] = primitive.vertexBuffers[attribute.buffer];
						}
						return command;
					});
				}
			);
			shadowMapNode = node.get_render_target().depthStencilAttachment.outNodeID;
		}
		return { shadowMapNode };
	}
 }