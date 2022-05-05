#pragma once
#include "gpu/type.h"
#include "../data.h"

namespace soul_fila
{
	/*
		 *   Command key encoding
		 *   --------------------
		 *
		 *   a     = alpha masking
		 *   ppp   = priority
		 *   t     = two-pass transparency ordering
		 *   0     = reserved, must be zero
		 *
		 *   DEPTH command
		 *   |   6  | 2| 2|1| 3 | 2|       16       |               32               |
		 *   +------+--+--+-+---+--+----------------+--------------------------------+
		 *   |000000|01|00|0|ppp|00|0000000000000000|          distanceBits          |
		 *   +------+--+--+-+---+-------------------+--------------------------------+
		 *   | correctness      |     optimizations (truncation allowed)             |
		 *
		 *
		 *   COLOR command
		 *   |   6  | 2| 2|1| 3 | 2|  6   |   10     |               32               |
		 *   +------+--+--+-+---+--+------+----------+--------------------------------+
		 *   |000001|01|00|a|ppp|00|000000| Z-bucket |          material-id           |
		 *   |000010|01|00|a|ppp|00|000000| Z-bucket |          material-id           | refraction
		 *   +------+--+--+-+---+--+------+----------+--------------------------------+
		 *   | correctness      |      optimizations (truncation allowed)             |
		 *
		 *
		 *   BLENDED command
		 *   |   6  | 2| 2|1| 3 | 2|              32                |         15    |1|
		 *   +------+--+--+-+---+--+--------------------------------+---------------+-+
		 *   |000011|01|00|0|ppp|00|         ~distanceBits          |   blendOrder  |t|
		 *   +------+--+--+-+---+--+--------------------------------+---------------+-+
		 *   | correctness                                                            |
		 *
		 *
		 *   pre-CUSTOM command
		 *   |   6  | 2| 2|         22           |               32               |
		 *   +------+--+--+----------------------+--------------------------------+
		 *   | pass |00|00|        order         |      custom command index      |
		 *   +------+--+--+----------------------+--------------------------------+
		 *   | correctness                                                        |
		 *
		 *
		 *   post-CUSTOM command
		 *   |   6  | 2| 2|         22           |               32               |
		 *   +------+--+--+----------------------+--------------------------------+
		 *   | pass |11|00|        order         |      custom command index      |
		 *   +------+--+--+----------------------+--------------------------------+
		 *   | correctness                                                        |
		 *
		 *
		 *   SENTINEL command
		 *   |                                   64                                  |
		 *   +--------.--------.--------.--------.--------.--------.--------.--------+
		 *   |11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111|
		 *   +-----------------------------------------------------------------------+
		 */
	using CommandKey = uint64_t;

	static constexpr uint64_t DISTANCE_BITS_MASK = 0xFFFFFFFFllu;
	static constexpr unsigned DISTANCE_BITS_SHIFT = 0;

	static constexpr uint64_t BLEND_ORDER_MASK = 0xFFFEllu;
	static constexpr unsigned BLEND_ORDER_SHIFT = 1;

	static constexpr uint64_t BLEND_TWO_PASS_MASK = 0x1llu;
	static constexpr unsigned BLEND_TWO_PASS_SHIFT = 0;

	static constexpr uint64_t MATERIAL_INSTANCE_ID_MASK = 0x00000FFFllu;
	static constexpr unsigned MATERIAL_INSTANCE_ID_SHIFT = 0;

	static constexpr uint64_t MATERIAL_VARIANT_KEY_MASK = 0x000FF000llu;
	static constexpr unsigned MATERIAL_VARIANT_KEY_SHIFT = 12;

	static constexpr uint64_t MATERIAL_ID_MASK = 0xFFF00000llu;
	static constexpr unsigned MATERIAL_ID_SHIFT = 20;

	static constexpr uint64_t BLEND_DISTANCE_MASK = 0xFFFFFFFF0000llu;
	static constexpr unsigned BLEND_DISTANCE_SHIFT = 16;

	static constexpr uint64_t MATERIAL_MASK = 0xFFFFFFFFllu;
	static constexpr unsigned MATERIAL_SHIFT = 0;

	static constexpr uint64_t Z_BUCKET_MASK = 0x3FF00000000llu;
	static constexpr unsigned Z_BUCKET_SHIFT = 32;

	static constexpr uint64_t PRIORITY_MASK = 0x001C000000000000llu;
	static constexpr unsigned PRIORITY_SHIFT = 50;

	static constexpr uint64_t BLENDING_MASK = 0x0020000000000000llu;
	static constexpr unsigned BLENDING_SHIFT = 53;

	static constexpr uint64_t PASS_MASK = 0xFC00000000000000llu;
	static constexpr unsigned PASS_SHIFT = 58;

	static constexpr uint64_t CUSTOM_MASK = 0x0300000000000000llu;
	static constexpr unsigned CUSTOM_SHIFT = 56;

	static constexpr uint64_t CUSTOM_ORDER_MASK = 0x003FFFFF00000000llu;
	static constexpr unsigned CUSTOM_ORDER_SHIFT = 32;

	static constexpr uint64_t CUSTOM_INDEX_MASK = 0x00000000FFFFFFFFllu;
	static constexpr unsigned CUSTOM_INDEX_SHIFT = 0;


