#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>

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
	//GLuint preview;

public:

	ColorFunction(const std::string& src);

	//~ColorFunction();

	std::vector<Uniform>& GetUniforms();

	const std::vector<Uniform>& GetUniforms() const;

	const std::string& GetSource() const;

	//ImTextureID GetPreview() const;
};