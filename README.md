# Soul
Vulkan Rendering Engine

Implemented feature:
- RenderGraph: Automatic GPU-CPU and GPU-GPU synchronization. RenderGraph will track dependencies between render pass and insert the require memory and execution barrier.
- Custom std library with rust philosophy in mind.
- Custom memory allocator like linear allocator, pool allocator and proxy allocator.
- Job system with Chales-Lev work stealing deque.
- Hybrid rendering pipeline with ray traced AO, shadow. DDGI is used for global illumination.

Video Demo:
https://www.youtube.com/watch?v=qsXRj9AflO4

Screenshots:

![Glow postprocess](https://i0.wp.com/kevinyu.net/wp-content/uploads/2019/04/ybYmvZpYC7.jpg?resize=768%2C388&ssl=1)

![Global illumination](https://i2.wp.com/kevinyu.net/wp-content/uploads/2019/01/QXhkdnJy3a-1.gif?fit=900%2C490&quality=95)
