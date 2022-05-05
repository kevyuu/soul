#include "renderer.h"
#include "core/geometry.h"
#include "gpu_program_registry.h"
#include "runtime/scope_allocator.h"

#include "exposure.h"
#include "zip2iterator.h"
#include "range.h"

#include "render_module/lighting_pass.h"


namespace soul_fila {

	void Cull(Renderables& renderables, const Frustum& frustum, soul_size bit)
	{
		const Vec3f* aabbCenter = renderables.data<RenderablesIdx::WORLD_AABB_CENTER>();
		const Vec3f* halfExtent = renderables.data<RenderablesIdx::WORLD_AABB_EXTENT>();
		VisibleMask* visibleMasks = renderables.data<RenderablesIdx::VISIBLE_MASK>();
		for (soul_size i = 0; i < renderables.size(); i++)
		{
			visibleMasks[i] |= FrustumCull(frustum, aabbCenter[i], halfExtent[i]) << bit;
		}
	}

	static void Cull(const Scene& scene, Lights& lights, const Frustum& frustum)
	{
		const Vec4f* spheres = lights.data<LightsIdx::POSITION_RADIUS>();
		VisibleMask* visibleMasks = lights.data<LightsIdx::VISIBLE_MASK>();
		for (soul_size i = 0; i < lights.size(); i++)
		{
			visibleMasks[i] |= uint8(FrustumCull(frustum, spheres[i]));
		}

		soul_size visibleLightCount = Scene::DIRECTIONAL_LIGHTS_COUNT;
		const EntityID* entities = lights.data<LightsIdx::ENTITY_ID>();
		const Vec3f* directions = lights.data<LightsIdx::DIRECTION>();
		for (soul_size i = Scene::DIRECTIONAL_LIGHTS_COUNT; i < lights.size(); i++)
		{
			if (visibleMasks[i])
			{
				const LightComponent& lightComp = scene.get_light_component(entities[i]);
				if (!lightComp.lightType.lightCaster)
				{
					visibleMasks[i] = 0;
					continue;
				}
				if (lightComp.intensity <= 0.0f)
				{
					visibleMasks[i] = 0;
					continue;
				}
				if (lightComp.lightType.type == LightRadiationType::FOCUSED_SPOT || lightComp.lightType.type == LightRadiationType::SPOT)
				{
					const Vec3f position = spheres[i].xyz;
					const Vec3f direction = directions[i];
					const float cosSqr = lightComp.spotParams.cosOuterSquared;
					bool invisible = false;
					//TODO(kevinyu) : implement cone-frustum intersection
				}
				visibleLightCount++;
			}
		}

		std::partition(lights.begin() + Scene::DIRECTIONAL_LIGHTS_COUNT, lights.end(),
			[](auto const& it)
			{
				return it.template get<LightsIdx::VISIBLE_MASK>() != 0;
			});
		lights.resize(std::min(visibleLightCount, CONFIG_MAX_LIGHT_COUNT + Scene::DIRECTIONAL_LIGHTS_COUNT));

	}

	static void ComputeCameraDistanceAndSort(const Scene& scene, const CameraInfo& cameraInfo, Lights& lights)
	{
		float* distances = (float*) runtime::get_temp_allocator()->allocate(sizeof(float) * lights.size(), alignof(float));
		const Vec4f* spheres = lights.data<LightsIdx::POSITION_RADIUS>();
		for (soul_size i = 0; i < lights.size(); i++)
		{
			Vec4f sphere = spheres[i];
			Vec4f center = cameraInfo.view * Vec4f(sphere.xyz, 1.0f);
			distances[i] = length(center);
		}

		soul_fila::Zip2Iterator<Lights::iterator, float*> b = { lights.begin(), distances };
		std::sort(b + Scene::DIRECTIONAL_LIGHTS_COUNT, b + lights.size(),
			[](auto const& lhs, auto const& rhs) { return lhs.second < rhs.second; });
	}

