#include <filesystem>
#include <cinttypes>

#include "core/math.h"
#include "core/string.h"

#include "runtime/runtime.h"
#include "runtime/scope_allocator.h"
#include "gpu/system.h"

#include "shader_generator.h"
#include "utils.h"

namespace Demo {

	static uint64 _GetHash(const char* templateKey, int length) {
		return Soul::hashFNV1((uint8*)templateKey, length * sizeof(char));
	}

	static const char* _GetPrecisionQualifier(ShaderPrecision precision) {
		switch (precision) {
			case ShaderPrecision::LOW:     return "lowp";
			case ShaderPrecision::MEDIUM:  return "mediump";
			case ShaderPrecision::HIGH:    return "highp";
			case ShaderPrecision::DEFAULT: return "";
		}
		SOUL_NOT_IMPLEMENTED();
		return "ERROR!";
	}

	static const char* _GetVarTypeName(ShaderVarType type) noexcept {
		switch (type) {
			case ShaderVarType::BOOL:   return "bool";
			case ShaderVarType::BOOL2:  return "bvec2";
			case ShaderVarType::BOOL3:  return "bvec3";
			case ShaderVarType::BOOL4:  return "bvec4";
			case ShaderVarType::FLOAT:  return "float";
			case ShaderVarType::FLOAT2: return "vec2";
			case ShaderVarType::FLOAT3: return "vec3";
			case ShaderVarType::FLOAT4: return "vec4";
			case ShaderVarType::INT:    return "int";
			case ShaderVarType::INT2:   return "ivec2";
			case ShaderVarType::INT3:   return "ivec3";
			case ShaderVarType::INT4:   return "ivec4";
			case ShaderVarType::UINT:   return "uint";
			case ShaderVarType::UINT2:  return "uvec2";
			case ShaderVarType::UINT3:  return "uvec3";
			case ShaderVarType::UINT4:  return "uvec4";
			case ShaderVarType::MAT3:   return "mat3";
			case ShaderVarType::MAT4:   return "mat4";
		}
		SOUL_NOT_IMPLEMENTED();
		return "ERROR!";
	}

	const char* _GetSamplerTypeName(SamplerType type, SamplerFormat format) {
		switch (type) {
		case SamplerType::SAMPLER_2D:
			switch (format) {
			case SamplerFormat::INT:    return "isampler2D";
			case SamplerFormat::UINT:   return "usampler2D";
			case SamplerFormat::FLOAT:  return "sampler2D";
			case SamplerFormat::SHADOW: return "sampler2DShadow";
			}
		case SamplerType::SAMPLER_3D:
			SOUL_ASSERT(0, format != SamplerFormat::SHADOW, "");
			switch (format) {
			case SamplerFormat::INT:    return "isampler3D";
			case SamplerFormat::UINT:   return "usampler3D";
			case SamplerFormat::FLOAT:  return "sampler3D";
			case SamplerFormat::SHADOW: return nullptr;
			}
		case SamplerType::SAMPLER_2D_ARRAY:
			switch (format) {
			case SamplerFormat::INT:    return "isampler2DArray";
			case SamplerFormat::UINT:   return "usampler2DArray";
			case SamplerFormat::FLOAT:  return "sampler2DArray";
			case SamplerFormat::SHADOW: return "sampler2DArrayShadow";
			}
		case SamplerType::SAMPLER_CUBEMAP:
			switch (format) {
			case SamplerFormat::INT:    return "isamplerCube";
			case SamplerFormat::UINT:   return "usamplerCube";
			case SamplerFormat::FLOAT:  return "samplerCube";
			case SamplerFormat::SHADOW: return "samplerCubeShadow";
			}
		}
		SOUL_NOT_IMPLEMENTED();
		return "ERROR!";
	}

	static ShaderPrecision _GetDefaultPrecision(ShaderType type) {
		using Precision = ShaderPrecision;
		return Precision::HIGH;
	}

