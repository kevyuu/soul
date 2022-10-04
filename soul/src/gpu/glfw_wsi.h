#include "core/type.h"
#include "gpu/type.h"

struct GLFWwindow;

namespace soul::gpu
{
    class GLFWWsi : public WSI
    {
    public:
        explicit GLFWWsi(GLFWwindow* window);
        [[nodiscard]] VkSurfaceKHR create_vulkan_surface(VkInstance instance) override;
        [[nodiscard]] vec2ui32 get_framebuffer_size() const override;
        ~GLFWWsi() override = default;
    private:
        GLFWwindow* window_;
    };
}