	static void ComputeVisibilityMasks(
		uint8 visibleLayers,
		uint8 const* SOUL_RESTRICT layers,
		Visibility const* SOUL_RESTRICT visibility,
		uint8* SOUL_RESTRICT visibleMask, soul_size count) {

		for (size_t i = 0; i < count; ++i) {
			uint8 mask = visibleMask[i];
			Visibility v = visibility[i];
			bool inVisibleLayer = layers[i] & visibleLayers;
			

			const bool visRenderables = (!v.culling || (mask & VISIBLE_RENDERABLE)) && inVisibleLayer;
			const bool visShadowParticipant = v.castShadows;
			const bool visShadowRenderable = (!v.culling || (mask & VISIBLE_DIR_SHADOW_RENDERABLE))
				&& inVisibleLayer && visShadowParticipant;
			visibleMask[i] = uint8(visRenderables) |
				uint8(visShadowRenderable << VISIBLE_DIR_SHADOW_RENDERABLE_BIT);
			// this loop gets fully unrolled
			for (size_t j = 0; j < CONFIG_MAX_SHADOW_CASTING_SPOTS; ++j) {
				const bool visSpotShadowRenderable =
					(!v.culling || (mask & VISIBLE_SPOT_SHADOW_RENDERABLE_N(j))) &&
					inVisibleLayer && visShadowParticipant;
				visibleMask[i] |=
					uint8(visSpotShadowRenderable << VISIBLE_SPOT_SHADOW_RENDERABLE_N_BIT(j));
			}
		}
	}
	