	static void _GenerateProlog(Soul::String& stringBuilder, ShaderType shaderType) {
		const char* defaultPrecisionQual = _GetPrecisionQualifier(_GetDefaultPrecision(shaderType));
		stringBuilder
			.appendf("#version 450 core\n\n")
			.appendf("#extension GL_GOOGLE_cpp_style_line_directive : enable\n\n")
			.appendf("precision %s float;\n", defaultPrecisionQual)
			.appendf("precision %s int;\n\n", defaultPrecisionQual);
	}

	static void _GenerateDefine(Soul::String& stringBuilder, const ShaderDefine& shaderDefine) {
		switch (shaderDefine.type) {
			case ShaderDefineType::BOOL: {
				if (shaderDefine.boolean) stringBuilder.appendf("#define %s\n", shaderDefine.name);
				break;
			}
			case ShaderDefineType::STRING: {
				stringBuilder.appendf("#define %s %s\n", shaderDefine.name, shaderDefine.string);
				break;
			}
			case ShaderDefineType::INTEGER: {
				stringBuilder.appendf("#define %s %" PRIu64 "\n", shaderDefine.name, shaderDefine.integer);
				break;
			}
			default: {
				SOUL_NOT_IMPLEMENTED();
			}
		}
	}

	static void _GenerateShaderInput(Soul::String& stringBuilder, const ShaderInput& shaderInput, uint32 location) {
		const char* varTypeName = _GetVarTypeName(shaderInput.varType);
		const char* precisionQual = _GetPrecisionQualifier(shaderInput.precision);
		stringBuilder.appendf("layout(location = %" PRIu32 ") in %s %s %s", location, precisionQual, varTypeName, shaderInput.name);
		if (shaderInput.count != 1) {
			stringBuilder.appendf("[%" PRIu32 "]", shaderInput.count);
		}
		stringBuilder.appendf(";\n");
	}

	static void _GenerateShaderOutput(Soul::String& stringBuilder, const ShaderOutput& shaderOutput, uint32 location) {
		const char* varTypeName = _GetVarTypeName(shaderOutput.varType);
		const char* precisionQual = _GetPrecisionQualifier(shaderOutput.precision);
		stringBuilder.appendf("layout(location = %" PRIu32 ") out %s %s %s", location, precisionQual, varTypeName, shaderOutput.name);
		if (shaderOutput.count != 1) {
			stringBuilder.appendf("[%" PRIu32 "]", shaderOutput.count);
		}
		stringBuilder.appendf(";\n");
	}

	static void _GenerateUniform(Soul::String& stringBuilder, const ShaderUniform& shaderUniform) {
		stringBuilder.appendf("layout(set = %d, binding = %d, std140) uniform %s {\n", shaderUniform.set, shaderUniform.binding, shaderUniform.typeName);
		for (uint64 memberIndex = 0; memberIndex < shaderUniform.memberCount; memberIndex++) {
			const ShaderUniformMember& member = shaderUniform.members[memberIndex];
			const char* precision = _GetPrecisionQualifier(member.precision);
			const char* typeName = _GetVarTypeName(member.varType);
			stringBuilder.appendf("     %s %s %s", precision, typeName, member.name);
			if (member.count > 1) {
				stringBuilder.appendf("[%" PRIu32 "]", member.count);
			}
			stringBuilder.appendf(";\n");
		}
		stringBuilder.appendf("} %s;\n", shaderUniform.instanceName);
	}

	static void _GenerateSampler(Soul::String& stringBuilder, const ShaderSampler& shaderSampler) {
		const char* typeName = _GetSamplerTypeName(shaderSampler.type, shaderSampler.format);
		const char* precision = _GetPrecisionQualifier(shaderSampler.precision);
		stringBuilder.appendf("layout(set = %d, binding = %d) uniform %s %s %s;\n",
			shaderSampler.set, shaderSampler.binding,
			_GetPrecisionQualifier(shaderSampler.precision), 
			_GetSamplerTypeName(shaderSampler.type, shaderSampler.format), 
			shaderSampler.name);
	}

