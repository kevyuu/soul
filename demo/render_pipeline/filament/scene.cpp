#include "data.h"
#include "gltf_importer.h"
#include "../../camera_manipulator.h"
#include "../../ktx_bundle.h"

#include "core/math.h"
#include "core/geometry.h"
#include "core/enum_array.h"
#include "imgui/imgui.h"
#include "../../ui/widget.h"

#include "gpu_program_registry.h"

#include "runtime/scope_allocator.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

constexpr ImGuiTreeNodeFlags SCENE_TREE_FLAGS = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
static constexpr float IBL_INTENSITY = 30000.0f;

void item_rows_background(float line_height = -1.0f, const ImColor& color = ImColor(20, 20, 20, 64))
{
    auto* draw_list = ImGui::GetWindowDrawList();
    const auto& style = ImGui::GetStyle();

    if (line_height < 0)
    {
        line_height = ImGui::GetTextLineHeight();
    }
    line_height += style.ItemSpacing.y;

    const float scroll_offset_h = ImGui::GetScrollX();
    const float scrolled_out_lines = floorf(ImGui::GetScrollY() / line_height);
    const float scroll_offset_v = ImGui::GetScrollY() - line_height * scrolled_out_lines;

    const ImVec2 clip_rect_min(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);

	ImVec2 clip_rect_max(clip_rect_min.x + ImGui::GetWindowWidth(), clip_rect_min.y + ImGui::GetWindowHeight());
    if (ImGui::GetScrollMaxX() > 0)
    {
        clip_rect_max.y -= style.ScrollbarSize;
    }

    draw_list->PushClipRect(clip_rect_min, clip_rect_max);

    bool is_odd = (static_cast<int>(scrolled_out_lines) % 2) == 0;

    float y_min = clip_rect_min.y - scroll_offset_v + ImGui::GetCursorPosY();
    float y_max = clip_rect_max.y - scroll_offset_v + line_height;
    float x_min = clip_rect_min.x + scroll_offset_h + ImGui::GetWindowContentRegionMin().x;
    float x_max = clip_rect_min.x + scroll_offset_h + ImGui::GetWindowContentRegionMax().x;

    for (float y = y_min; y < y_max; y += line_height, is_odd = !is_odd)
    {
        if (is_odd)
        {
            draw_list->AddRectFilled({ x_min, y - style.ItemSpacing.y }, { x_max, y + line_height }, color);
        }
    }

    draw_list->PopClipRect();
}

static soul::Vec4f compute_ibl_color_estimate(soul::Vec3f const* Le, soul::Vec3f direction) noexcept
{
    using namespace soul;
    // See: https://www.gamasutra.com/view/news/129689/Indepth_Extracting_dominant_light_from_Spherical_Harmonics.php

    // note Le is our pre-convolved, pre-scaled SH coefficients for the environment

    // first get the direction
    const Vec3f s = -direction;

    // The light intensity on one channel is given by: dot(Ld, Le) / dot(Ld, Ld)

    // SH coefficients of the directional light pre-scaled by 1/A[i]
    // (we pre-scale by 1/A[i] to undo Le's pre-scaling by A[i]
    const float ld[9] = {
            1.0f,
            s.y, s.z, s.x,
            s.y * s.x, s.y * s.z, (3 * s.z * s.z - 1), s.z * s.x, (s.x * s.x - s.y * s.y)
    };

    // dot(Ld, Le) -- notice that this is equivalent to "sampling" the sphere in the light
    // direction; this is the exact same code used in the shader for SH reconstruction.
    Vec3f ld_dot_le = ld[0] * Le[0]
        + ld[1] * Le[1] + ld[2] * Le[2] + ld[3] * Le[3]
        + ld[4] * Le[4] + ld[5] * Le[5] + ld[6] * Le[6] + ld[7] * Le[7] + ld[8] * Le[8];

    // The scale factor below is explained in the gamasutra article above, however it seems
    // to cause the intensity of the light to be too low.
    //      constexpr float c = (16.0f * F_PI / 17.0f);
    //      constexpr float LdSquared = (9.0f / (4.0f * F_PI)) * c * c;
    //      LdDotLe *= c / LdSquared; // Note the final coefficient is 17/36

    // We multiply by PI because our SH coefficients contain the 1/PI lambertian BRDF.
    ld_dot_le *= soul::Fconst::PI;

    // Make sure we don't have negative intensities
    ld_dot_le = componentMax(ld_dot_le, Vec3f());

    const float intensity = *std::max_element(ld_dot_le.mem, ld_dot_le.mem + 3);
    return { ld_dot_le / intensity, intensity };
}

static void make_bone(soul_fila::BoneUBO* out, const soul::Mat4f& t) {
    soul::Mat4f m = mat4Transpose(t);

    // figure out the scales
    soul::Vec4f s = { length(m.rows[0]), length(m.rows[1]), length(m.rows[2]), 0.0f };
    if (dot(cross(m.rows[0].xyz, m.rows[1].xyz), m.rows[2].xyz) < 0) {
        s.mem[2] = -s.mem[2];
    }

    // compute the inverse scales
    const soul::Vec4f is = { 1.0f / s.x, 1.0f / s.y, 1.0f / s.z, 0.0f };

    // normalize the matrix
    m.rows[0] *= is.mem[0];
    m.rows[1] *= is.mem[1];
    m.rows[2] *= is.mem[2];

    out->s = s;
    out->q = quaternionFromMat4(mat4Transpose(m));
    out->t = m.rows[3];
    out->ns = is / std::max({ abs(is.x), abs(is.y), abs(is.z), abs(is.w) });
}


namespace soul_fila {

    const uint16 DFG::LUT[] = {
		#include "dfg.inc"
    };
}

