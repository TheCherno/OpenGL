// "Copyright 2022 Tobias Onoufriou"
#include "Mesh.h"

Mesh::Mesh() {}

Mesh::~Mesh() {}

bool Mesh::Init() {
  return false;
}

// Draw will be added to the update function.
void Mesh::Draw(GLuint shaderId) {
  // get uniform location.
}

bool Mesh::LoadModel(const std::string& path) {
  return true;
}
