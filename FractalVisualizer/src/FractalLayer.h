#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>

#include "ColorFunction.h"


class FractalLayer : public GLCore::Layer
{
public:
	FractalLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnEvent(GLCore::Event& event) override;
	virtual void OnUpdate(GLCore::Timestep ts) override;
	virtual void OnImGuiRender() override;
private:

	glm::dvec2 MapPosToCoords(const glm::dvec2& pos);
	glm::dvec2 MapCoordsToPos(const glm::dvec2& coords);

	void UpdateRange();

	void CleanOpenGL();
	void CreateFrameBuffer();

	void SetColorFunc(const ColorFunction& colorFunc);
	void RefreshColorFunctions();

	glm::uvec2 GetMultipliedResolution();

	// Color function
	std::vector<ColorFunction> m_colors;
	size_t m_selectedColor = 0;

	// Core shader
	std::string m_coreShaderSrc;
	GLuint m_Shader = 0;

	// Default quad
	GLuint m_QuadVA, m_QuadVB, m_QuadIB;

	// Textures
	GLuint m_FrameBuffer, m_Texture;
	GLuint m_InData, m_OutData, m_InIter, m_OutIter;


	// Fractal parameters
	glm::dvec2 m_center;
	double m_radius;
	int m_itersPerFrame;
	int m_frame;
	glm::uvec2 m_size;
	int m_resolutionPercentage = 100;

	glm::dvec2 m_xRange;
	glm::dvec2 m_yRange;

	// Window and iteraction
	glm::uvec2 m_screenSize;
	bool m_mousePressed = false;
	glm::dvec2 m_startPos;
	bool m_minimized = false;

	float frame_rate;
	bool m_shouldRefreshColors = true;
};