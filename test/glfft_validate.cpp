/* Copyright (C) 2015 Hans-Kristian Arntzen <maister@archlinux.us>
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "glfft_validate.hpp"
#include "ShaderLang.h"
#include "GlslangToSpv.h"
#include "doc.h"
#include "disassemble.h"
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdlib>

using namespace GLFFT;
using namespace glslang;
using namespace std;

struct SlangProcess
{
    public:
        SlangProcess() { InitializeProcess(); }
        ~SlangProcess() { FinalizeProcess(); }
};

// Shameless copy-pasta from glslang/StandAlone :)
static void set_default_resources(TBuiltInResource &Resources)
{
    char DefaultConfig[] =
        "MaxLights 32\n"
        "MaxClipPlanes 6\n"
        "MaxTextureUnits 32\n"
        "MaxTextureCoords 32\n"
        "MaxVertexAttribs 64\n"
        "MaxVertexUniformComponents 4096\n"
        "MaxVaryingFloats 64\n"
        "MaxVertexTextureImageUnits 32\n"
        "MaxCombinedTextureImageUnits 80\n"
        "MaxTextureImageUnits 32\n"
        "MaxFragmentUniformComponents 4096\n"
        "MaxDrawBuffers 32\n"
        "MaxVertexUniformVectors 128\n"
        "MaxVaryingVectors 8\n"
        "MaxFragmentUniformVectors 16\n"
        "MaxVertexOutputVectors 16\n"
        "MaxFragmentInputVectors 15\n"
        "MinProgramTexelOffset -8\n"
        "MaxProgramTexelOffset 7\n"
        "MaxClipDistances 8\n"
        "MaxComputeWorkGroupCountX 65535\n"
        "MaxComputeWorkGroupCountY 65535\n"
        "MaxComputeWorkGroupCountZ 65535\n"
        "MaxComputeWorkGroupSizeX 1024\n"
        "MaxComputeWorkGroupSizeY 1024\n"
        "MaxComputeWorkGroupSizeZ 64\n"
        "MaxComputeUniformComponents 1024\n"
        "MaxComputeTextureImageUnits 16\n"
        "MaxComputeImageUniforms 8\n"
        "MaxComputeAtomicCounters 8\n"
        "MaxComputeAtomicCounterBuffers 1\n"
        "MaxVaryingComponents 60\n" 
        "MaxVertexOutputComponents 64\n"
        "MaxGeometryInputComponents 64\n"
        "MaxGeometryOutputComponents 128\n"
        "MaxFragmentInputComponents 128\n"
        "MaxImageUnits 8\n"
        "MaxCombinedImageUnitsAndFragmentOutputs 8\n"
        "MaxCombinedShaderOutputResources 8\n"
        "MaxImageSamples 0\n"
        "MaxVertexImageUniforms 0\n"
        "MaxTessControlImageUniforms 0\n"
        "MaxTessEvaluationImageUniforms 0\n"
        "MaxGeometryImageUniforms 0\n"
        "MaxFragmentImageUniforms 8\n"
        "MaxCombinedImageUniforms 8\n"
        "MaxGeometryTextureImageUnits 16\n"
        "MaxGeometryOutputVertices 256\n"
        "MaxGeometryTotalOutputComponents 1024\n"
        "MaxGeometryUniformComponents 1024\n"
        "MaxGeometryVaryingComponents 64\n"
        "MaxTessControlInputComponents 128\n"
        "MaxTessControlOutputComponents 128\n"
        "MaxTessControlTextureImageUnits 16\n"
        "MaxTessControlUniformComponents 1024\n"
        "MaxTessControlTotalOutputComponents 4096\n"
        "MaxTessEvaluationInputComponents 128\n"
        "MaxTessEvaluationOutputComponents 128\n"
        "MaxTessEvaluationTextureImageUnits 16\n"
        "MaxTessEvaluationUniformComponents 1024\n"
        "MaxTessPatchComponents 120\n"
        "MaxPatchVertices 32\n"
        "MaxTessGenLevel 64\n"
        "MaxViewports 16\n"
        "MaxVertexAtomicCounters 0\n"
        "MaxTessControlAtomicCounters 0\n"
        "MaxTessEvaluationAtomicCounters 0\n"
        "MaxGeometryAtomicCounters 0\n"
        "MaxFragmentAtomicCounters 8\n"
        "MaxCombinedAtomicCounters 8\n"
        "MaxAtomicCounterBindings 1\n"
        "MaxVertexAtomicCounterBuffers 0\n"
        "MaxTessControlAtomicCounterBuffers 0\n"
        "MaxTessEvaluationAtomicCounterBuffers 0\n"
        "MaxGeometryAtomicCounterBuffers 0\n"
        "MaxFragmentAtomicCounterBuffers 1\n"
        "MaxCombinedAtomicCounterBuffers 1\n"
        "MaxAtomicCounterBufferSize 16384\n"
        "MaxTransformFeedbackBuffers 4\n"
        "MaxTransformFeedbackInterleavedComponents 64\n"
        "MaxCullDistances 8\n"
        "MaxCombinedClipAndCullDistances 8\n"
        "MaxSamples 4\n"

        "nonInductiveForLoops 1\n"
        "whileLoops 1\n"
        "doWhileLoops 1\n"
        "generalUniformIndexing 1\n"
        "generalAttributeMatrixVectorIndexing 1\n"
        "generalVaryingIndexing 1\n"
        "generalSamplerIndexing 1\n"
        "generalVariableIndexing 1\n"
        "generalConstantMatrixVectorIndexing 1\n";

    const char *delims = " \t\n\r";
    const char *token = strtok(DefaultConfig, delims);
    while (token)
    {
        const char *value_str = strtok(0, delims);
        int value = strtoul(value_str, nullptr, 0);

        if (strcmp(token, "MaxLights") == 0)
            Resources.maxLights = value;
        else if (strcmp(token, "MaxClipPlanes") == 0)
            Resources.maxClipPlanes = value;
        else if (strcmp(token, "MaxTextureUnits") == 0)
            Resources.maxTextureUnits = value;
        else if (strcmp(token, "MaxTextureCoords") == 0)
            Resources.maxTextureCoords = value;
        else if (strcmp(token, "MaxVertexAttribs") == 0)
            Resources.maxVertexAttribs = value;
        else if (strcmp(token, "MaxVertexUniformComponents") == 0)
            Resources.maxVertexUniformComponents = value;
        else if (strcmp(token, "MaxVaryingFloats") == 0)
            Resources.maxVaryingFloats = value;
        else if (strcmp(token, "MaxVertexTextureImageUnits") == 0)
            Resources.maxVertexTextureImageUnits = value;
        else if (strcmp(token, "MaxCombinedTextureImageUnits") == 0)
            Resources.maxCombinedTextureImageUnits = value;
        else if (strcmp(token, "MaxTextureImageUnits") == 0)
            Resources.maxTextureImageUnits = value;
        else if (strcmp(token, "MaxFragmentUniformComponents") == 0)
            Resources.maxFragmentUniformComponents = value;
        else if (strcmp(token, "MaxDrawBuffers") == 0)
            Resources.maxDrawBuffers = value;
        else if (strcmp(token, "MaxVertexUniformVectors") == 0)
            Resources.maxVertexUniformVectors = value;
        else if (strcmp(token, "MaxVaryingVectors") == 0)
            Resources.maxVaryingVectors = value;
        else if (strcmp(token, "MaxFragmentUniformVectors") == 0)
            Resources.maxFragmentUniformVectors = value;
        else if (strcmp(token, "MaxVertexOutputVectors") == 0)
            Resources.maxVertexOutputVectors = value;
        else if (strcmp(token, "MaxFragmentInputVectors") == 0)
            Resources.maxFragmentInputVectors = value;
        else if (strcmp(token, "MinProgramTexelOffset") == 0)
            Resources.minProgramTexelOffset = value;
        else if (strcmp(token, "MaxProgramTexelOffset") == 0)
            Resources.maxProgramTexelOffset = value;
        else if (strcmp(token, "MaxClipDistances") == 0)
            Resources.maxClipDistances = value;
        else if (strcmp(token, "MaxComputeWorkGroupCountX") == 0)
            Resources.maxComputeWorkGroupCountX = value;
        else if (strcmp(token, "MaxComputeWorkGroupCountY") == 0)
            Resources.maxComputeWorkGroupCountY = value;
        else if (strcmp(token, "MaxComputeWorkGroupCountZ") == 0)
            Resources.maxComputeWorkGroupCountZ = value;
        else if (strcmp(token, "MaxComputeWorkGroupSizeX") == 0)
            Resources.maxComputeWorkGroupSizeX = value;
        else if (strcmp(token, "MaxComputeWorkGroupSizeY") == 0)
            Resources.maxComputeWorkGroupSizeY = value;
        else if (strcmp(token, "MaxComputeWorkGroupSizeZ") == 0)
            Resources.maxComputeWorkGroupSizeZ = value;
        else if (strcmp(token, "MaxComputeUniformComponents") == 0)
            Resources.maxComputeUniformComponents = value;
        else if (strcmp(token, "MaxComputeTextureImageUnits") == 0)
            Resources.maxComputeTextureImageUnits = value;
        else if (strcmp(token, "MaxComputeImageUniforms") == 0)
            Resources.maxComputeImageUniforms = value;
        else if (strcmp(token, "MaxComputeAtomicCounters") == 0)
            Resources.maxComputeAtomicCounters = value;
        else if (strcmp(token, "MaxComputeAtomicCounterBuffers") == 0)
            Resources.maxComputeAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxVaryingComponents") == 0)
            Resources.maxVaryingComponents = value;
        else if (strcmp(token, "MaxVertexOutputComponents") == 0)
            Resources.maxVertexOutputComponents = value;
        else if (strcmp(token, "MaxGeometryInputComponents") == 0)
            Resources.maxGeometryInputComponents = value;
        else if (strcmp(token, "MaxGeometryOutputComponents") == 0)
            Resources.maxGeometryOutputComponents = value;
        else if (strcmp(token, "MaxFragmentInputComponents") == 0)
            Resources.maxFragmentInputComponents = value;
        else if (strcmp(token, "MaxImageUnits") == 0)
            Resources.maxImageUnits = value;
        else if (strcmp(token, "MaxCombinedImageUnitsAndFragmentOutputs") == 0)
            Resources.maxCombinedImageUnitsAndFragmentOutputs = value;
        else if (strcmp(token, "MaxCombinedShaderOutputResources") == 0)
            Resources.maxCombinedShaderOutputResources = value;
        else if (strcmp(token, "MaxImageSamples") == 0)
            Resources.maxImageSamples = value;
        else if (strcmp(token, "MaxVertexImageUniforms") == 0)
            Resources.maxVertexImageUniforms = value;
        else if (strcmp(token, "MaxTessControlImageUniforms") == 0)
            Resources.maxTessControlImageUniforms = value;
        else if (strcmp(token, "MaxTessEvaluationImageUniforms") == 0)
            Resources.maxTessEvaluationImageUniforms = value;
        else if (strcmp(token, "MaxGeometryImageUniforms") == 0)
            Resources.maxGeometryImageUniforms = value;
        else if (strcmp(token, "MaxFragmentImageUniforms") == 0)
            Resources.maxFragmentImageUniforms = value;
        else if (strcmp(token, "MaxCombinedImageUniforms") == 0)
            Resources.maxCombinedImageUniforms = value;
        else if (strcmp(token, "MaxGeometryTextureImageUnits") == 0)
            Resources.maxGeometryTextureImageUnits = value;
        else if (strcmp(token, "MaxGeometryOutputVertices") == 0)
            Resources.maxGeometryOutputVertices = value;
        else if (strcmp(token, "MaxGeometryTotalOutputComponents") == 0)
            Resources.maxGeometryTotalOutputComponents = value;
        else if (strcmp(token, "MaxGeometryUniformComponents") == 0)
            Resources.maxGeometryUniformComponents = value;
        else if (strcmp(token, "MaxGeometryVaryingComponents") == 0)
            Resources.maxGeometryVaryingComponents = value;
        else if (strcmp(token, "MaxTessControlInputComponents") == 0)
            Resources.maxTessControlInputComponents = value;
        else if (strcmp(token, "MaxTessControlOutputComponents") == 0)
            Resources.maxTessControlOutputComponents = value;
        else if (strcmp(token, "MaxTessControlTextureImageUnits") == 0)
            Resources.maxTessControlTextureImageUnits = value;
        else if (strcmp(token, "MaxTessControlUniformComponents") == 0)
            Resources.maxTessControlUniformComponents = value;
        else if (strcmp(token, "MaxTessControlTotalOutputComponents") == 0)
            Resources.maxTessControlTotalOutputComponents = value;
        else if (strcmp(token, "MaxTessEvaluationInputComponents") == 0)
            Resources.maxTessEvaluationInputComponents = value;
        else if (strcmp(token, "MaxTessEvaluationOutputComponents") == 0)
            Resources.maxTessEvaluationOutputComponents = value;
        else if (strcmp(token, "MaxTessEvaluationTextureImageUnits") == 0)
            Resources.maxTessEvaluationTextureImageUnits = value;
        else if (strcmp(token, "MaxTessEvaluationUniformComponents") == 0)
            Resources.maxTessEvaluationUniformComponents = value;
        else if (strcmp(token, "MaxTessPatchComponents") == 0)
            Resources.maxTessPatchComponents = value;
        else if (strcmp(token, "MaxPatchVertices") == 0)
            Resources.maxPatchVertices = value;
        else if (strcmp(token, "MaxTessGenLevel") == 0)
            Resources.maxTessGenLevel = value;
        else if (strcmp(token, "MaxViewports") == 0)
            Resources.maxViewports = value;
        else if (strcmp(token, "MaxVertexAtomicCounters") == 0)
            Resources.maxVertexAtomicCounters = value;
        else if (strcmp(token, "MaxTessControlAtomicCounters") == 0)
            Resources.maxTessControlAtomicCounters = value;
        else if (strcmp(token, "MaxTessEvaluationAtomicCounters") == 0)
            Resources.maxTessEvaluationAtomicCounters = value;
        else if (strcmp(token, "MaxGeometryAtomicCounters") == 0)
            Resources.maxGeometryAtomicCounters = value;
        else if (strcmp(token, "MaxFragmentAtomicCounters") == 0)
            Resources.maxFragmentAtomicCounters = value;
        else if (strcmp(token, "MaxCombinedAtomicCounters") == 0)
            Resources.maxCombinedAtomicCounters = value;
        else if (strcmp(token, "MaxAtomicCounterBindings") == 0)
            Resources.maxAtomicCounterBindings = value;
        else if (strcmp(token, "MaxVertexAtomicCounterBuffers") == 0)
            Resources.maxVertexAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxTessControlAtomicCounterBuffers") == 0)
            Resources.maxTessControlAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxTessEvaluationAtomicCounterBuffers") == 0)
            Resources.maxTessEvaluationAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxGeometryAtomicCounterBuffers") == 0)
            Resources.maxGeometryAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxFragmentAtomicCounterBuffers") == 0)
            Resources.maxFragmentAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxCombinedAtomicCounterBuffers") == 0)
            Resources.maxCombinedAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxAtomicCounterBufferSize") == 0)
            Resources.maxAtomicCounterBufferSize = value;
        else if (strcmp(token, "MaxTransformFeedbackBuffers") == 0)
            Resources.maxTransformFeedbackBuffers = value;
        else if (strcmp(token, "MaxTransformFeedbackInterleavedComponents") == 0)
            Resources.maxTransformFeedbackInterleavedComponents = value;
        else if (strcmp(token, "MaxCullDistances") == 0)
            Resources.maxCullDistances = value;
        else if (strcmp(token, "MaxCombinedClipAndCullDistances") == 0)
            Resources.maxCombinedClipAndCullDistances = value;
        else if (strcmp(token, "MaxSamples") == 0)
            Resources.maxSamples = value;
        else if (strcmp(token, "nonInductiveForLoops") == 0)
            Resources.limits.nonInductiveForLoops = (value != 0);
        else if (strcmp(token, "whileLoops") == 0)
            Resources.limits.whileLoops = (value != 0);
        else if (strcmp(token, "doWhileLoops") == 0)
            Resources.limits.doWhileLoops = (value != 0);
        else if (strcmp(token, "generalUniformIndexing") == 0)
            Resources.limits.generalUniformIndexing = (value != 0);
        else if (strcmp(token, "generalAttributeMatrixVectorIndexing") == 0)
            Resources.limits.generalAttributeMatrixVectorIndexing = (value != 0);
        else if (strcmp(token, "generalVaryingIndexing") == 0)
            Resources.limits.generalVaryingIndexing = (value != 0);
        else if (strcmp(token, "generalSamplerIndexing") == 0)
            Resources.limits.generalSamplerIndexing = (value != 0);
        else if (strcmp(token, "generalVariableIndexing") == 0)
            Resources.limits.generalVariableIndexing = (value != 0);
        else if (strcmp(token, "generalConstantMatrixVectorIndexing") == 0)
            Resources.limits.generalConstantMatrixVectorIndexing = (value != 0);

        token = strtok(0, delims);
    }
}

bool GLFFT::validate_glsl_source(const char *source)
{
    static SlangProcess process;

    string final_source = "#version 310 es\n";
    final_source += source;
    source = final_source.c_str();

    TBuiltInResource resources;
    set_default_resources(resources);

    TProgram program;
    EShLanguage language = EShLangCompute;
    TShader shader(language);

    shader.setStrings(&source, 1);

    string msg;
    if (!shader.preprocess(&resources, 100, ENoProfile, false, false, EShMsgDefault, &msg, TShader::ForbidInclude()))
        return false;
    if (!shader.parse(&resources, 100, false, EShMsgDefault))
        return false;

    program.addShader(&shader);
    if (!program.link(EShMsgDefault))
        return false;

    vector<uint32_t> spirv;
    GlslangToSpv(*program.getIntermediate(EShLangCompute), spirv);
    spv::Parameterize();
    spv::Disassemble(std::cout, spirv);
    return true;
}