	void ShaderGenerator::addShaderTemplates(const char* groupName, const char* path) {
		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			SOUL_LOG_INFO("Entry path = %s", entry.path().string().c_str());
			char* shaderCode = LoadFile(entry.path().string().c_str(), this->_allocator);
			
			const std::filesystem::path& path = entry.path();
			if (!std::filesystem::is_regular_file(path)) continue;
			std::string filename = entry.path().filename().string();

			Soul::Runtime::ScopeAllocator<> scopeAllocator("AddShderTemplates");
			uint64 templateKeyLength = strlen(groupName) + strlen("::") + strlen(filename.c_str());
			Soul::String templateKey(&scopeAllocator, templateKeyLength + 1);
			templateKey.appendf("%s::%s", groupName, filename.c_str());

			uint64 templateKeyHash = _GetHash(templateKey.data(), templateKeyLength);
			SOUL_ASSERT(0, _templateMap.empty() || !_templateMap.isExist(templateKeyHash), "There duplicate hash for shader template");

			_templateMap.add(templateKeyHash, shaderCode);
		}
	}

	Soul::GPU::ShaderID ShaderGenerator::createShader(const ShaderDesc& shaderDesc) const {
		
		Soul::String stringBuilder(_allocator, 10000);

		auto _generateNewline = [&stringBuilder]() {
			stringBuilder.appendf("\n");
		};

		_GenerateProlog(stringBuilder, shaderDesc.type);

		for (uint32 defineIdx = 0; defineIdx < shaderDesc.defineCount; defineIdx++) {
			_GenerateDefine(stringBuilder, shaderDesc.defines[defineIdx]);
		}
		_generateNewline();
		
		for (uint32 inputIdx = 0; inputIdx < Soul::GPU::MAX_INPUT_PER_SHADER; inputIdx++) {
			const ShaderInput& input = shaderDesc.inputs[inputIdx];
			if (input.name == nullptr) continue;
			_GenerateShaderInput(stringBuilder, input, inputIdx);
		}
		_generateNewline();

		for (uint32 outputIdx = 0; outputIdx < Soul::GPU::MAX_INPUT_PER_SHADER; outputIdx++) {
			const ShaderOutput& output = shaderDesc.outputs[outputIdx];
			if (output.name == nullptr) continue;
			_GenerateShaderOutput(stringBuilder, output, outputIdx);
		}
		_generateNewline();

		for (uint32 uniformIdx = 0; uniformIdx < shaderDesc.uniformCount; uniformIdx++) {
			_GenerateUniform(stringBuilder, shaderDesc.uniforms[uniformIdx]);
			_generateNewline();
		}
		_generateNewline();

		for (uint32 samplerIdx = 0; samplerIdx < shaderDesc.samplerCount; samplerIdx++) {
			_GenerateSampler(stringBuilder, shaderDesc.samplers[samplerIdx]);
		}
		_generateNewline();

		for (uint32 templateCodeIdx = 0; templateCodeIdx < shaderDesc.templateCodeCount; templateCodeIdx++) {
			const char* templateCodeKey = shaderDesc.templateCodes[templateCodeIdx];
			char* templateCode = _templateMap[_GetHash(templateCodeKey, strlen(templateCodeKey))];
			stringBuilder.appendf("%s\n", templateCode);
		}


		if (shaderDesc.customCode != nullptr) stringBuilder.appendf("%s\n", shaderDesc.customCode);

		static auto _getShaderStage = [](ShaderType shaderType) -> Soul::GPU::ShaderStage {
			switch (shaderType) {
			case ShaderType::VERTEX: return Soul::GPU::ShaderStage::VERTEX;
			case ShaderType::FRAGMENT: return Soul::GPU::ShaderStage::FRAGMENT;
			}
			SOUL_NOT_IMPLEMENTED();
			return Soul::GPU::ShaderStage::NONE;
		};
		
		return _gpuSystem->shaderCreate({ "default", stringBuilder.data(), uint32(stringBuilder.size())}, _getShaderStage(shaderDesc.type));
	}
}