	void Renderer::prepare_render_data()
	{
		const IBL& ibl = scene_.get_ibl();
		Mat3f inverse_ibl_rot = mat3Transpose(ibl.rotation);
		Mat4f world_origin_transform = mat4FromMat3(inverse_ibl_rot);

		render_data_.clear();
		Renderables& renderables = render_data_.renderables;
		renderables.setCapacity(scene_.get_renderable_count() + 1);
		scene_.for_each_renderable([&renderables, world_origin_transform, this](EntityID entityID, const TransformComponent& transformComp, const RenderComponent& renderComp)
		{
			soul::Mat4f worldTransform = world_origin_transform * transformComp.world;
			float scale = (length(transformComp.world.columns(0).xyz) + length(transformComp.world.columns(1).xyz) + length(transformComp.world.columns(2).xyz)) / 3.0f;
			const bool reversedWindingOrder = determinant(mat3UpperLeft(worldTransform)) < 0.0f;

			const Mesh& mesh = this->scene_.meshes()[renderComp.meshID.id];
			AABB worldAABB = aabbTransform(mesh.aabb, worldTransform);
			Vec3f worldAABBCenter = (worldAABB.min + worldAABB.max) / 2.0f;
			Vec3f worldAABBHalfExtent = (worldAABB.max - worldAABB.min) / 2.0f;

			renderables.push_back_unsafe(
				entityID,
				worldTransform,
				reversedWindingOrder,
				renderComp.visibility,
				renderComp.skinID,
				worldAABBCenter,
				0,
				renderComp.morphWeights,
				renderComp.layer,
				worldAABBHalfExtent,
				&mesh.primitives,
				0,
				scale
			);
		});

		Lights& lights = render_data_.lights;
		lights.setCapacity(scene_.get_light_count());
		// we only store 1 directional light with the maximum intensity
		lights.resize(Scene::DIRECTIONAL_LIGHTS_COUNT);
		float maxIntensity = 0.0f;
		scene_.for_each_light([&lights, world_origin_transform, &maxIntensity, this](EntityID entityID, const TransformComponent& transformComp, const LightComponent& lightComp)
		{
			soul::Mat4f worldTransform = world_origin_transform * transformComp.world;
			if (scene_.is_directional_light(entityID))
			{
				if (lightComp.intensity >= maxIntensity)
				{
					maxIntensity = lightComp.intensity;
					Vec3f direction = lightComp.direction;
					direction = unit(cofactor(mat3UpperLeft(worldTransform)) * direction);
					lights.elementAt<LightsIdx::POSITION_RADIUS>(0) = Vec4f(0.0f, 0.0f, 0.0f, std::numeric_limits<float>::infinity());
					lights.elementAt<LightsIdx::DIRECTION>(0) = direction;
					lights.elementAt<LightsIdx::ENTITY_ID>(0) = entityID;
				}
			}
			else
			{
				Vec4f worldPosition = worldTransform * Vec4f(lightComp.position, 1.0f);
				Vec3f direction;

				if (lightComp.lightType.type != LightRadiationType::POINT)
				{
					direction = lightComp.direction;
					direction = unit(cofactor(mat3UpperLeft(worldTransform)) * direction);
				}

				lights.push_back_unsafe(
					Vec4f(worldPosition.xyz, lightComp.spotParams.radius),
					direction,
					entityID,
					{},
					{},
					{}
				);
			}
		});

		const CameraInfo& cameraInfo = scene_.get_active_camera(world_origin_transform);
		render_data_.cameraInfo = cameraInfo;

		Frustum cameraFrustum = cameraInfo.get_culling_frustum();
		Cull(render_data_.renderables, cameraFrustum, VISIBLE_RENDERABLE_BIT);
		Cull(scene_, lights, cameraFrustum);
		ComputeCameraDistanceAndSort(scene_, cameraInfo, lights);
		auto shadowFlags = shadow_map_pass_.prepare(scene_, cameraInfo, renderables, lights, render_data_.frameUBO);

		/*
		 * Partition the SoA so that renderables are partitioned w.r.t their visibility into the
		 * following groups:
		 *
		 * 1. renderables
		 * 2. renderables and directional shadow casters
		 * 3. directional shadow casters only
		 * 4. punctual light shadow casters only
		 * 5. invisible renderables
		 *
		 * Note that the first three groups are partitioned based only on the lowest two bits of the
		 * VISIBLE_MASK (VISIBLE_RENDERABLE and VISIBLE_DIR_SHADOW_CASTER), and thus can also
		 * contain punctual light shadow casters as well. The fourth group contains *only* punctual
		 * shadow casters.
		 *
		 * This operation is somewhat heavy as it sorts the whole SoA. We use std::partition instead
		 * of sort(), which gives us O(4.N) instead of O(N.log(N)) application of swap().
		 */

		 // calculate the sorting key for all elements, based on their visibility
		uint8 const* layers = renderables.data<RenderablesIdx::LAYERS>();
		auto const* visibilities = renderables.data<RenderablesIdx::VISIBILITY_STATE>();
		uint8* visibleMask = renderables.data<RenderablesIdx::VISIBLE_MASK>();

		ComputeVisibilityMasks(scene_.get_visible_layers(), layers, visibilities, visibleMask,
			renderables.size());

		auto const beginRenderables = renderables.begin();

		auto partition = [](Renderables::iterator begin, Renderables::iterator end, uint8 mask)
		{
			return std::partition(begin, end, [mask](auto it) {
				// Mask VISIBLE_MASK to ignore higher bits related to spot shadows. We only partition based
				// on renderable and directional shadow visibility.
				return (it.template get<RenderablesIdx::VISIBLE_MASK>() &
					(VISIBLE_RENDERABLE | VISIBLE_DIR_SHADOW_RENDERABLE)) == mask;
				});
		};

		auto beginCasters = partition(beginRenderables, renderables.end(), VISIBLE_RENDERABLE);
		auto beginCastersOnly = partition(beginCasters, renderables.end(),
			VISIBLE_RENDERABLE | VISIBLE_DIR_SHADOW_RENDERABLE);
		auto beginSpotLightCastersOnly = partition(beginCastersOnly, renderables.end(),
			VISIBLE_DIR_SHADOW_RENDERABLE);
		auto endSpotLightCastersOnly = std::partition(beginSpotLightCastersOnly,
			renderables.end(), [](auto it) {
				return (it.template get<RenderablesIdx::VISIBLE_MASK>() & VISIBLE_SPOT_SHADOW_RENDERABLE);
			});

		// convert to indices
		uint32_t iEnd = uint32_t(beginSpotLightCastersOnly - beginRenderables);
		uint32_t iSpotLightCastersEnd = uint32_t(endSpotLightCastersOnly - beginRenderables);
		render_data_.visibleRenderables = Range(0u, uint32(beginCastersOnly - beginRenderables));
		render_data_.directionalShadowCasters = Range(uint32(beginCasters - beginRenderables), iEnd);
		render_data_.spotLightShadowCasters = Range(0u, iSpotLightCastersEnd);
		render_data_.merged = Range(0u, iSpotLightCastersEnd);

		SOUL_ASSERT(0, render_data_.merged.size() != 0, "");

		FrameUBO& frameUBO = render_data_.frameUBO;
		frameUBO.viewFromWorldMatrix = cameraInfo.view;
		frameUBO.worldFromViewMatrix = cameraInfo.model;
		frameUBO.clipFromViewMatrix = cameraInfo.projection;
		frameUBO.viewFromClipMatrix = mat4Inverse(cameraInfo.projection);
		frameUBO.clipFromWorldMatrix = cameraInfo.projection * cameraInfo.view;
		frameUBO.worldFromClipMatrix = cameraInfo.model * mat4Inverse(cameraInfo.projection);
		frameUBO.cameraPosition = cameraInfo.get_position();
		frameUBO.worldOffset = cameraInfo.worldOffset;
		frameUBO.cameraFar = cameraInfo.zf;
		frameUBO.clipControl = soul::Vec2f(-0.5f, 0.5f);
		const float exposure = Exposure::exposure(cameraInfo.ev100);
		frameUBO.exposure = exposure;
		frameUBO.ev100 = cameraInfo.ev100;

		// lighting
		frameUBO.iblLuminance = ibl.intensity * exposure;
		frameUBO.iblRoughnessOneLevel = float(gpu_system_->get_texture_mip_levels(ibl.reflectionTex) - 1);
		std::transform(ibl.irradianceCoefs, ibl.irradianceCoefs + 9, frameUBO.iblSH, [](Vec3f v)
		{
			return Vec4f(v, 0.0f);
		});
		EntityID dirLightEntity = lights.elementAt<LightsIdx::ENTITY_ID>(0);
		if (dirLightEntity != ENTITY_ID_NULL)
		{
			const Vec3f l = -lights.elementAt<LightsIdx::DIRECTION>(0); // guaranteed normalized
			const LightComponent& lightComp = scene_.get_light_component(dirLightEntity);
			const Vec4f colorIntensity = {
					lightComp.color, lightComp.intensity * exposure };

			frameUBO.lightDirection = l;
			frameUBO.lightColorIntensity = colorIntensity;

			const bool isSun = lightComp.lightType.type == LightRadiationType::SUN;
			// The last parameter must be < 0.0f for regular directional lights
			Vec4f sun{ 0.0f, 0.0f, 0.0f, -1.0f };
			if (SOUL_UNLIKELY(isSun && colorIntensity.w > 0.0f)) {
				// currently we have only a single directional light, so it's probably likely that it's
				// also the Sun. However, conceptually, most directional lights won't be sun lights.
				float radius = lightComp.sunAngularRadius;
				float haloSize = lightComp.sunHaloSize;
				float haloFalloff = lightComp.sunHaloFalloff;
				sun.x = std::cos(radius);
				sun.y = std::sin(radius);
				sun.z = 1.0f / (std::cos(radius * haloSize) - sun.x);
				sun.w = haloFalloff;
			}
			frameUBO.sun = sun;
		} else
		{
			// Disable the sun if there's no directional light
			frameUBO.sun = Vec4f(0.0f, 0.0f, 0.0f, -1.0f);
		}

		// viewport
		Vec2ui32 viewport = scene_.get_viewport();
		frameUBO.resolution = Vec4f(viewport.x, viewport.y, 1.0f / viewport.x, 1.0f / viewport.y);
		frameUBO.origin = Vec2f();

		// Fog
		auto const& fogOptions = scene_.get_fog_options();	

		// this can't be too high because we need density / heightFalloff to produce something
		// close to fogOptions.density in the fragment shader which use 16-bits floats.
		constexpr float epsilon = 0.001f;
		const float heightFalloff = std::max(epsilon, fogOptions.heightFalloff);

		// precalculate the constant part of density  integral and correct for exp2() in the shader
		const float density = ((fogOptions.density / heightFalloff) *
			std::exp(-heightFalloff * (cameraInfo.get_position().y - fogOptions.height)))
			* float(1.0f / soul::Fconst::LN2);

		frameUBO.fogStart = fogOptions.distance;
		frameUBO.fogMaxOpacity = fogOptions.maximumOpacity;
		frameUBO.fogHeight = fogOptions.height;
		frameUBO.fogHeightFalloff = heightFalloff;
		frameUBO.fogColor = fogOptions.color;
		frameUBO.fogDensity = density;
		frameUBO.fogInscatteringStart = fogOptions.inScatteringStart;
		frameUBO.fogInscatteringSize = fogOptions.inScatteringSize;
		frameUBO.fogColorFromIbl = fogOptions.fogColorFromIbl ? 1.0f : 0.0f;

		// time
		const clock::time_point now{ clock::now() };
		const uint64_t oneSecondRemainder = (now - epoch).count() % 1000000000;
		const float fraction = float(double(oneSecondRemainder) / 1000000000.0);
		frameUBO.time = fraction;
		std::chrono::duration<double> time(now - epoch);
		float h = float(time.count());
		float l = float(time.count() - h);
		frameUBO.userTime = { h, l, 0, 0}; //TODO(kevinyu) : figure out if this is actually used in the shader, we use current timestamp instead of appVSync that is implemented in filament

		render_data_.lightsUBO = {};

		Array<MaterialUBO>& materialUBOs = render_data_.materialUBOs;
		materialUBOs.reserve(scene_.materials().size());
		for (const Material& material : scene_.materials())
		{
			materialUBOs.add(material.buffer);
		}

		Array<PerRenderableUBO>& renderableUBOs = render_data_.renderableUBOs;
		renderableUBOs.reserve(render_data_.merged.size());
		for (soul_size i : render_data_.merged)
		{
			const Mat4f& model = renderables.elementAt<RenderablesIdx::WORLD_TRANSOFRM>(i);

			soul::Mat3f m = cofactor(mat3UpperLeft(model));
			soul::Mat3f mTranspose = mat3Transpose(m);

			float mFactor = 1.0f / std::sqrt(std::max({ squareLength(mTranspose.rows[0]), squareLength(mTranspose.rows[1]), squareLength(mTranspose.rows[2]) }));

			Mat3f mIdentityFactor;
			mIdentityFactor.elem[0][0] = mFactor;
			mIdentityFactor.elem[1][1] = mFactor;
			mIdentityFactor.elem[2][2] = mFactor;

			m *= mIdentityFactor;

			PerRenderableUBO renderableUBO;
			const Visibility& visibility = renderables.elementAt<RenderablesIdx::VISIBILITY_STATE>(i);
			renderableUBO.worldFromModelMatrix = model;
			renderableUBO.worldFromModelNormalMatrix = soul::GLSLMat3f(m);
			renderableUBO.skinningEnabled = visibility.skinning;
			renderableUBO.morphingEnabled = visibility.morphing;
			renderableUBO.screenSpaceContactShadows = visibility.screenSpaceContactShadows;
			renderableUBO.morphWeights = renderables.elementAt<RenderablesIdx::MORPH_WEIGHTS>(i);
			renderableUBO.userData = renderables.elementAt<RenderablesIdx::USER_DATA>(i);
			renderableUBOs.add(renderableUBO);
		}

		soul::Array<BonesUBO>& bonesUBOs = render_data_.bonesUBOs;
		const Array<Skin>& skins = scene_.skins();
		bonesUBOs.resize(skins.size());
		for (uint64 skinIdx = 0; skinIdx < skins.size(); skinIdx++) {
			const Skin& skin = skins[skinIdx];
			std::copy(skin.bones.data(), skin.bones.data() + skin.bones.size(), bonesUBOs[skinIdx].bones);
		}
		if (bonesUBOs.size() == 0) {
			bonesUBOs.add(BonesUBO());
		}

		uint32 count = 0;
		uint32* summedPrimitiveCounts = renderables.data<RenderablesIdx::SUMMED_PRIMITIVE_COUNT>();
		const Array<Primitive>** primitives = renderables.data<RenderablesIdx::PRIMITIVES>();
		for (soul_size i : render_data_.merged)
		{
			summedPrimitiveCounts[i] = count;
			count += primitives[i]->size();
		}
		summedPrimitiveCounts[render_data_.merged.last] = count;
			
		if (shadowFlags) { render_data_.flags |= HAS_SHADOWING; }
		if (dirLightEntity != ENTITY_ID_NULL) { render_data_.flags |= HAS_DIRECTIONAL_LIGHT; }
		if (lights.size() > Scene::DIRECTIONAL_LIGHTS_COUNT) { render_data_.flags |= HAS_DYNAMIC_LIGHTING; }
		if (shadow_map_pass_.getShadowType() == ShadowType::VSM) { render_data_.flags |= HAS_VSM; }
		if (fogOptions.enabled && fogOptions.density > 0.0f) { render_data_.flags |= HAS_FOG; }

	}

