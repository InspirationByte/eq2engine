#pragma once

static constexpr EqStringRef s_outputTag = "%OUTPUT%";
static constexpr EqStringRef s_argumentsTag = "%ARGS%";
static constexpr EqStringRef s_inputFileNameTag = "%INPUT_FILENAME%";
static constexpr EqStringRef s_outputFileNameTag = "%OUTPUT_FILENAME%";
static constexpr EqStringRef s_outputFilePathTag = "%OUTPUT_FILEPATH%";
static constexpr EqStringRef s_engineDirTag ="%ENGINE_DIR%";
static constexpr EqStringRef s_gameDirTag = "%GAME_DIR%";

static constexpr EqStringRef s_sourceTextureFileExt = "tga";
static constexpr EqStringRef s_materialAtlasFileExt = "atlas";
static constexpr EqStringRef s_materialFileExt = "mat";
static constexpr EqStringRef s_atlasFileExt = "atl";

void ProcessAtlasFile(const char* atlasSrcFileName, const char* materialsPath);
void CookTarget(const char* pszTargetName);;