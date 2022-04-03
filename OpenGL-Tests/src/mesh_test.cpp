// Copyright 2022 Tobias Onoufriou
#include "gtest/gtest.h"
#include "Mesh.h"
#include "Mesh.cpp"

TEST(MeshTest, LoadModelTrue) {
  Mesh mesh;
  EXPECT_TRUE(mesh.LoadModel("somepath"));
}
