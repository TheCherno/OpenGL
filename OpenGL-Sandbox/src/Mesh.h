// "Copyright 2022 Tobias Onoufriou"
#pragma once
#include "GLCore.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>

struct MeshInfo {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texcoords;
  glm::vec3 tangent;
  glm::vec3 bitangent;
};

struct Texture {
  unsigned int id;
  std::string type;
  std::string path;
};

class Mesh{
 public:
  Mesh() {}
  bool Init();  // Call loadmodel, processnode and loadmaterialtexture.
  void Draw(GLuint shaderId);
  bool loadModel(const std::string&  path);
  bool processNode(aiNode* node, const aiScene* scene);
  bool loadMaterialTextures(
    aiMaterial* mat,
    aiTextureType type,
    std::string typeName);
 private:
  MeshInfo m_meshs;
  unsigned int m_indicies;
  Texture m_textures;
  unsigned int VBO, EBO;
};
