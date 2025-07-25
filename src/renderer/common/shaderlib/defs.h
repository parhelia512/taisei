/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

// IWYU pragma: private, include "renderer/common/shaderlib/shaderlib.h"

typedef enum ShaderStage {
	SHADER_STAGE_INVALID,
	SHADER_STAGE_VERTEX,
	SHADER_STAGE_FRAGMENT,
} ShaderStage;

typedef enum ShaderLanguage {
	SHLANG_INVALID,
	SHLANG_GLSL,
	SHLANG_SPIRV,
	SHLANG_HLSL,
	SHLANG_DXBC,
	SHLANG_MSL,

	SHLANG_FIRST = SHLANG_INVALID + 1,
} ShaderLanguage;

typedef struct ShaderMacro {
	const char *name;
	const char *value;
} ShaderMacro;

typedef struct ShaderSource ShaderSource;
typedef struct ShaderLangInfo ShaderLangInfo;
typedef struct ShaderReflection ShaderReflection;
