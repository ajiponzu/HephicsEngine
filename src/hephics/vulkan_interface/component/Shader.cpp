#include "../Interface.hpp"

std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<vk_interface::component::Shader>>>
vk_interface::component::ShaderProvider::s_shaderDictionary;

void vk_interface::component::Shader::SetModule(const vk::UniqueDevice& logical_device,
	const vk::ShaderModuleCreateInfo& create_info)
{
	m_module = logical_device->createShaderModuleUnique(create_info);
}

static std::pair<std::string, ::EShLanguage> translate_shader_stage(const std::string& shader_code_path)
{
	if (shader_code_path.ends_with("vert"))
		return { "vert", EShLangVertex };
	else if (shader_code_path.ends_with("frag"))
		return { "frag", EShLangFragment };
	else if (shader_code_path.ends_with("comp"))
		return { "comp", EShLangCompute };
	else if (shader_code_path.ends_with("rgen"))
		return { "rgen", EShLangRayGen };
	else if (shader_code_path.ends_with("rmiss"))
		return { "rmiss", EShLangMiss };
	else if (shader_code_path.ends_with("rchit"))
		return { "rchit", EShLangClosestHit };
	else if (shader_code_path.ends_with("rahit"))
		return { "rahit", EShLangAnyHit };
	else
		throw std::runtime_error("Unknown shader stage");
}

static TBuiltInResource init_t_built_in_resources()
{
	TBuiltInResource resources{};

	resources.maxLights = 32;
	resources.maxClipPlanes = 6;
	resources.maxTextureUnits = 32;
	resources.maxTextureCoords = 32;
	resources.maxVertexAttribs = 64;
	resources.maxVertexUniformComponents = 4096;
	resources.maxVaryingFloats = 64;
	resources.maxVertexTextureImageUnits = 32;
	resources.maxCombinedTextureImageUnits = 80;
	resources.maxTextureImageUnits = 32;
	resources.maxFragmentUniformComponents = 4096;
	resources.maxDrawBuffers = 32;
	resources.maxVertexUniformVectors = 128;
	resources.maxVaryingVectors = 8;
	resources.maxFragmentUniformVectors = 16;
	resources.maxVertexOutputVectors = 16;
	resources.maxFragmentInputVectors = 15;
	resources.minProgramTexelOffset = -8;
	resources.maxProgramTexelOffset = 7;
	resources.maxClipDistances = 8;
	resources.maxComputeWorkGroupCountX = 65535;
	resources.maxComputeWorkGroupCountY = 65535;
	resources.maxComputeWorkGroupCountZ = 65535;
	resources.maxComputeWorkGroupSizeX = 1024;
	resources.maxComputeWorkGroupSizeY = 1024;
	resources.maxComputeWorkGroupSizeZ = 64;
	resources.maxComputeUniformComponents = 1024;
	resources.maxComputeTextureImageUnits = 16;
	resources.maxComputeImageUniforms = 8;
	resources.maxComputeAtomicCounters = 8;
	resources.maxComputeAtomicCounterBuffers = 1;
	resources.maxVaryingComponents = 60;
	resources.maxVertexOutputComponents = 64;
	resources.maxGeometryInputComponents = 64;
	resources.maxGeometryOutputComponents = 128;
	resources.maxFragmentInputComponents = 128;
	resources.maxImageUnits = 8;
	resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
	resources.maxCombinedShaderOutputResources = 8;
	resources.maxImageSamples = 0;
	resources.maxVertexImageUniforms = 0;
	resources.maxTessControlImageUniforms = 0;
	resources.maxTessEvaluationImageUniforms = 0;
	resources.maxGeometryImageUniforms = 0;
	resources.maxFragmentImageUniforms = 8;
	resources.maxCombinedImageUniforms = 8;
	resources.maxGeometryTextureImageUnits = 16;
	resources.maxGeometryOutputVertices = 256;
	resources.maxGeometryTotalOutputComponents = 1024;
	resources.maxGeometryUniformComponents = 1024;
	resources.maxGeometryVaryingComponents = 64;
	resources.maxTessControlInputComponents = 128;
	resources.maxTessControlOutputComponents = 128;
	resources.maxTessControlTextureImageUnits = 16;
	resources.maxTessControlUniformComponents = 1024;
	resources.maxTessControlTotalOutputComponents = 4096;
	resources.maxTessEvaluationInputComponents = 128;
	resources.maxTessEvaluationOutputComponents = 128;
	resources.maxTessEvaluationTextureImageUnits = 16;
	resources.maxTessEvaluationUniformComponents = 1024;
	resources.maxTessPatchComponents = 120;
	resources.maxPatchVertices = 32;
	resources.maxTessGenLevel = 64;
	resources.maxViewports = 16;
	resources.maxVertexAtomicCounters = 0;
	resources.maxTessControlAtomicCounters = 0;
	resources.maxTessEvaluationAtomicCounters = 0;
	resources.maxGeometryAtomicCounters = 0;
	resources.maxFragmentAtomicCounters = 8;
	resources.maxCombinedAtomicCounters = 8;
	resources.maxAtomicCounterBindings = 1;
	resources.maxVertexAtomicCounterBuffers = 0;
	resources.maxTessControlAtomicCounterBuffers = 0;
	resources.maxTessEvaluationAtomicCounterBuffers = 0;
	resources.maxGeometryAtomicCounterBuffers = 0;
	resources.maxFragmentAtomicCounterBuffers = 1;
	resources.maxCombinedAtomicCounterBuffers = 1;
	resources.maxAtomicCounterBufferSize = 16384;
	resources.maxTransformFeedbackBuffers = 4;
	resources.maxTransformFeedbackInterleavedComponents = 64;
	resources.maxCullDistances = 8;
	resources.maxCombinedClipAndCullDistances = 8;
	resources.maxSamples = 4;
	resources.maxMeshOutputVerticesNV = 256;
	resources.maxMeshOutputPrimitivesNV = 512;
	resources.maxMeshWorkGroupSizeX_NV = 32;
	resources.maxMeshWorkGroupSizeY_NV = 1;
	resources.maxMeshWorkGroupSizeZ_NV = 1;
	resources.maxTaskWorkGroupSizeX_NV = 32;
	resources.maxTaskWorkGroupSizeY_NV = 1;
	resources.maxTaskWorkGroupSizeZ_NV = 1;
	resources.maxMeshViewCountNV = 4;

	resources.limits.nonInductiveForLoops = 1;
	resources.limits.whileLoops = 1;
	resources.limits.doWhileLoops = 1;
	resources.limits.generalUniformIndexing = 1;
	resources.limits.generalAttributeMatrixVectorIndexing = 1;
	resources.limits.generalVaryingIndexing = 1;
	resources.limits.generalSamplerIndexing = 1;
	resources.limits.generalVariableIndexing = 1;
	resources.limits.generalConstantMatrixVectorIndexing = 1;

	return resources;
}

