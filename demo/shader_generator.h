#pragma once

#include "core/type.h"
#include "core/uint64_hash_map.h"
#include "gpu/type.h"

namespace Demo {
    enum class ShaderType : uint8 {
        VERTEX,
        FRAGMENT,
        COUNT
    };

    enum class ShaderPrecision : uint8 {
        LOW,
        MEDIUM,
        HIGH,
        DEFAULT,
        COUNT,
    };

    enum class ShaderVarType : uint8 {
        BOOL,
        BOOL2,
        BOOL3,
        BOOL4,
        FLOAT,
        FLOAT2,
        FLOAT3,
        FLOAT4,
        INT,
        INT2,
        INT3,
        INT4,
        UINT,
        UINT2,
        UINT3,
        UINT4,
        MAT3,   //!< a 3x3 float matrix
        MAT4,    //!< a 4x4 float matrix
        COUNT
    };

    enum class ShaderDefineType : uint8 {
        BOOL,
        INTEGER,
        STRING,
        COUNT
    };

    struct ShaderInput {
        const char* name = nullptr;
        uint32 count = 1;
        ShaderVarType varType = ShaderVarType::COUNT;
        ShaderPrecision precision = ShaderPrecision::COUNT;
        constexpr ShaderInput() = default;
        constexpr ShaderInput(const char* name, ShaderVarType type, ShaderPrecision precision = ShaderPrecision::DEFAULT, uint32 count = 1) :
            name(name), count(count), varType(type), precision(precision) {}
    };

    struct ShaderOutput {
        const char* name = nullptr;
        uint32 count = 0;
        ShaderVarType varType = ShaderVarType::COUNT;
        ShaderPrecision precision = ShaderPrecision::COUNT;
        constexpr ShaderOutput() = default;
        constexpr ShaderOutput(const char* name, ShaderVarType type, ShaderPrecision precision = ShaderPrecision::DEFAULT, uint32 count = 1) :
            name(name), count(count), varType(type), precision(precision) {}
    };

    struct ShaderUniformMember {
        const char* name = nullptr;
        ShaderPrecision precision = ShaderPrecision::DEFAULT;
        ShaderVarType varType = ShaderVarType::COUNT;
        uint32 count = 0;

        ShaderUniformMember() = default;
        constexpr ShaderUniformMember(const char* name, ShaderVarType varType,
            ShaderPrecision precision = ShaderPrecision::DEFAULT, uint32 count = 1) :
            name(name), precision(precision), varType(varType), count(count) {}
    };

    struct ShaderUniform {
        const char* typeName = nullptr;
        const char* instanceName = nullptr;
        const ShaderUniformMember* members = nullptr;
        uint8 memberCount = 0;
        uint8 set = 0;
        uint8 binding = 0;
    };

    struct ShaderDefine {
        ShaderDefineType type;
        const char* name;
        union {
            bool boolean;
            uint64 integer;
            const char* string;
        };
        explicit constexpr ShaderDefine(const char* name) : type(ShaderDefineType::BOOL), name(name), boolean(true) {}  // NOLINT(cppcoreguidelines-pro-type-member-init)
        constexpr ShaderDefine(const char* name, uint64 integer) : type(ShaderDefineType::INTEGER), name(name), integer(integer) {}  // NOLINT(cppcoreguidelines-pro-type-member-init)
        constexpr ShaderDefine(const char* name, const char* str) : type(ShaderDefineType::STRING), name(name), string(str) {}  // NOLINT(cppcoreguidelines-pro-type-member-init)
    };

    enum class SamplerType : uint8 {
        SAMPLER_2D,         //!< 2D texture
        SAMPLER_2D_ARRAY,   //!< 2D array texture   
        SAMPLER_CUBEMAP,    //!< Cube map texture
        SAMPLER_3D,         //!< 3D texture
        COUNT
    };

    enum class SamplerFormat : uint8 {
        INT,
        UINT,
        FLOAT,
        SHADOW,
        COUNT
    };

    struct ShaderSampler {
        const char* name;
        SamplerType type;
        SamplerFormat format;
        ShaderPrecision precision;
        uint8 set;
        uint8 binding;

        ShaderSampler(const char* name, uint8 set, uint8 binding, SamplerType type, SamplerFormat format, ShaderPrecision precision = ShaderPrecision::DEFAULT) :
            name(name), type(type), format(format), precision(precision), binding(binding), set(set) {}
    };


    struct ShaderDesc {
        const char* name = nullptr;

        ShaderType type = ShaderType::COUNT;

        ShaderInput inputs[soul::gpu::MAX_INPUT_PER_SHADER];
        ShaderOutput outputs[soul::gpu::MAX_INPUT_PER_SHADER];

        const ShaderUniform* uniforms = nullptr;
        uint8 uniformCount = 0;

        const ShaderSampler* samplers = nullptr;
        uint8 samplerCount = 0;

        const ShaderDefine* defines = nullptr;
        uint8 defineCount = 0;

        const char** templateCodes = nullptr;
        uint8 templateCodeCount = 0;

        const char* customCode = nullptr;
    };

    class ShaderGenerator {

    public:

        ShaderGenerator(soul::memory::Allocator* allocator, soul::gpu::System* gpuSystem) :
            _allocator(allocator),
            _allocatorInitializer(_allocator),
            _gpuSystem(gpuSystem) {
            _allocatorInitializer.end();
        }

        ShaderGenerator(const ShaderGenerator&) = delete;
        ShaderGenerator(ShaderGenerator&&) = delete;
        ShaderGenerator& operator=(const ShaderGenerator&) = delete;
        ShaderGenerator& operator=(ShaderGenerator&&) = delete;
        ~ShaderGenerator() = default;

        void addShaderTemplates(const char* groupName, const char* path);
        soul::gpu::ShaderID createShader(const ShaderDesc& desc) const;

    private:

        soul::memory::Allocator* _allocator;
        soul::runtime::AllocatorInitializer _allocatorInitializer;

        soul::UInt64HashMap<char*> _templateMap;
        soul::gpu::System* _gpuSystem;
    };
}