	enum class Pass : uint64_t {    // 6-bits max
		DEPTH = uint64_t(0x00) << PASS_SHIFT,
		COLOR = uint64_t(0x01) << PASS_SHIFT,
		REFRACT = uint64_t(0x02) << PASS_SHIFT,
		BLENDED = uint64_t(0x03) << PASS_SHIFT,
		SENTINEL = 0xffffffffffffffffllu
	};

	enum class CustomCommand : uint64_t {    // 2-bits max
		PROLOG = uint64_t(0x0) << CUSTOM_SHIFT,
		PASS = uint64_t(0x1) << CUSTOM_SHIFT,
		EPILOG = uint64_t(0x2) << CUSTOM_SHIFT
	};

	struct RasterState {

		using CullingMode = gpu::CullMode;
		using DepthFunc = gpu::CompareOp;
		using BlendEquation = gpu::BlendOp;
		using BlendFunction = gpu::BlendFactor;

		RasterState() noexcept { // NOLINT
			static_assert(sizeof(RasterState) == sizeof(uint32_t),
				"RasterState size not what was intended");
			culling = CullingMode::BACK;
			blendEquationRGB = BlendEquation::ADD;
			blendEquationAlpha = BlendEquation::ADD;
			blendFunctionSrcRGB = BlendFunction::ONE;
			blendFunctionSrcAlpha = BlendFunction::ONE;
			blendFunctionDstRGB = BlendFunction::ZERO;
			blendFunctionDstAlpha = BlendFunction::ZERO;
		}

		bool operator == (RasterState rhs) const noexcept { return u == rhs.u; }
		bool operator != (RasterState rhs) const noexcept { return u != rhs.u; }

		void disableBlending() noexcept {
			blendEquationRGB = BlendEquation::ADD;
			blendEquationAlpha = BlendEquation::ADD;
			blendFunctionSrcRGB = BlendFunction::ONE;
			blendFunctionSrcAlpha = BlendFunction::ONE;
			blendFunctionDstRGB = BlendFunction::ZERO;
			blendFunctionDstAlpha = BlendFunction::ZERO;
		}

		// note: clang reduces this entire function to a simple load/mask/compare
		bool hasBlending() const noexcept {
			// This is used to decide if blending needs to be enabled in the h/w
			return !(blendEquationRGB == BlendEquation::ADD &&
				blendEquationAlpha == BlendEquation::ADD &&
				blendFunctionSrcRGB == BlendFunction::ONE &&
				blendFunctionSrcAlpha == BlendFunction::ONE &&
				blendFunctionDstRGB == BlendFunction::ZERO &&
				blendFunctionDstAlpha == BlendFunction::ZERO);
		}

		union {
			struct {
				//! culling mode
				CullingMode culling : 2;        //  2

				//! blend equation for the red, green and blue components
				BlendEquation blendEquationRGB : 3;        //  5
				//! blend equation for the alpha component
				BlendEquation blendEquationAlpha : 3;        //  8

				//! blending function for the source color
				BlendFunction blendFunctionSrcRGB : 4;        // 12
				//! blending function for the source alpha
				BlendFunction blendFunctionSrcAlpha : 4;        // 16
				//! blending function for the destination color
				BlendFunction blendFunctionDstRGB : 4;        // 20
				//! blending function for the destination alpha
				BlendFunction blendFunctionDstAlpha : 4;        // 24

				//! Whether depth-buffer writes are enabled
				bool depthWrite : 1;        // 25
				//! Depth test function
				DepthFunc depthFunc : 3;        // 28

				//! Whether color-buffer writes are enabled
				bool colorWrite : 1;        // 29

				//! use alpha-channel as coverage mask for anti-aliasing
				bool alphaToCoverage : 1;        // 30

				//! whether front face winding direction must be inverted
				bool inverseFrontFaces : 1;        // 31

				//! padding, must be 0
				uint8_t padding : 1;        // 32
			};
			uint32_t u = 0;
		};
	};

	struct DrawItem
	{
		CommandKey key = 0;
		const Material* material = nullptr;
		const Primitive* primitive = nullptr;
		RasterState rasterState;
		gpu::ProgramID programID;
		uint32 index = 0;

		bool operator < (DrawItem const& rhs) const noexcept { return key < rhs.key; }

		static void ToPipelineStateDesc(const DrawItem& drawItem, soul::gpu::GraphicPipelineStateDesc& desc);
	};

	template<typename T>
	static uint64 MakeField(T value, uint64_t mask, unsigned shift) noexcept {
		SOUL_ASSERT(0, !((uint64_t(value) << shift) & ~mask), "");
		return uint64_t(value) << shift;
	}

	/*
	 * The sorting material key is 32 bits and encoded as:
	 *
	 * |     24     |   8    |
	 * +------------+--------+
	 * |  material  |variant |
	 * +------------+--------+
	 *
	 * The variant is inserted while building the commands, because we don't know it before that
	 */
	static CommandKey MakeMaterialSortingKey(uint32 materialID, GPUProgramVariant variant) noexcept {
		CommandKey key = ((materialID << MATERIAL_ID_SHIFT) & MATERIAL_ID_MASK);
		key |= MakeField(variant.key, MATERIAL_VARIANT_KEY_MASK, MATERIAL_VARIANT_KEY_SHIFT);
		return (key << MATERIAL_SHIFT) & MATERIAL_MASK;
	}

	template<typename T>
	static uint64 Select(T boolish) noexcept {
		return boolish ? std::numeric_limits<uint64_t>::max() : uint64_t(0);
	}

}