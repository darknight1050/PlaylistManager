#pragma once

#include "UnityEngine/Texture2D.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"

std::string GetLevelHash(GlobalNamespace::CustomPreviewBeatmapLevel* level);

std::string SanitizeFileName(std::string const& fileName);

bool UniqueFileName(std::string const& fileName, std::string const& compareDirectory);

std::string GetNewPlaylistPath(std::string const& title);

std::string GetBase64ImageType(std::string const& base64);

std::string ProcessImage(UnityEngine::Texture2D* texture, bool returnPngString);

void WriteImageToFile(std::string const& pathToPng, UnityEngine::Texture2D* texture);