soul_fila::Scene::Scene(soul::gpu::System* gpuSystem, GPUProgramRegistry* programRegistry) :
    gpu_system_(gpuSystem), program_registry_(programRegistry),
    camera_man_({ 0.1f, 0.001f, soul::Vec3f(0.0f, 1.0f, 0.0f) })
{
    create_root_entity();
}

void soul_fila::Scene::import_from_gltf(const char* path)
{
    SOUL_ASSERT(1, check_resources_validity(), "");
    GLTFImporter importer(path, *gpu_system_, *program_registry_, *this);
    importer.import();
    SOUL_ASSERT(1, check_resources_validity(), "");
}

bool soul_fila::Scene::check_resources_validity()
{
	for (const Texture& texture : textures_)
	{
        if (texture.gpuHandle.is_null()) return false;
	}
    for (const Mesh& mesh : meshes_)
    {
	    for (const Primitive& primitive : mesh.primitives)
	    {
            if (primitive.indexBuffer.is_null()) return false;
	    }
    }
    return true;
}

void soul_fila::Scene::create_root_entity()
{
    root_entity_ = registry_.create();
    registry_.emplace<NameComponent>(root_entity_, "Root");
    registry_.emplace<TransformComponent>(root_entity_, mat4Identity(), mat4Identity(), root_entity_, ENTITY_ID_NULL, ENTITY_ID_NULL, ENTITY_ID_NULL);
}

void soul_fila::Scene::create_default_sunlight()
{
    constexpr auto sunlight_direction = Vec3f(0.723f, -0.688f, -0.062f);
    const Vec4f c = compute_ibl_color_estimate(ibl_.mBands, sunlight_direction);
    const Vec3f sunlight_color = c.xyz;
    const float sunlight_intensity = c.w * ibl_.intensity;
    const LightDesc light_desc = {
        .type = LightType(LightRadiationType::SUN, true, false),
        .linearColor = sunlight_color,
        .intensity = sunlight_intensity,
        .direction = unit(sunlight_direction),
        .sunAngle = 1.9f,
        .sunHaloSize = 10.0f,
        .sunHaloFalloff = 80.0f
    };
    create_light(light_desc);
}

void soul_fila::Scene::create_default_camera()
{
    default_camera_ = registry_.create();

    soul::Mat4f camera_model_mat = soul::mat4Inverse(soul::mat4View(soul::Vec3f(-0.557f, 0.204f, -3.911f), soul::Vec3f(0, 0, -4), soul::Vec3f(0, 1, 0)));
    registry_.emplace<TransformComponent>(default_camera_, camera_model_mat, camera_model_mat, root_entity_, ENTITY_ID_NULL, ENTITY_ID_NULL, ENTITY_ID_NULL);
    CameraComponent& camera_comp = registry_.emplace<CameraComponent>(default_camera_);
    Vec2ui32 viewport = get_viewport();
    camera_comp.setLensProjection(28.0f, static_cast<float>(viewport.x) / static_cast<float>(viewport.y), 0.01f, 100.0f);
    registry_.emplace<NameComponent>(default_camera_, "Default camera");
    set_active_camera(default_camera_);
}

void soul_fila::Scene::update_bounding_box()
{
    auto view = registry_.view<TransformComponent, RenderComponent>();
    for (auto entity : view) {
        auto [transform, renderComp] = view.get<TransformComponent, RenderComponent>(entity);
        const Mesh& mesh = meshes_[renderComp.meshID.id];
        bounding_box_ = aabbCombine(bounding_box_, aabbTransform(mesh.aabb, transform.world));
    }
}

void soul_fila::Scene::fit_into_unit_cube()
{
    auto get_fit_unit_cube_transform = [](const AABB& bounds, const float z_offset) -> soul::Mat4f {
        auto bound_min = bounds.min;
        auto bound_max = bounds.max;
        float max_extent = std::max(bound_max.x - bound_min.x, bound_max.y - bound_min.y);
        max_extent = std::max(max_extent, bound_max.z - bound_min.z);
        float scale_factor = 2.0f / max_extent;
        Vec3f center = (bound_min + bound_max) / 2.0f;
        center.z += z_offset / scale_factor;
        return mat4Scale(Vec3f(scale_factor, scale_factor, scale_factor)) * mat4Translate(center * -1.0f);
    };

    const Mat4f fit_transform = get_fit_unit_cube_transform(bounding_box_, 4);
    TransformComponent& root_transform = registry_.get<TransformComponent>(root_entity_);
    root_transform.local = fit_transform;
    update_world_transform(root_entity_);
}