	void Renderer::init() {

		const gpu::ClearValue clear_value(Vec4f(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 0);
		const gpu::TextureDesc stub_texture_2d_desc = gpu::TextureDesc::D2(
			"Stub texture",
			gpu::TextureFormat::RGBA8,
			1,
			{ gpu::TextureUsage::SAMPLED },
			{ gpu::QueueType::GRAPHIC },
			{ 1, 1 });
		render_data_.stubTexture = gpu_system_->create_texture(stub_texture_2d_desc, clear_value);
		gpu_system_->finalize_texture(render_data_.stubTexture, { gpu::TextureUsage::SAMPLED });

		const gpu::ClearValue clear_value_uint(Vec4ui32(0, 0, 0, 0), 0, 0);
		const gpu::TextureDesc stub_texture_2d_uint_desc = gpu::TextureDesc::D2(
			"Stub texture Uint",
			gpu::TextureFormat::RG16UI,
			1,
			{ gpu::TextureUsage::SAMPLED },
			{ gpu::QueueType::GRAPHIC },
			{ 1, 1 });
		render_data_.stubTextureUint = gpu_system_->create_texture(stub_texture_2d_uint_desc, clear_value_uint);
		gpu_system_->finalize_texture(render_data_.stubTextureUint, { gpu::TextureUsage::SAMPLED });

		gpu::TextureDesc stub_texture_array_desc = gpu::TextureDesc::D2Array(
			"Stub texture array",
			gpu::TextureFormat::RGBA8,
			1,
			{ gpu::TextureUsage::SAMPLED },
			{ gpu::QueueType::GRAPHIC },
			{ 1, 1 }, 1);
		render_data_.stubTextureArray = gpu_system_->create_texture(stub_texture_array_desc, clear_value);
		gpu_system_->finalize_texture(render_data_.stubTextureArray, { gpu::TextureUsage::SAMPLED });

		static constexpr Vec2f FULL_SCREEN_TRIANGLE_VERTICES[] = {
			{ -1.0f, -1.0f },
			{  -1.0f, 1.0f },
			{ 1.0f,  -1.0f },
			{ 1.0f, 1.0f }
		};

		static constexpr uint32 FULLSCREEN_INDICES[] = {
			2, 1, 0,
			3, 1, 2
		};

		render_data_.fullscreenVb = gpu_system_->create_buffer({
			.count = std::size(FULL_SCREEN_TRIANGLE_VERTICES),
			.typeSize = sizeof(Vec2f),
			.typeAlignment = alignof(Vec2f),
			.usageFlags = {gpu::BufferUsage::VERTEX},
			.queueFlags = {gpu::QueueType::GRAPHIC}
			}, FULL_SCREEN_TRIANGLE_VERTICES);
		gpu_system_->finalize_buffer(render_data_.fullscreenVb);

		render_data_.fullscreenIb = gpu_system_->create_buffer({
			.count = std::size(FULLSCREEN_INDICES),
			.typeSize = sizeof(uint32),
			.typeAlignment = alignof(uint32),
			.usageFlags = {gpu::BufferUsage::INDEX},
			.queueFlags = {gpu::QueueType::GRAPHIC}
			}, FULLSCREEN_INDICES);
		gpu_system_->finalize_buffer(render_data_.fullscreenIb);

		lighting_pass_.init(gpu_system_, &program_registry_);
		structure_pass_.init(gpu_system_, &program_registry_);
		shadow_map_pass_.init(gpu_system_, &program_registry_);
		depth_mipmap_pass_.init(gpu_system_);
	}

	gpu::TextureNodeID Renderer::computeRenderGraph(gpu::RenderGraph* renderGraph) {

		// TODO: Should remove this in the future. make this function run even when there is no object or use better check
		if (scene_.meshes().size() == 0) {
			return gpu::TextureNodeID();
		}

		prepare_render_data();
		runtime::ScopeAllocator<> scopeAllocator("computeRenderGraph");
		
		soul::gpu::BufferDesc frame_ubo_desc;
		frame_ubo_desc.typeSize = sizeof(FrameUBO);
		frame_ubo_desc.typeAlignment = alignof(FrameUBO);
		frame_ubo_desc.count = 1;
		frame_ubo_desc.usageFlags = {gpu:: BufferUsage::UNIFORM };
		frame_ubo_desc.queueFlags = { gpu::QueueType::GRAPHIC };
		soul::gpu::BufferID frame_gpu_buffer = gpu_system_->create_buffer(frame_ubo_desc, &render_data_.frameUBO);
		gpu_system_->destroy_buffer(frame_gpu_buffer);
		soul::gpu::BufferNodeID frame_uniform_buffer = renderGraph->import_buffer("Frame Uniform Buffer", frame_gpu_buffer);

		soul::gpu::BufferDesc light_ubo_desc;
		light_ubo_desc.typeSize = sizeof(LightsUBO);
		light_ubo_desc.typeAlignment = alignof(LightsUBO);
		light_ubo_desc.count = 1;
		light_ubo_desc.usageFlags = {gpu:: BufferUsage::UNIFORM };
		light_ubo_desc.queueFlags = { gpu::QueueType::GRAPHIC };
		soul::gpu::BufferID light_gpu_buffer = gpu_system_->create_buffer(light_ubo_desc, &render_data_.lightsUBO);
		gpu_system_->destroy_buffer(light_gpu_buffer);
		soul::gpu::BufferNodeID light_uniform_buffer = renderGraph->import_buffer("Light Uniform Buffer", light_gpu_buffer);

		soul::gpu::BufferDesc shadow_ubo_desc;
		shadow_ubo_desc.typeSize = sizeof(ShadowUBO);
		shadow_ubo_desc.typeAlignment = alignof(ShadowUBO);
		shadow_ubo_desc.count = 1;
		shadow_ubo_desc.usageFlags = {gpu:: BufferUsage::UNIFORM };
		shadow_ubo_desc.queueFlags = { gpu::QueueType::GRAPHIC };
		soul::gpu::BufferID shadow_gpu_buffer = gpu_system_->create_buffer(shadow_ubo_desc, &render_data_.shadowUBO);
		gpu_system_->destroy_buffer(shadow_gpu_buffer);
		soul::gpu::BufferNodeID shadowUniformBuffer = renderGraph->import_buffer("Shadow Uniform Buffer", shadow_gpu_buffer);

		soul::gpu::BufferDesc froxel_records_buffer_desc;
		froxel_records_buffer_desc.typeSize = sizeof(FroxelRecordsUBO);
		froxel_records_buffer_desc.typeAlignment = alignof(FroxelRecordsUBO);
		froxel_records_buffer_desc.count = 1;
		froxel_records_buffer_desc.usageFlags = {gpu:: BufferUsage::UNIFORM };
		froxel_records_buffer_desc.queueFlags = { gpu::QueueType::GRAPHIC };
		FroxelRecordsUBO froxel_records_ubo = {};
		soul::gpu::BufferID froxel_records_gpu_buffer = gpu_system_->create_buffer(froxel_records_buffer_desc, &froxel_records_ubo);
		gpu_system_->destroy_buffer(froxel_records_gpu_buffer);
		soul::gpu::BufferNodeID froxel_records_uniform_buffer = renderGraph->import_buffer("Froxel Records Uniform Buffer", froxel_records_gpu_buffer);

		soul::gpu::BufferDesc material_ubo_desc;
		material_ubo_desc.typeSize = sizeof(MaterialUBO);
		material_ubo_desc.typeAlignment = alignof(MaterialUBO);
		material_ubo_desc.count = scene_.materials().size();
		material_ubo_desc.usageFlags = {gpu:: BufferUsage::UNIFORM };
		material_ubo_desc.queueFlags = { gpu::QueueType::GRAPHIC };
		soul::gpu::BufferID material_gpu_buffer = gpu_system_->create_buffer(material_ubo_desc,render_data_.materialUBOs.data());
		gpu_system_->destroy_buffer(material_gpu_buffer);
		soul::gpu::BufferNodeID material_uniform_buffer = renderGraph->import_buffer("Material Uniform Buffer", material_gpu_buffer);


		soul::gpu::BufferDesc renderable_ubo_desc;
		renderable_ubo_desc.typeSize = sizeof(PerRenderableUBO);
		renderable_ubo_desc.typeAlignment = alignof(PerRenderableUBO);
		renderable_ubo_desc.count = render_data_.renderableUBOs.size();
		renderable_ubo_desc.usageFlags = {gpu:: BufferUsage::UNIFORM };
		renderable_ubo_desc.queueFlags = { gpu::QueueType::GRAPHIC };
			soul::gpu::BufferID renderable_gpu_buffer = gpu_system_->create_buffer(renderable_ubo_desc, render_data_.renderableUBOs.data());
		gpu_system_->destroy_buffer(renderable_gpu_buffer);
		soul::gpu::BufferNodeID object_uniform_buffer = renderGraph->import_buffer("Renderable Uniform Buffer", renderable_gpu_buffer);


		soul::gpu::BufferDesc bones_ubo_desc;
		bones_ubo_desc.typeSize = sizeof(BonesUBO);
		bones_ubo_desc.typeAlignment = alignof(BonesUBO);
		bones_ubo_desc.count = render_data_.bonesUBOs.size();
		bones_ubo_desc.usageFlags = {gpu:: BufferUsage::UNIFORM };
		bones_ubo_desc.queueFlags = { gpu::QueueType::GRAPHIC };
		soul::gpu::BufferID bones_ubogpu_buffer = gpu_system_->create_buffer(bones_ubo_desc, render_data_.bonesUBOs.data());
		gpu_system_->destroy_buffer(bones_ubogpu_buffer);
		soul::gpu::BufferNodeID bone_uniform_buffer = renderGraph->import_buffer("Bones Uniform Buffer", bones_ubogpu_buffer);


		ShadowMapGenPass::Input shadow_map_input;
		shadow_map_input.objectsUb = object_uniform_buffer;
		shadow_map_input.bonesUb = bone_uniform_buffer;
		shadow_map_input.materialsUb = material_uniform_buffer;
		ShadowMapGenPass::Output shadow_map_output = shadow_map_pass_.computeRenderGraph(*renderGraph, shadow_map_input, render_data_, scene_);


		StructurePass::Input structure_input;
		structure_input.frameUb = frame_uniform_buffer;
		structure_input.objectsUb = object_uniform_buffer;
		structure_input.bonesUb = bone_uniform_buffer;
		structure_input.materialsUb = material_uniform_buffer;
		StructurePass::Output structure_output = structure_pass_.computeRenderGraph(*renderGraph, structure_input, render_data_, scene_);

		DepthMipmapPass::Output depth_mipmap_output = depth_mipmap_pass_.computeRenderGraph(*renderGraph, { structure_output.depthTarget }, render_data_, scene_);

		LightingPass::Input lighting_input;
		lighting_input.frameUb = frame_uniform_buffer;
		lighting_input.lightsUb = light_uniform_buffer;
		lighting_input.shadowUb = shadowUniformBuffer;
		lighting_input.froxelRecrodUb = froxel_records_uniform_buffer;
		lighting_input.objectsUb = object_uniform_buffer;
		lighting_input.bonesUb = bone_uniform_buffer;
		lighting_input.materialsUb = material_uniform_buffer;
		lighting_input.structureTex = depth_mipmap_output.depthMap;
		lighting_input.shadowMap = shadow_map_output.depthTarget;
		LightingPass::Output lighting_output = lighting_pass_.computeRenderGraph(*renderGraph, lighting_input, render_data_, scene_);
		
		return lighting_output.renderTarget;
	}
}