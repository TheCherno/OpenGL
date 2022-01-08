#if 1

#include "MainLayer.h"

#include <iomanip>
#include <filesystem>
#include <fstream>
#include <commdlg.h>

ImVec2 operator+(const ImVec2& l, const ImVec2& r)
{
	return { l.x + r.x, l.y + r.y };
}

ImVec2 operator-(const ImVec2& l, const ImVec2& r)
{
	return { l.x - r.x, l.y - r.y };
}

ImVec2 operator*(const ImVec2& vec, float scalar)
{
	return { vec.x * scalar, vec.y * scalar };
}

std::ostream& operator<<(std::ostream& os, const ImVec2& vec)
{
	os << '(' << vec.x << ", " << vec.y << ')';
	return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const glm::vec<2, T>& vec)
{
	os << '(' << vec.x << ", " << vec.y << ')';
	return os;
}

template<typename T>
ImVec2 GlmToImVec(const glm::vec<2, T>& vec)
{
	return { (float)vec.x, (float)vec.y };
}

static void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

template<size_t file_size>
static bool SaveImageDialog(char(&fileName)[file_size])
{
	const char* filter = "PNG (*.png)\0*.png\0JPEG (*jpg; *jpeg)\0*.jpg;*.jpeg\0BMP (*.bmp)\0*.bmp\0TGA (*.tga)\0*.tga\0";

	OPENFILENAMEA ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	//ofn.hwndOwner = Application::Get().GetWindow().GetNativeWindow(); TODO ;-;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = sizeof(fileName);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	// Sets the default extension by extracting it from the filter
	ofn.lpstrDefExt = strchr(filter, '\0') + 1;

	return GetSaveFileNameA(&ofn) == TRUE;
}


void MainLayer::RefreshColorFunctions()
{
	// Crear previews colors
	m_colors.clear();

	for (auto prev : m_colorsPreview)
		glDeleteTextures(1, &prev);
	m_colorsPreview.clear();

	// Allocate new colors
	m_colors.reserve(10);
	for (const auto& path : std::filesystem::directory_iterator("assets/colors"))
	{
		std::ifstream colorSrc(path.path());
		m_colors.emplace_back(std::string((std::istreambuf_iterator<char>(colorSrc)), std::istreambuf_iterator<char>()));
	}

	// Allocate the prevews
	m_colorsPreview.reserve(m_colors.size());
	for (const auto& c : m_colors)
	{
		// Make the preview
		glm::uvec2 previewSize = { 100, 1 };
		{
			// Framebuffer
			GLuint fb;
			glGenFramebuffers(1, &fb);
			glBindFramebuffer(GL_FRAMEBUFFER, fb);

			GLuint tex;
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, previewSize.x, previewSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
			GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, buffers);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				std::cout << "NOT TOTOTOTOTOT\n";
				exit(EXIT_FAILURE);
			}

			// Shader
			std::stringstream ss;
			ss << "#version 400\n\n";
			ss << c.GetSource() << '\n';
			ss << R"(
layout (location = 0) out vec3 outColor;

uniform uint range;
uniform uvec2 size;

void main()
{
    int i = int((gl_FragCoord.x / size.x) * range);
    outColor = get_color(i);
}
		)";

			GLuint shader = GLCore::Utils::CreateShader(ss.str());
			glUseProgram(shader);
			GLint loc;

			loc = glGetUniformLocation(shader, "range");
			glUniform1ui(loc, 1000);

			loc = glGetUniformLocation(shader, "size");
			glUniform2ui(loc, previewSize.x, previewSize.y);

			for (const auto& u : c.GetUniforms())
			{
				loc = glGetUniformLocation(shader, u.name.c_str());
				glUniform1f(loc, u.default_val);
			}

			// Drawing
			glViewport(0, 0, previewSize.x, previewSize.y);
			glDisable(GL_BLEND);

			glBindVertexArray(GLCore::Application::GetDefaultQuadVA());
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

			// Cleaning up
			glDeleteFramebuffers(1, &fb);
			glDeleteProgram(shader);

			m_colorsPreview.push_back(tex);
		}
	}

	if (m_selectedColor >= m_colors.size())
		m_selectedColor = 0;

	m_Mandelbrot.SetColorFunction(&m_colors[m_selectedColor]);
}