static auto compile_shader(const ::EShLanguage& shader_stage, const std::string& shader_code)
{
	glslang::InitializeProcess();

	std::vector shader_c_strings = { shader_code.data() };

	glslang::TShader shader(shader_stage);
	shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv,
		glslang::EShTargetLanguageVersion::EShTargetSpv_1_5);
	shader.setStrings(shader_c_strings.data(), static_cast<int32_t>(shader_c_strings.size()));

	EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
	const auto t_built_in_resources = init_t_built_in_resources();
	if (!shader.parse(&t_built_in_resources, 100, false, messages))
		throw std::runtime_error(shader_code + "\n" + shader.getInfoLog());

	glslang::TProgram program;
	program.addShader(&shader);

	if (!program.link(messages))
		throw std::runtime_error(shader_code + "\n" + shader.getInfoLog());

	std::vector<uint32_t> spv_binary;
	glslang::GlslangToSpv(*program.getIntermediate(shader_stage), spv_binary);
	glslang::FinalizeProcess();

	return spv_binary;
}

void vk_interface::component::ShaderProvider::AddShader(const vk::UniqueDevice& logical_device,
	const std::string& shader_code_path, const std::string& shader_key)
{
	const auto [shader_type_str, shader_stage] = translate_shader_stage(shader_code_path);

	if (!s_shaderDictionary.contains(shader_type_str))
		s_shaderDictionary[shader_type_str] = {};

	if (s_shaderDictionary.at(shader_type_str).contains(shader_key))
		return;

	std::ifstream ifs(std::format("assets/shader/{}", shader_code_path));
	if (!ifs.is_open())
		throw std::runtime_error("Failed to open file: " + shader_code_path);

	std::stringstream buffer;
	buffer << ifs.rdbuf();

	const auto spv_binary = compile_shader(shader_stage, buffer.str());
	vk::ShaderModuleCreateInfo create_info;
	create_info.setCode(spv_binary);

	Shader shader;
	shader.SetModule(logical_device, create_info);
	s_shaderDictionary.at(shader_type_str).emplace(shader_key, std::make_shared<Shader>(std::move(shader)));
}

const std::shared_ptr<vk_interface::component::Shader>&
vk_interface::component::ShaderProvider::GetShader(const std::string& shader_type_key, const std::string& shader_key)
{
	if (!s_shaderDictionary.contains(shader_type_key))
		throw std::runtime_error("shader: not found");

	if (!(s_shaderDictionary.at(shader_type_key).contains(shader_key)))
		throw std::runtime_error("shader: not found");

	return s_shaderDictionary.at(shader_type_key).at(shader_key);
}