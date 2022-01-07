#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>

#include "FractalVisualizer.h"
#include "ColorFunction.h"

class MainLayer : public GLCore::Layer
{
public:
	MainLayer();
	~MainLayer();

	//virtual void OnAttach() override;
	//virtual void OnDetach() override;
	virtual void OnUpdate(GLCore::Timestep ts) override;
	virtual void OnImGuiRender() override;
private:

	void RefreshColorFunctions();
	bool m_ShouldRefreshColors = false;

	float frame_rate = 0;
	int m_ResolutionPercentage = 100;

	std::vector<GLuint> m_colorsPreview;
	std::vector<ColorFunction> m_colors;
	size_t m_selectedColor = 0;
	
	//glm::dvec2 m_MandelbrotZ = { 0, 0 };
	bool m_MandelbrotMinimized = true;
	std::string m_MandelbrotSrcPath;
	FractalVisualizer m_Mandelbrot;

	glm::dvec2 m_JuliaC = { 0, 0 };
	bool m_JuliaMinimized = true;
	std::string m_JuliaSrcPath;
	FractalVisualizer m_Julia;
};

