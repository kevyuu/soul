#pragma once
#include <optional>

struct GLFWwindow;
class ImGuiRenderGraphPass;

#include "camera_manipulator.h"

#include "memory/allocators/page_allocator.h"
#include "runtime/runtime.h"

namespace soul::gpu
{
	class System;
	class RenderGraph;
	class GLFWWsi;
}

struct ScreenDimension
{
	int32 width;
	int32 height;
};

struct AppConfig
{
	std::optional<ScreenDimension> screen_dimension = std::nullopt;
	bool enable_imgui = false;
};

struct WindowData
{
	bool resized = false;
};

class App
{
public:
	explicit App(const AppConfig& app_config);
	App(const App&) = delete;
	App& operator=(const App&) = delete;
	App(App&& app) = delete;
	App& operator=(App&& app) = delete;
	virtual ~App();

	void run();
	float get_elapsed_seconds() const;
	
private:

	soul::memory::MallocAllocator malloc_allocator_;
	soul::runtime::DefaultAllocator default_allocator_;
	soul::memory::PageAllocator page_allocator_;
	soul::memory::ProxyAllocator<soul::memory::PageAllocator, soul::memory::ProfileProxy> proxy_page_allocator_;
	soul::memory::LinearAllocator linear_allocator_;
	soul::runtime::TempAllocator temp_allocator_;

	GLFWwindow* window_ = nullptr;
	ImGuiRenderGraphPass* imgui_render_graph_pass_ = nullptr;

	soul::gpu::GLFWWsi* wsi_ = nullptr;
	WindowData window_data_;

	const AppConfig app_config_;
	bool first_frame_ = true;

	const std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();

	virtual void render(soul::gpu::RenderGraph& render_graph) = 0;
	virtual void handle_input() {}

protected:
	soul::gpu::System* gpu_system_ = nullptr;
	CameraManipulator camera_man_;
};