void soul_fila::Scene::create_dfg(const char* path, const char* name)
{
    
    using namespace std;
    ifstream file(path, ios::binary);
    vector<uint8_t> contents((istreambuf_iterator<char>(file)), {});
    image::KtxBundle ktx(contents.data(), soul::cast<uint32>(contents.size()));
    const auto& ktx_info = ktx.getInfo();
    const uint32_t mip_count = ktx.getNumMipLevels();

    SOUL_ASSERT(0, ktx_info.glType == image::KtxBundle::R11F_G11F_B10F, "");

    const gpu::TextureDesc reflection_tex_desc = gpu::TextureDesc::Cube(name, gpu::TextureFormat::R11F_G11F_B10F, mip_count, { gpu::TextureUsage::SAMPLED }, { gpu::QueueType::GRAPHIC }, { ktx_info.pixelWidth, ktx_info.pixelHeight });

    Array<gpu::TextureRegionLoad> reflection_region_loads;
    reflection_region_loads.reserve(mip_count);

    for (uint32 level = 0; level < mip_count; level++)
    {
        uint8* level_data;
        uint32 level_size;
        ktx.getBlob({ level, 0, 0 }, &level_data, &level_size);

        const uint32_t level_width = std::max(1u, ktx_info.pixelWidth >> level);
        const uint32_t level_height = std::max(1u, ktx_info.pixelHeight >> level);
        const gpu::TextureRegionLoad region_load = {
            .bufferOffset = soul::cast<soul_size>(level_data - ktx.getRawData()),
            .textureRegion = {
                .extent = {level_width, level_height, 1},
                .mipLevel = level,
                .layerCount = 6
            }
        };


        reflection_region_loads.add(region_load);
    }

    const gpu::TextureLoadDesc reflection_load_desc = {
	    .data = ktx.getRawData(),
	    .dataSize = ktx.getTotalSize(),
        .regionLoadCount = soul::cast<uint32>(reflection_region_loads.size()),
        .regionLoads = reflection_region_loads.data()
    };

    ibl_.reflectionTex = gpu_system_->create_texture(reflection_tex_desc, reflection_load_desc);
    gpu_system_->finalize_texture(ibl_.reflectionTex, { gpu::TextureUsage::SAMPLED });

    ktx.getSphericalHarmonics(ibl_.mBands);
    ibl_.intensity = IBL_INTENSITY;

    static constexpr soul_size byteCount = DFG::LUT_SIZE * DFG::LUT_SIZE * 3 * sizeof(uint16);
    static_assert(sizeof(DFG::LUT) == byteCount, "DFG_LUT_SIZE doesn't match size of the DFG LUT!");

    const gpu::TextureDesc dfg_tex_desc = gpu::TextureDesc::D2("DFG LUT", gpu::TextureFormat::RGBA16F, 1, { gpu::TextureUsage::SAMPLED }, { gpu::QueueType::GRAPHIC }, { DFG::LUT_SIZE, DFG::LUT_SIZE });

    uint32 reshaped_size = DFG::LUT_SIZE * DFG::LUT_SIZE * 4 * sizeof(uint16);
    auto reshaped_lut = soul::cast<uint16*>(runtime::get_temp_allocator()->allocate(reshaped_size, sizeof(uint16), "DFG LUT"));

    for (soul_size i = 0; i < DFG::LUT_SIZE * DFG::LUT_SIZE; i++)
    {
        reshaped_lut[i * 4] = DFG::LUT[i * 3];
        reshaped_lut[i * 4 + 1] = DFG::LUT[i * 3 + 1];
        reshaped_lut[i * 4 + 2] = DFG::LUT[i * 3 + 2];
        reshaped_lut[i * 4 + 3] = 0x3c00; // 0x3c00 is 1.0 in float16;
    }


    gpu::TextureRegionLoad dfg_tex_region_load = {
        .textureRegion = {
            .extent = { DFG::LUT_SIZE, DFG::LUT_SIZE, 1 },
            .layerCount = 1
        }
    };
    const gpu::TextureLoadDesc dfg_load_desc = {
        .data = reshaped_lut,
		.dataSize = reshaped_size,
		.regionLoadCount = 1,
		.regionLoads = &dfg_tex_region_load
    };

    dfg_.tex = gpu_system_->create_texture(dfg_tex_desc, dfg_load_desc);
    gpu_system_->finalize_texture(dfg_.tex, { gpu::TextureUsage::SAMPLED });

}

void soul_fila::Scene::render_entity_tree_node(EntityID entityID) {
    if (entityID == ENTITY_ID_NULL) return;
    const auto& [transformComp, nameComp] = registry_.get<TransformComponent, NameComponent>(entityID);
    ImGuiTreeNodeFlags flags = SCENE_TREE_FLAGS;
    if (selected_entity_ == entityID) flags |= ImGuiTreeNodeFlags_Selected;
    if (transformComp.firstChild == ENTITY_ID_NULL) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)entityID, flags, "%s", nameComp.name.data());  // NOLINT(performance-no-int-to-ptr)
    if (ImGui::IsItemClicked()) {
        selected_entity_ = entityID;
    }
    if (nodeOpen && transformComp.firstChild != ENTITY_ID_NULL) {
        render_entity_tree_node(transformComp.firstChild);
        ImGui::TreePop();
    }
    render_entity_tree_node(transformComp.next);
}

