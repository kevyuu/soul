#include "app.h"

#include "core/type.h"
#include "core/dev_util.h"

#include "memory/allocators/page_allocator.h"
#include "runtime/runtime.h"
#include "gpu/gpu.h"

#include <GLFW/glfw3.h>

#include "memory/allocators/proxy_allocator.h"

using namespace soul;

void glfw_print_error_callback(const int code, const char* message)
{
	SOUL_LOG_INFO("GLFW Error. Error code : %d. Message = %s", code, message);
}

App::App() :
	malloc_allocator_("Default Allocator"),
	default_allocator_(&malloc_allocator_,
		runtime::DefaultAllocatorProxy::Config(
			memory::MutexProxy::Config(),
			memory::ProfileProxy::Config(),
			memory::CounterProxy::Config(),
			memory::ClearValuesProxy::Config{ char(0xFA), char(0xFF) },
			memory::BoundGuardProxy::Config())),
	page_allocator_("Page allocator"),
	proxy_page_allocator_(&page_allocator_, memory::ProfileProxy::Config()),
	linear_allocator_("Main Thread Temporary Allocator", 10 * soul::memory::ONE_MEGABYTE, &proxy_page_allocator_),
	temp_allocator_(&linear_allocator_, runtime::TempProxy::Config())
{
	SOUL_PROFILE_THREAD_SET_NAME("Main Thread");

	glfwSetErrorCallback(glfw_print_error_callback);
	bool glfw_init_success = glfwInit();
	SOUL_ASSERT(0, glfw_init_success, "GLFW initialization failed!");
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	SOUL_LOG_INFO("GLFW Initialization sucessful");

	SOUL_ASSERT(0, glfwVulkanSupported(), "Vulkan is not supported by glfw");

	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	SOUL_ASSERT(0, mode != nullptr, "Mode cannot be nullpointer");
	window_ = glfwCreateWindow(mode->width, mode->height, "Vulkan", nullptr, nullptr);
	glfwMaximizeWindow(window_);
	SOUL_LOG_INFO("GLFW window creation sucessful");

	runtime::init({
		0,
		4096,
		&temp_allocator_,
		20 * soul::memory::ONE_MEGABYTE,
		&default_allocator_
		});

	gpu_system_ = default_allocator_.create<gpu::System>(runtime::get_context_allocator());
	const gpu::System::Config config = {
		.windowHandle = window_,
		.swapchainWidth = 3360,
		.swapchainHeight = 2010,
		.maxFrameInFlight = 3,
		.threadCount = runtime::get_thread_count()
	};

	gpu_system_->init(config);
}

App::~App()
{
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

		{
			SOUL_PROFILE_ZONE_WITH_NAME("GLFW Poll Events");
			glfwPollEvents();
		}
		gpu::RenderGraph render_graph;

		render(render_graph);

		gpu_system_->execute(render_graph);
		gpu_system_->flush_frame();
	}
}