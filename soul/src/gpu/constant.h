#pragma once

#include "core/type.h"

namespace soul::gpu
{
	constexpr uint32 MAX_SET_PER_SHADER_PROGRAM = 6;
	constexpr uint32 SET_COUNT = 8;
	constexpr uint32 MAX_BINDING_PER_SET = 16;
	constexpr uint32 MAX_INPUT_PER_SHADER = 16;
	constexpr uint32 MAX_INPUT_BINDING_PER_SHADER = 16;

	constexpr uint32 MAX_DEPTH_ATTACHMENT_PER_SHADER = 1;
	constexpr uint32 MAX_COLOR_ATTACHMENT_PER_SHADER = 8;
	constexpr uint32 MAX_INPUT_ATTACHMENT_PER_SHADER = 8;
	constexpr uint32 MAX_RESOLVE_ATTACHMENT_PER_SHADER = MAX_COLOR_ATTACHMENT_PER_SHADER;
	constexpr uint32 MAX_ATTACHMENT_PER_SHADER = MAX_COLOR_ATTACHMENT_PER_SHADER
		+ MAX_RESOLVE_ATTACHMENT_PER_SHADER + MAX_INPUT_ATTACHMENT_PER_SHADER
		+ MAX_DEPTH_ATTACHMENT_PER_SHADER;

	constexpr uint32 MAX_DYNAMIC_BUFFER_PER_SET = 6;
	constexpr uint32 MAX_SIGNAL_SEMAPHORE = 4;
	constexpr uint32 MAX_VERTEX_BINDING = 16;
}