void soul_fila::Scene::renderPanels() {

    if (ImGui::Begin("Scene configuration")) {
        auto render_camera_list = [this]() {
	        const auto view = registry_.view<CameraComponent, NameComponent>();
            if (const char* comboLabel = active_camera_ == ENTITY_ID_NULL ? "No camera" : registry_.get<NameComponent>(active_camera_).name.data(); 
                ImGui::BeginCombo("Camera List", comboLabel, ImGuiComboFlags_PopupAlignLeft))
            {
                for (const auto entity : view) {
                    const bool isSelected = (active_camera_ == entity);
                    if (ImGui::Selectable(view.get<NameComponent>(entity).name.data(), isSelected)) {
                        set_active_camera(entity);
                    }

                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        };
        render_camera_list();

        auto render_animation_list = [this]() {
            static constexpr char const * NO_ACTIVE_ANIMATION_LABEL = "No active animation";
            if (const char* combo_label = active_animation_.is_null() ? NO_ACTIVE_ANIMATION_LABEL : animations_[active_animation_.id].name.data(); 
                ImGui::BeginCombo("Animation List", combo_label, 0)) {
                for (uint64 anim_idx = 0; anim_idx < animations_.size(); anim_idx++) {
                    const bool is_selected = (active_animation_.id == anim_idx);
                    if (ImGui::Selectable(animations_[anim_idx].name.data(), is_selected)) {
                        set_active_animation(AnimationID(anim_idx));
                    }

                    if (is_selected) ImGui::SetItemDefaultFocus();
                }

                const bool isSelected = (active_animation_.is_null());
                if (ImGui::Selectable(NO_ACTIVE_ANIMATION_LABEL, isSelected)) {
                    set_active_animation(AnimationID());
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
                ImGui::EndCombo();
            }
        };
        render_animation_list();
    }
    ImGui::End();

    if (ImGui::Begin("Scene Tree")) {
        item_rows_background();
        render_entity_tree_node(root_entity_);
    }
    ImGui::End();

    if (ImGui::Begin("Entity Components")) {
        if (selected_entity_ != ENTITY_ID_NULL) {
            auto [transformComp, nameComp] = registry_.get<TransformComponent, NameComponent>(selected_entity_);
            nameComp.renderUI();
            transformComp.renderUI();
        }
    }
    ImGui::End();
}

void soul_fila::Scene::set_active_animation(AnimationID animationID) {
    active_animation_ = animationID;
    animation_delta_ = 0;
    reset_animation_ = true;
    if (animationID.is_null()) return;
    channel_cursors_.resize(animations_[animationID.id].channels.size());
    for (uint64& cursor : channel_cursors_) {
        cursor = 0;
    }
}

void soul_fila::Scene::set_active_camera(EntityID camera) {
    active_camera_ = camera;
    TransformComponent& transformComp = registry_.get<TransformComponent>(camera);
    soul::Vec3f cameraPosition = soul::Vec3f(transformComp.world.elem[0][3], transformComp.world.elem[1][3], transformComp.world.elem[2][3]);
    soul::Vec3f cameraForward = unit(soul::Vec3f(transformComp.world.elem[0][2], transformComp.world.elem[1][2], transformComp.world.elem[2][2]) * -1.0f);
    soul::Vec3f cameraUp = unit(soul::Vec3f(transformComp.world.elem[0][1], transformComp.world.elem[1][1], transformComp.world.elem[2][1]));
    soul::Plane groundPlane(soul::Vec3f(0, 1, 0), soul::Vec3f(0, 0, 0));
    soul::Ray cameraRay(cameraPosition, cameraForward);
	const auto [intersectPoint, isIntersect] = IntersectRayPlane(cameraRay, groundPlane);
    soul::Vec3f cameraTarget = intersectPoint;
    if (!isIntersect)
    {
	    cameraTarget = cameraPosition + 5.0f * cameraForward;
    }
    camera_man_.setCamera(cameraPosition, cameraTarget, cameraUp);
}

bool soul_fila::Scene::update(const Demo::Input& input) {

    auto handleViewPopup = [this](const Demo::Input& input) {
        if (input.keysDown[Demo::Input::KEY_GRAVE_ACCENT]) {
            ImGui::OpenPopup("##ViewPopup");
        }
        enum class CameraViewDirection : uint8 {
            RIGHT,
            BOTTOM,
            FRONT,
            LEFT,
            BACK,
            TOP,
            COUNT
        };
        static const char* VIEW_PIE_ITEMS[] = { "Right", "Bottom", "Front", "Left", "Front", "Top" };
        static auto CAMERA_VIEW_DIR = soul::EnumArray<CameraViewDirection, soul::Vec3f>::build_from_list({
            soul::Vec3f(1.0f, 0.0f, 0.0f),
            soul::Vec3f(0.0f, -1.0f, 0.0f),
            soul::Vec3f(0.0f, 0.0f, 1.0f),
            soul::Vec3f(-1.0f, 0.0f, 0.0f),
            soul::Vec3f(0.0f, 0.0f, -1.0f),
            soul::Vec3f(0.0f, 1.0f, 0.0f)
        });
        static auto CAMERA_UP_DIR = soul::EnumArray<CameraViewDirection, soul::Vec3f>::build_from_list({
            soul::Vec3f(0.0f, 1.0f, 0.0f),
            soul::Vec3f(0.0f, 0.0f, 1.0f),
            soul::Vec3f(0.0f, 1.0f, 0.0f),
            soul::Vec3f(0.0f, 1.0f, 0.0f),
            soul::Vec3f(0.0f, 1.0f, 0.0f),
            soul::Vec3f(0.0f, 0.0f, -1.0f)
        });
        
        int viewPopupSelected = Demo::UI::PiePopupSelectMenu("##ViewPopup", VIEW_PIE_ITEMS, sizeof(VIEW_PIE_ITEMS) / sizeof(*VIEW_PIE_ITEMS), Demo::Input::KEY_GRAVE_ACCENT);
        if (viewPopupSelected >= 0 && selected_entity_ != ENTITY_ID_NULL) {
            TransformComponent& transformComp = registry_.get<TransformComponent>(selected_entity_);
            Transformf transform = transformMat4(transformComp.world);
            soul::Vec3f cameraTarget = transform.position;
            soul::Vec3f cameraPos = transform.position + 5.0f * rotate(transform.rotation, CAMERA_VIEW_DIR[CameraViewDirection(viewPopupSelected)]);
            soul::Vec3f cameraUp = rotate(transform.rotation, CAMERA_UP_DIR[CameraViewDirection(viewPopupSelected)]);
            camera_man_.setCamera(cameraPos, cameraTarget, cameraUp);
        }
    };
    handleViewPopup(input);

    if (!reset_animation_) {
        animation_delta_ += input.deltaTime;
    }
    else {
        reset_animation_ = false;
    }

    auto applyAnimation = [this]() {
        const Animation& activeAnimation = animations_[active_animation_.id];

        if (animation_delta_ > activeAnimation.duration) {
            animation_delta_ = fmod(animation_delta_, activeAnimation.duration);
            for (uint64& cursor : channel_cursors_) {
                cursor = 0;
            }
        }

        for (uint64 channelIdx = 0; channelIdx < activeAnimation.channels.size(); channelIdx++) {
            uint64& cursor = channel_cursors_[channelIdx];

            const AnimationChannel& channel = activeAnimation.channels[channelIdx];
            const AnimationSampler& sampler = activeAnimation.samplers[channel.samplerIdx];

            while (cursor < sampler.times.size() && sampler.times[cursor] < animation_delta_) {
                cursor++;
            }

            float t = 0.0f;
            uint64 prevIndex;
            uint64 nextIndex;
            if (cursor == 0) {
                nextIndex = 0;
                prevIndex = 0;
            }
            else if (cursor == sampler.times.size()) {
                nextIndex = sampler.times.size() - 1;
                prevIndex = nextIndex;
            }
            else {
                nextIndex = cursor;
                prevIndex = cursor - 1;

                float deltaTime = sampler.times[nextIndex] - sampler.times[prevIndex];
                SOUL_ASSERT(0, deltaTime >= 0, "");

                if (deltaTime > 0) {
                    t = (animation_delta_ - sampler.times[prevIndex]) / deltaTime;
                }
            }

            if (sampler.interpolation == AnimationSampler::STEP) {
                t = 0.0f;
            }

            SOUL_ASSERT(0, t <= 1 && t >= 0, "");

            TransformComponent& transformComp = registry_.get<TransformComponent>(channel.entity);
            Transformf transform = transformMat4(transformComp.local);

            switch (channel.transformType) {

            case AnimationChannel::SCALE: {
                const Vec3f* srcVec3 = (const Vec3f*)sampler.values.data();
                if (sampler.interpolation == AnimationSampler::CUBIC) {
                    Vec3f vert0 = srcVec3[prevIndex * 3 + 1];
                    Vec3f tang0 = srcVec3[prevIndex * 3 + 2];
                    Vec3f tang1 = srcVec3[nextIndex * 3];
                    Vec3f vert1 = srcVec3[nextIndex * 3 + 1];
                    transform.scale = cubicSpline(vert0, tang0, vert1, tang1, t);
                }
                else {
                    transform.scale = (srcVec3[prevIndex] * (1 - t)) + (srcVec3[nextIndex] * t);
                }
                break;
            }

            case AnimationChannel::TRANSLATION: {
                const Vec3f* srcVec3 = (const Vec3f*)sampler.values.data();
                if (sampler.interpolation == AnimationSampler::CUBIC) {
                    Vec3f vert0 = srcVec3[prevIndex * 3 + 1];
                    Vec3f tang0 = srcVec3[prevIndex * 3 + 2];
                    Vec3f tang1 = srcVec3[nextIndex * 3];
                    Vec3f vert1 = srcVec3[nextIndex * 3 + 1];
                    transform.position = cubicSpline(vert0, tang0, vert1, tang1, t);
                }
                else {
                    transform.position = (srcVec3[prevIndex] * (1 - t)) + (srcVec3[nextIndex] * t);
                }
                break;
            }

            case AnimationChannel::ROTATION: {
                const Quaternionf* srcQuat = (const Quaternionf*)sampler.values.data();
                if (sampler.interpolation == AnimationSampler::CUBIC) {
                    Quaternionf vert0 = srcQuat[prevIndex * 3 + 1];
                    Quaternionf tang0 = srcQuat[prevIndex * 3 + 2];
                    Quaternionf tang1 = srcQuat[nextIndex * 3];
                    Quaternionf vert1 = srcQuat[nextIndex * 3 + 1];
                    transform.rotation = unit(cubicSpline(vert0, tang0, vert1, tang1, t));
                }
                else {
                    transform.rotation = slerp(srcQuat[prevIndex], srcQuat[nextIndex], t);
                }
                break;
            }

            case AnimationChannel::WEIGHTS: {
                const float* const samplerValues = sampler.values.data();
                const uint64 valuesPerKeyframe = sampler.values.size() / sampler.times.size();

                float weights[MAX_MORPH_TARGETS];
                uint64 num_morph_targets;

                if (sampler.interpolation == AnimationSampler::CUBIC) {
                    SOUL_ASSERT(0, valuesPerKeyframe % 3 == 0, "");
                    num_morph_targets = valuesPerKeyframe / 3;
                    const float* const inTangents = samplerValues;
                    const float* const splineVerts = samplerValues + num_morph_targets;
                    const float* const outTangents = samplerValues + num_morph_targets * 2;

                    for (uint64 comp = 0; comp < std::min(num_morph_targets, MAX_MORPH_TARGETS); ++comp) {
                        float vert0 = splineVerts[comp + prevIndex * valuesPerKeyframe];
                        float tang0 = outTangents[comp + prevIndex * valuesPerKeyframe];
                        float tang1 = inTangents[comp + nextIndex * valuesPerKeyframe];
                        float vert1 = splineVerts[comp + nextIndex * valuesPerKeyframe];
                        weights[comp] = cubicSpline(vert0, tang0, vert1, tang1, t);
                    }
                }
                else {
                    num_morph_targets = valuesPerKeyframe;
                    for (uint64 comp = 0; comp < std::min(valuesPerKeyframe, MAX_MORPH_TARGETS); ++comp) {
                        float previous = samplerValues[comp + prevIndex * valuesPerKeyframe];
                        float current = samplerValues[comp + nextIndex * valuesPerKeyframe];
                        weights[comp] = (1 - t) * previous + t * current;
                    }
                }

                RenderComponent& renderComp = registry_.get<RenderComponent>(channel.entity);
                for (uint64 weightIdx = 0; weightIdx < std::min(num_morph_targets, MAX_MORPH_TARGETS); weightIdx++) {
                    renderComp.morphWeights.mem[weightIdx] = weights[weightIdx];
                }
            }
            }
            
            Mat4f tmp = mat4Transform(transform);
            SOUL_ASSERT(0, std::none_of(tmp.mem, tmp.mem + 9, [](float i) { return std::isnan(i); }), "");
            transformComp.local = mat4Transform(transform);
        }
    };
    if (!active_animation_.is_null()) applyAnimation();

    if (active_camera_ != ENTITY_ID_NULL) {

        TransformComponent& transformComp = registry_.get<TransformComponent>(active_camera_);
        
        if (input.mouseDragging[Demo::Input::MOUSE_BUTTON_MIDDLE]) {
            // orbit camera
            ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
            if (input.keyShift) {
                camera_man_.pan(mouseDelta.x, mouseDelta.y);
            }
            else {
                camera_man_.orbit(mouseDelta.x, mouseDelta.y);
            }
        }

        if (input.keyShift) {
            camera_man_.zoom(input.mouseWheel);
        }

        transformComp.world = camera_man_.getTransformMatrix();
        if (transformComp.parent == ENTITY_ID_NULL) {
            transformComp.local = transformComp.world;
        }
        else {
            TransformComponent& parentComp = registry_.get<TransformComponent>(transformComp.parent);
            transformComp.local = mat4Inverse(parentComp.world) * transformComp.world;
        }
    }

    update_world_transform(root_entity_);
    update_bones();

    return true;
}

void soul_fila::Scene::update_world_transform(EntityID entityID) {
    if (entityID == ENTITY_ID_NULL) return;
    if (entityID == root_entity_) {
        TransformComponent& comp = registry_.get<TransformComponent>(entityID);
        comp.world = comp.local;
        SOUL_ASSERT(0, std::none_of(comp.world.mem, comp.world.mem + 9, [](float i) { return std::isnan(i); }), "");
        update_world_transform(comp.firstChild);
    }
    else {
        TransformComponent& comp = registry_.get<TransformComponent>(entityID);
        TransformComponent& parentComp = registry_.get<TransformComponent>(comp.parent);
        comp.world = parentComp.world * comp.local;
        SOUL_ASSERT(0, std::none_of(comp.world.mem, comp.world.mem + 9, [](float i) { return std::isnan(i); }), "");
        update_world_transform(comp.next);
        update_world_transform(comp.firstChild);
    }
}

void soul_fila::Scene::update_bones() {

    for (Skin& skin : skins_) {
        for (uint64 boneIdx = 0; boneIdx < skin.bones.size(); boneIdx++) {
            EntityID joint = skin.joints[boneIdx];
            const TransformComponent* transformComp = registry_.try_get<TransformComponent>(joint);
            if (transformComp == nullptr) continue;
            Mat4f boneMat = transformComp->world * skin.invBindMatrices[boneIdx];
            make_bone(&skin.bones[boneIdx], boneMat);
        }
    }
}

soul_fila::EntityID soul_fila::Scene::create_light(const LightDesc& light_desc, EntityID parent)
{
    auto entity_id = registry_.create();
    if (parent == ENTITY_ID_NULL)
    {
        parent = root_entity_;
    }
    registry_.emplace<TransformComponent>(entity_id, mat4Identity(), mat4Identity(), parent, ENTITY_ID_NULL, ENTITY_ID_NULL, ENTITY_ID_NULL);
    auto& light_comp = registry_.emplace<LightComponent>(entity_id);
    light_comp.lightType = light_desc.type;
    set_light_shadow_options(entity_id, light_desc.shadowOptions);
    set_light_local_position(entity_id, light_desc.position);
    set_light_local_direction(entity_id, light_desc.direction);
    set_light_color(entity_id, light_desc.linearColor);
    set_light_cone(entity_id, light_desc.spotInnerOuter.x, light_desc.spotInnerOuter.y);
    set_light_falloff(entity_id, light_desc.falloff);
    set_light_sun_angular_radius(entity_id, light_desc.sunAngle);
    set_light_sun_halo_size(entity_id, light_desc.sunHaloSize);
    set_light_sun_halo_falloff(entity_id, light_desc.sunHaloFalloff);
    set_light_intensity(entity_id, light_desc.intensity, light_desc.intensityUnit);
    return entity_id;
}

bool soul_fila::Scene::is_light(EntityID entity_id) const
{
    if (entity_id == ENTITY_ID_NULL) return false;
    const LightComponent* lightComp = registry_.try_get<LightComponent>(entity_id);
    return lightComp != nullptr;
}

bool soul_fila::Scene::is_sun_light(EntityID entity_id) const
{
    if (entity_id == ENTITY_ID_NULL) return false;
    const LightComponent* lightComp = registry_.try_get<LightComponent>(entity_id);
    if (lightComp == nullptr) return false;
    return lightComp->lightType.type == LightRadiationType::SUN;
}

bool soul_fila::Scene::is_directional_light(EntityID entity_id) const
{
    if (entity_id == ENTITY_ID_NULL) return false;
    const LightComponent* lightComp = registry_.try_get<LightComponent>(entity_id);
    if (lightComp == nullptr) return false;
    return lightComp->lightType.type == LightRadiationType::SUN || lightComp->lightType.type == LightRadiationType::DIRECTIONAL;
	
}

bool soul_fila::Scene::is_spot_light(EntityID entity_id) const
{
    if (entity_id == ENTITY_ID_NULL) return false;
    const LightComponent* lightComp = registry_.try_get<LightComponent>(entity_id);
    if (lightComp == nullptr) return false;
    return lightComp->lightType.type == LightRadiationType::SPOT || lightComp->lightType.type == LightRadiationType::FOCUSED_SPOT;
}

void soul_fila::Scene::set_light_shadow_options(EntityID entity_id, const ShadowOptions& options)
{
    SOUL_ASSERT(0, entity_id != ENTITY_ID_NULL, "");
    SOUL_ASSERT(0, is_light(entity_id), "");

    LightComponent& lightComp = registry_.get<LightComponent>(entity_id);
    ShadowParams& params = lightComp.shadowParams;
    params.options = options;
    params.options.mapSize = std::clamp(options.mapSize, 8u, 2048u);
    params.options.shadowCascades = std::clamp<uint8_t>(options.shadowCascades, 1, CONFIG_MAX_SHADOW_CASCADES);
    params.options.constantBias = std::clamp(options.constantBias, 0.0f, 2.0f);
    params.options.normalBias = std::clamp(options.normalBias, 0.0f, 3.0f);
    params.options.shadowFar = std::max(options.shadowFar, 0.0f);
    params.options.shadowNearHint = std::max(options.shadowNearHint, 0.0f);
    params.options.shadowFarHint = std::max(options.shadowFarHint, 0.0f);
    params.options.vsm.msaaSamples = std::max(uint8_t(0), options.vsm.msaaSamples);
    params.options.vsm.blurWidth = std::max(0.0f, options.vsm.blurWidth);
}

void soul_fila::Scene::set_light_local_position(EntityID entity_id, Vec3f position)
{
    SOUL_ASSERT(0, entity_id != ENTITY_ID_NULL, "");
    SOUL_ASSERT(0, is_light(entity_id), "");

    LightComponent& lightComp = registry_.get<LightComponent>(entity_id);
    lightComp.position = position;
}

void soul_fila::Scene::set_light_local_direction(EntityID entity_id, Vec3f direction)
{
    SOUL_ASSERT(0, entity_id != ENTITY_ID_NULL, "");
    SOUL_ASSERT(0, is_light(entity_id), "");

    LightComponent& lightComp = registry_.get<LightComponent>(entity_id);
    lightComp.direction = direction;
}

void soul_fila::Scene::set_light_color(EntityID entity_id, Vec3f color)
{
    SOUL_ASSERT(0, entity_id != ENTITY_ID_NULL, "");
    SOUL_ASSERT(0, is_light(entity_id), "");

    LightComponent& lightComp = registry_.get<LightComponent>(entity_id);
    lightComp.color = color;
}

void soul_fila::Scene::set_light_intensity(EntityID entity_id, float intensity, IntensityUnit unit)
{
    SOUL_ASSERT(0, entity_id != ENTITY_ID_NULL, "");
    SOUL_ASSERT(0, is_light(entity_id), "");

    SpotParams spotParams;
    float luminousPower = intensity;
    float luminousIntensity = 0.0f;
    using Type = LightRadiationType;
    LightComponent& lightComp = registry_.get<LightComponent>(entity_id);
    switch (lightComp.lightType.type) {
    case Type::SUN:
    case Type::DIRECTIONAL:
        // luminousPower is in lux, nothing to do.
        luminousIntensity = luminousPower;
        break;

    case Type::POINT:
        if (unit == IntensityUnit::LUMEN_LUX) {
            // li = lp / (4 * pi)
            luminousIntensity = luminousPower * soul::Fconst::ONE_OVER_PI * 0.25f;
        }
        else {
            SOUL_ASSERT(0, unit == IntensityUnit::CANDELA, "");
            // intensity specified directly in candela, no conversion needed
            luminousIntensity = luminousPower;
        }
        break;

    case Type::FOCUSED_SPOT: {
        float cosOuter = std::sqrt(spotParams.cosOuterSquared);
        if (unit == IntensityUnit::LUMEN_LUX) {
            // li = lp / (2 * pi * (1 - cos(cone_outer / 2)))
            luminousIntensity = luminousPower / (soul::Fconst::TAU * (1.0f - cosOuter));
        }
        else {
            SOUL_ASSERT(0, unit == IntensityUnit::CANDELA, "");
            // intensity specified directly in candela, no conversion needed
            luminousIntensity = luminousPower;
            // lp = li * (2 * pi * (1 - cos(cone_outer / 2)))
            luminousPower = luminousIntensity * (soul::Fconst::TAU * (1.0f - cosOuter));
        }
        spotParams.luminousPower = luminousPower;
        break;
    }
    case Type::SPOT:
        if (unit == IntensityUnit::LUMEN_LUX) {
            // li = lp / pi
            luminousIntensity = luminousPower * soul::Fconst::ONE_OVER_PI;
        }
        else {
            SOUL_ASSERT(0, unit == IntensityUnit::CANDELA, "");
            // intensity specified directly in candela, no conversion needed
            luminousIntensity = luminousPower;
        }
        break;
    case Type::COUNT:
    default:
        SOUL_NOT_IMPLEMENTED();
    }
    lightComp.intensity = luminousIntensity;
}

void soul_fila::Scene::set_light_falloff(EntityID entity_id, float falloff)
{

    SOUL_ASSERT(0, entity_id != ENTITY_ID_NULL, "");
    SOUL_ASSERT(0, is_light(entity_id), "");

    if (is_directional_light(entity_id))
    {
        LightComponent& lightComp = registry_.get<LightComponent>(entity_id);
    	float sqFalloff = falloff * falloff;
        SpotParams& spotParams = lightComp.spotParams;
        lightComp.squaredFallOffInv = sqFalloff > 0.0f ? (1 / sqFalloff) : 0;
        spotParams.radius = falloff;
    }

}

void soul_fila::Scene::set_light_cone(EntityID entity_id, float inner, float outer)
{
	if (is_spot_light(entity_id))
	{
        // clamp the inner/outer angles to pi
        float innerClamped = std::min(std::abs(inner), soul::Fconst::PI_2);
        float outerClamped = std::min(std::abs(outer), soul::Fconst::PI_2);

        // outer must always be bigger than inner
        outerClamped = std::max(innerClamped, outerClamped);

        float cosOuter = std::cos(outerClamped);
        float cosInner = std::cos(innerClamped);
        float cosOuterSquared = cosOuter * cosOuter;
        float scale = 1 / std::max(1.0f / 1024.0f, cosInner - cosOuter);
        float offset = -cosOuter * scale;

        LightComponent& lightComp = registry_.get<LightComponent>(entity_id);
        SpotParams& spotParams = lightComp.spotParams;
        spotParams.outerClamped = outerClamped;
        spotParams.cosOuterSquared = cosOuterSquared;
        spotParams.sinInverse = 1 / std::sqrt(1 - cosOuterSquared);
        spotParams.scaleOffset = { scale, offset };

        // we need to recompute the luminous intensity
        LightRadiationType type = lightComp.lightType.type;
        if (type == LightRadiationType::FOCUSED_SPOT) {
            // li = lp / (2 * pi * (1 - cos(cone_outer / 2)))
            float luminousPower = spotParams.luminousPower;
            float luminousIntensity = luminousPower / (soul::Fconst::TAU * (1.0f - cosOuter));
            lightComp.intensity = luminousIntensity;
        }
	}
}

void soul_fila::Scene::set_light_sun_angular_radius(EntityID entity_id, float angular_radius)
{
	if (is_sun_light(entity_id))
	{
        LightComponent& lightComp = registry_.get<LightComponent>(entity_id);
        lightComp.sunAngularRadius = soul::Fconst::DEG_TO_RAD * angular_radius;
	}
}

void soul_fila::Scene::set_light_sun_halo_size(EntityID entity_id, float halo_size)
{
    if (is_sun_light(entity_id))
    {
        LightComponent& lightComp = registry_.get<LightComponent>(entity_id);
        lightComp.sunHaloSize = halo_size;
    }
}

void soul_fila::Scene::set_light_sun_halo_falloff(EntityID entity_id, float halo_falloff)
{
    if (is_sun_light(entity_id))
    {
        LightComponent& lightComp = registry_.get<LightComponent>(entity_id);
        lightComp.sunHaloFalloff = halo_falloff;
    }
}

void soul_fila::CameraComponent::setLensProjection(float focalLengthInMillimeters, float aspect, float inNear, float inFar) {
    float h = (0.5f * inNear) * ((SENSOR_SIZE * 1000.0f) / focalLengthInMillimeters);
    float fovRadian = 2 * std::atan(h / inNear);
    setPerspectiveProjection(fovRadian, aspect, inNear, inFar);
}

void soul_fila::CameraComponent::setOrthoProjection(float left, float right, float bottom, float top, float inNear, float inFar) {
    this->projection = mat4Ortho(left, right, bottom, top, inNear, inFar);
    this->projectionForCulling = this->projection;

    this->near = inNear;
    this->far = inFar;
}

void soul_fila::CameraComponent::setPerspectiveProjection(float fovRadian, float aspectRatio, float inNear, float inFar) {
    this->projectionForCulling = mat4Perspective(fovRadian, aspectRatio, inNear, inFar);
    this->projection = this->projectionForCulling;
    
    // Make far infinity, lim (zFar -> infinity), then (zNear + zFar) * -1 / (zFar - zNear) = -1
    this->projection.elem[2][2] = -1;
    // lim (zFar -> infinity), then (-2 * zFar *zNear) / (zFar - zNear) = -2 * zNear
    this->projection.elem[2][3] = -2.0f * inNear;

    this->near = inNear;
    this->far = inFar;
}

void soul_fila::CameraComponent::setScaling(soul::Vec2f scale) {
    scaling = scale;
}

soul::Vec2f soul_fila::CameraComponent::getScaling() const {
    return this->scaling;
}

soul::Mat4f soul_fila::CameraComponent::getProjectionMatrix() const {
    return this->projection;
}

soul::Mat4f soul_fila::CameraComponent::getCullingProjectionMatrix() const {
    return this->projectionForCulling;
}

void soul_fila::TransformComponent::renderUI() {
    Transformf localTransform = soul::transformMat4(local);
    Transformf worldTransform = soul::transformMat4(world);

    Mat4f parentWorldMat = world * mat4Inverse(local);

    if (ImGui::CollapsingHeader("Transform Component")) {
        bool localTransformChange = false;
        ImGui::Text("Local Transform");
        localTransformChange |= ImGui::InputFloat3("Position##local", (float*)&localTransform.position);
        localTransformChange |= ImGui::InputFloat3("Scale##local", (float*)&localTransform.scale);
        localTransformChange |= ImGui::InputFloat4("Rotation##local", (float*)&localTransform.rotation);
        if (localTransformChange) {
            local = mat4Transform(localTransform);
            world = parentWorldMat * local;
        }

        bool worldTransformChange = false;
        ImGui::Text("World Transform");
        worldTransformChange |= ImGui::InputFloat3("Position##world", (float*)&worldTransform.position);
        worldTransformChange |= ImGui::InputFloat3("Scale##world", (float*)&worldTransform.scale);
        worldTransformChange |= ImGui::InputFloat4("Rotation##world", (float*)&worldTransform.rotation);
        if (worldTransformChange) {
            world = mat4Transform(worldTransform);
            local = mat4Inverse(parentWorldMat) * world;
        }
    }
}

void soul_fila::NameComponent::renderUI() {
    char nameBuffer[1024];
    std::copy(name.data(), name.data() + name.size() + 1, nameBuffer);
    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
        name = nameBuffer;
    }
}