//
// Created by chaku on 23/09/23.
//

#ifndef STABBY2D_ASSETMANAGER_HPP
#define STABBY2D_ASSETMANAGER_HPP

#include <unordered_map>
#include <memory>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

class AssetManager
{
private:
  std::unordered_map<std::string, SDL_Texture*> textures;

public:
  void ClearAssets();
  void AddTexture(const std::string& name, const std::string& filePath, SDL_Renderer* renderer);
  SDL_Texture*& GetTexture(const std::string& key);
};


#endif// STABBY2D_ASSETMANAGER_HPP
