#pragma once

#include "UnityEngine/Texture2D.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"

std::string GetLevelHash(GlobalNamespace::CustomPreviewBeatmapLevel* level);

std::string SanitizeFileName(std::string fileName);

bool UniqueFileName(std::string fileName, std::string compareDirectory);

std::string GetBase64ImageType(std::string& base64);

std::string ProcessImage(UnityEngine::Texture2D* texture, bool returnPngString);

void WriteImageToFile(std::string_view pathToPng, UnityEngine::Texture2D* texture);