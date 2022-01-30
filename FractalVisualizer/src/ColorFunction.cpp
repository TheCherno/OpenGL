#include "ColorFunction.h"

using namespace GLCore;
using namespace GLCore::Utils;

ColorFunction::ColorFunction(const std::string& src) : m_src(src)
{
	m_src = src;
	for (size_t start; (start = m_src.find("#uniform ")) != std::string::npos;)
	{
		size_t end = m_src.substr(start).find_first_of(';') + start;

		std::stringstream ss(m_src.substr(start + 9, end - start - 9));

		std::string name, min_s, max_s;
		float val, speed;
		ss >> name >> val >> speed >> min_s >> max_s;

		glm::vec2 range;
		range.x = (min_s == "NULL" ? FLT_MIN : std::stof(min_s));
		range.y = (max_s == "NULL" ? FLT_MAX : std::stof(max_s));

		std::stringstream uniform;
		uniform << "uniform float " << name;

		m_src.erase(start, end - start);
		m_src.insert(start, uniform.str());

		m_uniforms.emplace_back(name, range, val, speed);
	}
}

std::vector<ColorFunction::Uniform>& ColorFunction::GetUniforms()
{
	return m_uniforms;
}

const std::vector<ColorFunction::Uniform>& ColorFunction::GetUniforms() const
{
	return m_uniforms;
}

const std::string& ColorFunction::GetSource() const
{
	return m_src;
}
