#include "app.h"
#include "imgui_impl_glfw.h"

#include "core/dev_util.h"
#include "core/type.h"

#include "memory/allocators/page_allocator.h"
#include "runtime/runtime.h"
#include "gpu/gpu.h"

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>

#include "imgui_pass.h"
#include "memory/allocators/proxy_allocator.h"

using namespace soul;

void glfw_print_error_callback(const int code, const char* message)
{
	SOUL_LOG_INFO("GLFW Error. Error code : %d. Message = %s", code, message);
}

App::App(const AppConfig& app_config) :
	malloc_allocator_("Default Allocator"),
	default_allocator_(&malloc_allocator_,
		runtime::DefaultAllocatorProxy::Config(
			memory::MutexProxy::Config(),
			memory::ProfileProxy::Config(),
			memory::CounterProxy::Config(),
			memory::ClearValuesProxy::Config{ uint8{0xFA}, uint8{0xFF} },
			memory::BoundGuardProxy::Config())),
	page_allocator_("Page allocator"),
	proxy_page_allocator_(&page_allocator_, memory::ProfileProxy::Config()),
	linear_allocator_("Main Thread Temporary Allocator", 10 * soul::ONE_MEGABYTE, &proxy_page_allocator_),
	temp_allocator_(&linear_allocator_, runtime::TempProxy::Config()),
    app_config_(app_config)
{
	SOUL_PROFILE_THREAD_SET_NAME("Main Thread");

	glfwSetErrorCallback(glfw_print_error_callback);
	const bool glfw_init_success = glfwInit();
	SOUL_ASSERT(0, glfw_init_success, "GLFW initialization failed!");
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	SOUL_LOG_INFO("GLFW Initialization sucessful");

	SOUL_ASSERT(0, glfwVulkanSupported(), "Vulkan is not supported by glfw");

	if (app_config.screen_dimension)
	{
		const ScreenDimension screen_dimension = *app_config.screen_dimension;
		window_ = glfwCreateWindow(screen_dimension.width, screen_dimension.height, "Vulkan", nullptr, nullptr);
	}
	else
	{
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		SOUL_ASSERT(0, mode != nullptr, "Mode cannot be nullpointer");
		window_ = glfwCreateWindow(mode->width, mode->height, "Vulkan", nullptr, nullptr);
		glfwMaximizeWindow(window_);
	}
	SOUL_LOG_INFO("GLFW window creation sucessful");

	runtime::init({
		0,
		4096,
		&temp_allocator_,
		20 * soul::ONE_MEGABYTE,
		&default_allocator_
		});

	gpu_system_ = default_allocator_.create<gpu::System>(runtime::get_context_allocator());
	const gpu::System::Config config = {
		.window_handle = window_,
		.swapchain_width = 3360,
		.swapchain_height = 2010,
		.max_frame_in_flight = 3,
		.thread_count = runtime::get_thread_count()
	};

	gpu_system_->init(config);


	if (app_config.enable_imgui)
	{
		imgui_render_graph_pass_ = runtime::get_context_allocator()->create<ImGuiRenderGraphPass>(window_, gpu_system_);
	}

}

App::~App()
{
	if (app_config_.enable_imgui)
	{
		runtime::get_context_allocator()->destroy(imgui_render_graph_pass_);
	}
	default_allocator_.destroy(gpu_system_);
	glfwDestroyWindow(window_);
	glfwTerminate();
}

void App::run()
{
	while (!glfwWindowShouldClose(window_))
	{
		SOUL_PROFILE_FRAME();
		runtime::System::Get().beginFrame();

		soul::vec2f glfw_window_scale;
	    glfwGetWindowContentScale(window_, &glfw_window_scale.x, &glfw_window_scale.y);
		SOUL_ASSERT(0, glfw_window_scale.x - glfw_window_scale.y == 0.0f, "");
		
	    {
			SOUL_PROFILE_ZONE_WITH_NAME("GLFW Poll Events");
			glfwPollEvents();
		}
		gpu::RenderGraph render_graph;

		if (app_config_.enable_imgui)
		{
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGuiIO& io = ImGui::GetIO();
			io.FontGlobalScale = glfw_window_scale.x;
		}
		render(render_graph);
		if (app_config_.enable_imgui)
		{
			ImGui::Render();
			imgui_render_graph_pass_->add_pass(render_graph);
		}

		gpu_system_->execute(render_graph);
		gpu_system_->flush_frame();
	}
}

float App::get_elapsed_seconds() const
{
	const auto end = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - start_;
	const auto elapsed_seconds_float = static_cast<float>(elapsed_seconds.count());
	return elapsed_seconds_float;
}