MainLayer::MainLayer()
	: m_MandelbrotSrcPath("assets/mandelbrot.glsl")
	, m_Mandelbrot(m_MandelbrotSrcPath)
	, m_JuliaSrcPath("assets/julia.glsl")
	, m_Julia(m_JuliaSrcPath)
{
	RefreshColorFunctions();

	m_Mandelbrot.SetColorFunction(&m_colors[m_selectedColor]);
	m_Julia.SetColorFunction(&m_colors[m_selectedColor]);

	m_Mandelbrot.SetCenter({ -0.5, 0 });
	m_Julia.SetRadius(1.3);
}

MainLayer::~MainLayer()
{
	for (auto prev : m_colorsPreview)
		glDeleteTextures(1, &prev);
}

void MainLayer::OnUpdate(GLCore::Timestep ts)
{
	frame_rate = 1 / ts.GetSeconds();

	if (m_ShouldRefreshColors)
	{
		RefreshColorFunctions();
		m_ShouldRefreshColors = false;
	}

	GLint loc = glGetUniformLocation(m_Julia.GetShader(), "i_JuliaC");
	glUniform2d(loc, m_JuliaC.x, m_JuliaC.y);

	if (!m_MandelbrotMinimized)
		m_Mandelbrot.Update();

	if (!m_JuliaMinimized)
		m_Julia.Update();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MainLayer::OnImGuiRender()
{
	static bool dockspaceOpen = true;
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking; //| ImGuiWindowFlags_MenuBar;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
	ImGui::PopStyleVar();

	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
	}

	//ImGui::ShowDemoWindow();
	
	auto mouseDeltaScaled = io.MouseDelta * (m_ResolutionPercentage / 100.f);

	// Mandelbrot
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		m_MandelbrotMinimized = !ImGui::Begin("Mandelbrot");

		auto mousePos = ImGui::GetMousePos() - ImGui::GetWindowPos() - ImGui::GetWindowContentRegionMin();
		mousePos = mousePos * (m_ResolutionPercentage / 100.f);

		// Resize
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		ImVec2 viewportPanelSizeScaled = viewportPanelSize * (m_ResolutionPercentage / 100.f);
		{
			auto size = m_Mandelbrot.GetSize();
			if (viewportPanelSizeScaled.x != size.x || viewportPanelSizeScaled.y != size.y)
			{
				m_Mandelbrot.SetSize(glm::vec2{ viewportPanelSizeScaled.x, viewportPanelSizeScaled.y });
			}
		}

		// Events
		if (ImGui::IsWindowHovered())
		{
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || 
				(ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0) && (mouseDeltaScaled.x != 0 || mouseDeltaScaled.y != 0)))
			{
				m_JuliaC = m_Mandelbrot.MapCoordsToPos(mousePos);
				m_Julia.ResetRender();
			}

			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0) && mousePos.y >= 0)
			{
				if (mouseDeltaScaled.x != 0 || mouseDeltaScaled.y != 0)
				{
					glm::dvec2 center = m_Mandelbrot.MapCoordsToPos(m_Mandelbrot.MapPosToCoords(m_Mandelbrot.GetCenter()) - mouseDeltaScaled);
					m_Mandelbrot.SetCenter(center);
				}
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && io.KeyCtrl)
			{
				m_Mandelbrot.SetCenter(m_Mandelbrot.MapCoordsToPos(mousePos));
			}

			if (io.MouseWheel != 0)
			{
				glm::dvec2 iMousePos = m_Mandelbrot.MapCoordsToPos(mousePos);

				m_Mandelbrot.SetRadius(m_Mandelbrot.GetRadius() / std::pow(1.1f, io.MouseWheel));

				glm::dvec2 fMousePos = m_Mandelbrot.MapCoordsToPos(mousePos);

				glm::dvec2 delta = fMousePos - iMousePos;
				m_Mandelbrot.SetCenter(m_Mandelbrot.GetCenter() - delta);
			}
		}

		ImGui::Image((ImTextureID)(intptr_t)m_Mandelbrot.GetTexture(), viewportPanelSize, ImVec2{0, 1}, ImVec2{1, 0});

		ImGui::End();
		ImGui::PopStyleVar();
	}

	// Julia
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		m_JuliaMinimized = !ImGui::Begin("Julia");

		auto mousePos = ImGui::GetMousePos() - ImGui::GetWindowPos() - ImGui::GetWindowContentRegionMin();
		mousePos = mousePos * (m_ResolutionPercentage / 100.f);

		// Resize
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		ImVec2 viewportPanelSizeScaled = viewportPanelSize * (m_ResolutionPercentage / 100.f);
		{
			auto size = m_Julia.GetSize();
			if (viewportPanelSizeScaled.x != size.x || viewportPanelSizeScaled.y != size.y)
			{
				m_Julia.SetSize(glm::vec2{ viewportPanelSizeScaled.x, viewportPanelSizeScaled.y });
			}
		}

		// Events
		if (ImGui::IsWindowHovered())
		{
			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0) && mousePos.y >= 0)
			{
				if (mouseDeltaScaled.x != 0 || mouseDeltaScaled.y != 0)
				{
					glm::dvec2 center = m_Julia.MapCoordsToPos(m_Julia.MapPosToCoords(m_Julia.GetCenter()) - mouseDeltaScaled);
					m_Julia.SetCenter(center);
				}
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && io.KeyCtrl)
			{
				m_Julia.SetCenter(m_Julia.MapCoordsToPos(mousePos));
			}

			if (io.MouseWheel != 0)
			{
				glm::dvec2 iMousePos = m_Julia.MapCoordsToPos(mousePos);

				m_Julia.SetRadius(m_Julia.GetRadius() / std::pow(1.1f, io.MouseWheel));

				glm::dvec2 fMousePos = m_Julia.MapCoordsToPos(mousePos);

				glm::dvec2 delta = fMousePos - iMousePos;
				m_Julia.SetCenter(m_Julia.GetCenter() - delta);
			}
		}

		ImGui::Image((ImTextureID)(intptr_t)m_Julia.GetTexture(), viewportPanelSize, ImVec2{0, 1}, ImVec2{1, 0});

		ImGui::End();
		ImGui::PopStyleVar();
	}

	// Controls
	{
		auto windowBgCol = style.Colors[ImGuiCol_WindowBg];
		windowBgCol.w = 0.5f;
		ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBgCol);
		ImGui::Begin("Controls");


		std::stringstream ss;
		ss << std::fixed << std::setprecision(1) << frame_rate << "fps";

		ImGui::Text(ss.str().c_str());

		ImGui::Spacing();

		if (ImGui::CollapsingHeader("General"))
		{
			static int itersPerFrame = 100;
			if (ImGui::DragInt("Iterations per frame", &itersPerFrame, 10, 1, 10000, "%d", ImGuiSliderFlags_AlwaysClamp))
			{
				m_Mandelbrot.SetIterationsPerFrame(itersPerFrame);
				m_Julia.SetIterationsPerFrame(itersPerFrame);
			}

			if (ImGui::DragInt("Resolution percentage", &m_ResolutionPercentage, 1, 30, 500, "%d%%", ImGuiSliderFlags_AlwaysClamp))
			{
				glm::uvec2 mandelbrotSize = (glm::vec2)m_Mandelbrot.GetSize() * (m_ResolutionPercentage / 100.f);
				m_Mandelbrot.SetSize(mandelbrotSize);

				glm::uvec2 juliaSize = (glm::vec2)m_Mandelbrot.GetSize() * (m_ResolutionPercentage / 100.f);
				m_Julia.SetSize(juliaSize);
			}

			ImGui::Spacing();
		}

		if (ImGui::CollapsingHeader("Color function"))
		{

			if (ImGui::Button("Refresh"))
				m_ShouldRefreshColors = true;

			ImGui::SameLine(); HelpMarker("Edit the files (or add) in the 'assets/colors' folder and they will appear here after a refresh");

			ImVec2 button_size = { 100, 50 };
			float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
			for (size_t i = 0; i < m_colors.size(); i++)
			{
				ImGui::PushID((int)i);

				if (m_selectedColor == i)
					ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
				else
					ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);

				if (ImGui::ImageButton((ImTextureID)(intptr_t)m_colorsPreview[i], button_size))
				{
					m_selectedColor = i;
					m_Mandelbrot.SetColorFunction(&m_colors[m_selectedColor]);
					m_Julia.SetColorFunction(&m_colors[m_selectedColor]);
				}

				ImGui::PopStyleColor();

				float last_button_x2 = ImGui::GetItemRectMax().x;
				float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_size.x; // Expected position if next button was on same line

				if (i + 1 < m_colors.size() && next_button_x2 < window_visible_x2)
					ImGui::SameLine();

				ImGui::PopID();
			}

			ImGui::Spacing();
			ImGui::Text("Color function parameters");
			for (auto& u : m_colors[m_selectedColor].GetUniforms())
			{
				if (ImGui::DragFloat(u.name.c_str(), &u.val, 1, u.range.x, u.range.y))
				{
					m_Mandelbrot.ResetRender();
					m_Julia.ResetRender();
				}
			}

			ImGui::Spacing();
		}

		if (ImGui::CollapsingHeader("Mandelbrot"))
		{
			ImGui::PushID(0);

			if (ImGui::Button("Reload Mandelbrot Shader"))
				m_Mandelbrot.SetShader(m_MandelbrotSrcPath);

			double cmin = -2, cmax = 2;
			glm::dvec2 center = m_Mandelbrot.GetCenter();
			if (ImGui::DragScalarN("Center", ImGuiDataType_Double, glm::value_ptr(center), 2, (float)m_Mandelbrot.GetRadius() / 70.f, &cmin, &cmax, "%.15f"))
				m_Mandelbrot.SetCenter(center);

			double rmin = 1e-15, rmax = 50;
			double radius = m_Mandelbrot.GetRadius();
			if (ImGui::DragScalar("Radius", ImGuiDataType_Double, &radius, 0.01f, &rmin, &rmax, "%e", ImGuiSliderFlags_Logarithmic))
				m_Mandelbrot.SetRadius(radius);


			if (ImGui::Button("Screenshot"))
			{
				CHAR fileName[260];
				auto center = m_Mandelbrot.GetCenter();
				sprintf_s(fileName, "mandelbrot_%.15f,%.15f", center.x, center.y);

				if (SaveImageDialog(fileName))
					GLCore::Utils::ExportTexture(m_Mandelbrot.GetTexture(), fileName);
			}

			ImGui::Spacing();
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Julia"))
		{
			ImGui::PushID(1);

			if (ImGui::Button("Reload Julia Shader"))
				m_Julia.SetShader(m_JuliaSrcPath);

			double cmin = -2, cmax = 2;
			glm::dvec2 center = m_Julia.GetCenter();
			if (ImGui::DragScalarN("Center", ImGuiDataType_Double, glm::value_ptr(center), 2, (float)m_Julia.GetRadius() / 200.f, &cmin, &cmax, "%.15f"))
				m_Julia.SetCenter(center);

			double rmin = 1e-15, rmax = 50;
			double radius = m_Julia.GetRadius();
			if (ImGui::DragScalar("Radius", ImGuiDataType_Double, &radius, 0.01f, &rmin, &rmax, "%e", ImGuiSliderFlags_Logarithmic))
				m_Julia.SetRadius(radius);

			if (ImGui::DragScalarN("C value", ImGuiDataType_Double, glm::value_ptr(m_JuliaC), 2, (float)m_Julia.GetRadius() * 1e-5f, &cmin, &cmax, "%.15f"))
				m_Julia.ResetRender();


			if (ImGui::Button("Screenshot"))
			{
				CHAR fileName[260];
				sprintf_s(fileName, "julia_%.15f,%.15f", m_JuliaC.x, m_JuliaC.y);

				if (SaveImageDialog(fileName))
					GLCore::Utils::ExportTexture(m_Julia.GetTexture(), fileName);
			}

			ImGui::Spacing();
			ImGui::PopID();
		}

		ImGui::End(); // Controls
		ImGui::PopStyleColor();
	}

	ImGui::End(); // Dockspace
}


#endif