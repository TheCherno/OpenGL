#include "SandboxLayer.h"
#include "Mesh.h"

using namespace GLCore;
using namespace GLCore::Utils;

SandboxLayer::SandboxLayer()
{
}

SandboxLayer::~SandboxLayer()
{
}

void SandboxLayer::OnAttach()
{
	EnableGLDebugging();
  Mesh* mesh = new Mesh();
  mesh->LoadModel("sompath");
	// In this function all call for model and file name is needed
	// Init here
}

void SandboxLayer::OnDetach()
{
	// Shutdown here
}

void SandboxLayer::OnEvent(Event& event)
{
	// Events here
}

void SandboxLayer::OnUpdate(Timestep ts)
{
	// Render here
}

void SandboxLayer::OnImGuiRender()
{
	// ImGui here
}
