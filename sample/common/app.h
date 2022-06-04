#pragma once
struct GLFWwindow;

#include "memory/allocators/page_allocator.h"
#include "runtime/runtime.h"

namespace soul::gpu
{
	class System;
	class RenderGraph;
}

class App
{
public:
	App();
	App(const App&) = delete;
	App& operator=(const App&) = delete;
	App(App&& app) = delete;
	App& operator=(App&& app) = delete;
	virtual ~App();

	void run();
	
private:

	soul::memory::MallocAllocator malloc_allocator_;
	soul::runtime::DefaultAllocator default_allocator_;
	soul::memory::PageAllocator page_allocator_;
	soul::memory::ProxyAllocator<soul::memory::PageAllocator, soul::memory::ProfileProxy> proxy_page_allocator_;
	soul::memory::LinearAllocator linear_allocator_;
	soul::runtime::TempAllocator temp_allocator_;

	GLFWwindow* window_ = nullptr;

	virtual void render(soul::gpu::RenderGraph& render_graph) = 0;

protected:
	soul::gpu::System* gpu_system_ = nullptr;
};