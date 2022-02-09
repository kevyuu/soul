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

	static uint64 GetHash_(const char* templateKey, soul_size length) {
		return soul::hash_fnv1(soul::cast<const uint8*>(templateKey), length * sizeof(char));
	}

	static const char* GetPrecisionQualifier_(ShaderPrecision precision) {
		switch (precision) {
			case ShaderPrecision::LOW:     return "lowp";
			case ShaderPrecision::MEDIUM:  return "mediump";
			case ShaderPrecision::HIGH:    return "highp";
			case ShaderPrecision::DEFAULT: return "";
			case ShaderPrecision::COUNT:
			default:
				SOUL_NOT_IMPLEMENTED();
				return "ERROR!";
		}
	}

	static const char* GetVarTypeName_(ShaderVarType type) noexcept {
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
			case ShaderVarType::COUNT:
			default:
				SOUL_NOT_IMPLEMENTED();
				return "ERROR!";
		}
	}

	static const char* GetSamplerTypeName_(SamplerType type, SamplerFormat format) {
		switch (type) {
		case SamplerType::SAMPLER_2D:
			switch (format) {
			case SamplerFormat::INT:    return "isampler2D";
			case SamplerFormat::UINT:   return "usampler2D";
			case SamplerFormat::FLOAT:  return "sampler2D";
			case SamplerFormat::SHADOW: return "sampler2DShadow";
			case SamplerFormat::COUNT:
			default:
				SOUL_NOT_IMPLEMENTED();
				return "ERROR!";
			}
		case SamplerType::SAMPLER_3D:
			SOUL_ASSERT(0, format != SamplerFormat::SHADOW, "");
			switch (format) {
			case SamplerFormat::INT:    return "isampler3D";
			case SamplerFormat::UINT:   return "usampler3D";
			case SamplerFormat::FLOAT:  return "sampler3D";
			case SamplerFormat::SHADOW: return nullptr;
			case SamplerFormat::COUNT:
			default:
				SOUL_NOT_IMPLEMENTED();
				return "ERROR!";
			}
		case SamplerType::SAMPLER_2D_ARRAY:
			switch (format) {
			case SamplerFormat::INT:    return "isampler2DArray";
			case SamplerFormat::UINT:   return "usampler2DArray";
			case SamplerFormat::FLOAT:  return "sampler2DArray";
			case SamplerFormat::SHADOW: return "sampler2DArrayShadow";
			case SamplerFormat::COUNT:
			default:
				SOUL_NOT_IMPLEMENTED();
				return "ERROR!";
			}
		case SamplerType::SAMPLER_CUBEMAP:
			switch (format) {
			case SamplerFormat::INT:    return "isamplerCube";
			case SamplerFormat::UINT:   return "usamplerCube";
			case SamplerFormat::FLOAT:  return "samplerCube";
			case SamplerFormat::SHADOW: return "samplerCubeShadow";
			case SamplerFormat::COUNT:
			default:
				SOUL_NOT_IMPLEMENTED();
				return "ERROR!";
			}
		case SamplerType::COUNT:
		default:
			SOUL_NOT_IMPLEMENTED();
			return "ERROR!";
		}
	}

	static ShaderPrecision GetDefaultPrecision_(ShaderType type) {
		using Precision = ShaderPrecision;
		return Precision::HIGH;
	}

	static void GenerateProlog_(soul::String& stringBuilder, ShaderType shaderType) {
		const char* defaultPrecisionQual = GetPrecisionQualifier_(GetDefaultPrecision_(shaderType));
		stringBuilder
			.appendf("#version 450 core\n\n")
			.appendf("#extension GL_GOOGLE_cpp_style_line_directive : enable\n\n")
			.appendf("precision %s float;\n", defaultPrecisionQual)
			.appendf("precision %s int;\n\n", defaultPrecisionQual);
	}

	static void GenerateDefine_(soul::String& stringBuilder, const ShaderDefine& shaderDefine) {
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
			case ShaderDefineType::COUNT:
			default: {
				SOUL_NOT_IMPLEMENTED();
				break;
			}
		}
	}

	static void GenerateShaderInput_(soul::String& stringBuilder, const ShaderInput& shaderInput, uint32 location) {
		const char* varTypeName = GetVarTypeName_(shaderInput.varType);
		const char* precisionQual = GetPrecisionQualifier_(shaderInput.precision);
		stringBuilder.appendf("layout(location = %" PRIu32 ") in %s %s %s", location, precisionQual, varTypeName, shaderInput.name);
		if (shaderInput.count != 1) {
			stringBuilder.appendf("[%" PRIu32 "]", shaderInput.count);
		}
		stringBuilder.appendf(";\n");
	}

	static void GenerateShaderOutput_(soul::String& stringBuilder, const ShaderOutput& shaderOutput, uint32 location) {
		const char* varTypeName = GetVarTypeName_(shaderOutput.varType);
		const char* precisionQual = GetPrecisionQualifier_(shaderOutput.precision);
		stringBuilder.appendf("layout(location = %" PRIu32 ") out %s %s %s", location, precisionQual, varTypeName, shaderOutput.name);
		if (shaderOutput.count != 1) {
			stringBuilder.appendf("[%" PRIu32 "]", shaderOutput.count);
		}
		stringBuilder.appendf(";\n");
	}

	static void GenerateUniform_(soul::String& stringBuilder, const ShaderUniform& shaderUniform) {
		stringBuilder.appendf("layout(set = %d, binding = %d, std140) uniform %s {\n", shaderUniform.set, shaderUniform.binding, shaderUniform.typeName);
		for (uint64 memberIndex = 0; memberIndex < shaderUniform.memberCount; memberIndex++) {
			const ShaderUniformMember& member = shaderUniform.members[memberIndex];
			const char* precision = GetPrecisionQualifier_(member.precision);
			const char* typeName = GetVarTypeName_(member.varType);
			stringBuilder.appendf("     %s %s %s", precision, typeName, member.name);
			if (member.count > 1) {
				stringBuilder.appendf("[%" PRIu32 "]", member.count);
			}
			stringBuilder.appendf(";\n");
		}
		stringBuilder.appendf("} %s;\n", shaderUniform.instanceName);
	}

	static void GenerateSampler_(soul::String& stringBuilder, const ShaderSampler& shaderSampler) {
		stringBuilder.appendf("layout(set = %d, binding = %d) uniform %s %s %s;\n",
			shaderSampler.set, shaderSampler.binding,
			GetPrecisionQualifier_(shaderSampler.precision), 
			GetSamplerTypeName_(shaderSampler.type, shaderSampler.format), 
			shaderSampler.name);
	}

	void ShaderGenerator::addShaderTemplates(const char* groupName, const char* path) {
		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			SOUL_LOG_INFO("Entry path = %s", entry.path().string().c_str());
			char* shaderCode = LoadFile(entry.path().string().c_str(), this->_allocator);
			
			const std::filesystem::path& fpath = entry.path();
			if (!std::filesystem::is_regular_file(fpath)) continue;
			std::string filename = entry.path().filename().string();

			soul::runtime::ScopeAllocator<> scopeAllocator("AddShaderTemplates");
			const uint64 templateKeyLength = strlen(groupName) + strlen("::") + strlen(filename.c_str());
			soul::String templateKey(&scopeAllocator, templateKeyLength + 1);
			templateKey.appendf("%s::%s", groupName, filename.c_str());

			const uint64 templateKeyHash = GetHash_(templateKey.data(), templateKeyLength);
			SOUL_ASSERT(0, _templateMap.empty() || !_templateMap.isExist(templateKeyHash), "There duplicate hash for shader template");

			_templateMap.add(templateKeyHash, shaderCode);
		}
	}

	soul::gpu::ShaderID ShaderGenerator::createShader(const ShaderDesc& shaderDesc) const {
		
		soul::String stringBuilder(_allocator, 10000);

		auto _generateNewline = [&stringBuilder]() {
			stringBuilder.appendf("\n");
		};

		GenerateProlog_(stringBuilder, shaderDesc.type);

		for (uint32 defineIdx = 0; defineIdx < shaderDesc.defineCount; defineIdx++) {
			GenerateDefine_(stringBuilder, shaderDesc.defines[defineIdx]);
		}
		_generateNewline();
		
		for (uint32 inputIdx = 0; inputIdx < soul::gpu::MAX_INPUT_PER_SHADER; inputIdx++) {
			const ShaderInput& input = shaderDesc.inputs[inputIdx];
			if (input.name == nullptr) continue;
			GenerateShaderInput_(stringBuilder, input, inputIdx);
		}
		_generateNewline();

		for (uint32 outputIdx = 0; outputIdx < soul::gpu::MAX_INPUT_PER_SHADER; outputIdx++) {
			const ShaderOutput& output = shaderDesc.outputs[outputIdx];
			if (output.name == nullptr) continue;
			GenerateShaderOutput_(stringBuilder, output, outputIdx);
		}
		_generateNewline();

		for (uint32 uniformIdx = 0; uniformIdx < shaderDesc.uniformCount; uniformIdx++) {
			GenerateUniform_(stringBuilder, shaderDesc.uniforms[uniformIdx]);
			_generateNewline();
		}
		_generateNewline();

		for (uint32 samplerIdx = 0; samplerIdx < shaderDesc.samplerCount; samplerIdx++) {
			GenerateSampler_(stringBuilder, shaderDesc.samplers[samplerIdx]);
		}
		_generateNewline();

		for (uint32 templateCodeIdx = 0; templateCodeIdx < shaderDesc.templateCodeCount; templateCodeIdx++) {
			const char* templateCodeKey = shaderDesc.templateCodes[templateCodeIdx];
			char* templateCode = _templateMap[GetHash_(templateCodeKey, strlen(templateCodeKey))];
			stringBuilder.appendf("%s\n", templateCode);
		}


		if (shaderDesc.customCode != nullptr) stringBuilder.appendf("%s\n", shaderDesc.customCode);

		static auto _getShaderStage = [](ShaderType shaderType) -> soul::gpu::ShaderStage {
			switch (shaderType) {
			case ShaderType::VERTEX: return soul::gpu::ShaderStage::VERTEX;
			case ShaderType::FRAGMENT: return soul::gpu::ShaderStage::FRAGMENT;
			case ShaderType::COUNT:
			default:
				SOUL_NOT_IMPLEMENTED(); return soul::gpu::ShaderStage::COUNT;
			}
		};
		
		return _gpuSystem->create_shader({ "default", stringBuilder.data(), uint32(stringBuilder.size())}, _getShaderStage(shaderDesc.type));
	}
}