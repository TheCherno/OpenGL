#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>

#include <vector>

class ColorFunction
{
public:
	struct Uniform
	{
		std::string name;
		glm::vec2 range;
		float default_val;
		float val;

		Uniform(const std::string& name, const glm::vec2& range, float default_val)
			: name(name), range(range), default_val(default_val), val(default_val) {}
	};

private:
	std::vector<Uniform> m_uniforms;
	std::string m_src;

public:
	ImTextureID preview;

	ColorFunction(const std::string& src) : m_src(src) {}

	ColorFunction& AddUniform(const Uniform& uniform)
	{
		m_uniforms.push_back(uniform);
		return *this;
	}

	ColorFunction& AddUniform(const std::string& name, const glm::vec2& range, float default_val)
	{
		m_uniforms.emplace_back(name, range, default_val);
		return *this;
	}

	std::vector<Uniform>& GetUniforms()
	{
		return m_uniforms;
	}

	const std::vector<Uniform>& GetUniforms() const
	{
		return m_uniforms;
	}

	const std::string& GetSource() const
	{
		return m_src;
	}
};

class FractalLayer : public GLCore::Layer
{
public:
	FractalLayer();
	virtual ~FractalLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnEvent(GLCore::Event& event) override;
	virtual void OnUpdate(GLCore::Timestep ts) override;
	virtual void OnImGuiRender() override;
private:

	glm::dvec2 MapPosToCoords(const glm::dvec2& pos);
	glm::dvec2 MapCoordsToPos(const glm::dvec2& coords);

	void UpdateRange();

	void CreateFrameBuffer();

	void SetColorFunc(const ColorFunction& colorFunc);

	float frame_rate;

	std::vector<ColorFunction> m_colors;
	size_t m_selectedColor = 0;

	std::string m_coreShaderSrc;
	GLuint m_Shader = 0;

	GLuint m_QuadVA, m_QuadVB, m_QuadIB;

	GLuint m_FrameBuffer, m_Texture;
	GLuint m_InData, m_OutData, m_InIter, m_OutIter;

	glm::uvec2 m_size;
	glm::dvec2 m_center;
	double m_radius;
	int m_itersPerFrame;
	int m_frame;

	glm::dvec2 m_xRange;
	glm::dvec2 m_yRange;

	bool m_mousePressed = false;
	glm::dvec2 m_startPos;
};