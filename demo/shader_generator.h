#pragma once

#include "core/type.h"
#include "core/uint64_hash_map.h"
#include "gpu/data.h"
#include "memory/allocators/proxy_allocator.h"


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
        ShaderVarType varType;
        ShaderPrecision precision;
        constexpr ShaderInput() : name(nullptr), count(1), varType(ShaderVarType::COUNT), precision(ShaderPrecision::COUNT) {}
        constexpr ShaderInput(const char* name, ShaderVarType type, ShaderPrecision precision = ShaderPrecision::DEFAULT, uint32 count = 1) :
            name(name), varType(type), precision(precision), count(count) {}
    };

    struct ShaderOutput {
        const char* name = nullptr;
        uint32 count;
        ShaderVarType varType;
        ShaderPrecision precision;
        ShaderOutput() {}
        constexpr ShaderOutput(const char* name, ShaderVarType type, ShaderPrecision precision = ShaderPrecision::DEFAULT, uint32 count = 1) :
            name(name), varType(type), precision(precision), count(count) {}
    };

    struct ShaderUniformMember {
        const char* name = nullptr;
        ShaderPrecision precision = ShaderPrecision::DEFAULT;
        ShaderVarType varType = ShaderVarType::COUNT;
        uint32 count = 0;

        constexpr ShaderUniformMember() {}
        constexpr ShaderUniformMember(const char* name, ShaderVarType varType,
            ShaderPrecision precision = ShaderPrecision::DEFAULT, uint32 count = 1) :
            name(name), varType(varType), precision(precision), count(count) {

        }
    };

    struct ShaderUniform {
        const char* typeName = nullptr;
        const char* instanceName = nullptr;
        const ShaderUniformMember* members;
        uint8 memberCount;
        uint8 set;
        uint8 binding;
    };

    struct ShaderDefine {
        ShaderDefineType type;
        const char* name;
        union {
            bool boolean;
            uint64 integer;
            const char* string;
        };
        constexpr ShaderDefine(const char* name) : name(name), boolean(true), type(ShaderDefineType::BOOL) {}
        constexpr ShaderDefine(const char* name, uint64 integer) : name(name), integer(integer), type(ShaderDefineType::INTEGER) {}
        constexpr ShaderDefine(const char* name, const char* str) : name(name), string(str), type(ShaderDefineType::STRING) {}
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

        ShaderType type;

        ShaderInput inputs[Soul::GPU::MAX_INPUT_PER_SHADER];
        ShaderOutput outputs[Soul::GPU::MAX_INPUT_PER_SHADER];

        const ShaderUniform* uniforms;
        uint8 uniformCount;

        const ShaderSampler* samplers;
        uint8 samplerCount;

        const ShaderDefine* defines;
        uint8 defineCount;

        const char** templateCodes;
        uint8 templateCodeCount;

        const char* customCode = nullptr;
    };

    class ShaderGenerator {

    public:

        ShaderGenerator(Soul::Memory::Allocator* allocator, Soul::GPU::System* gpuSystem) :
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
        Soul::GPU::ShaderID createShader(const ShaderDesc& desc) const;

    private:

        Soul::Memory::Allocator* _allocator;
        Soul::Runtime::AllocatorInitializer _allocatorInitializer;

        Soul::UInt64HashMap<char*> _templateMap;
        Soul::GPU::System* _gpuSystem;
    };
}