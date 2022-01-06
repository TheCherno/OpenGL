#include "GLCore.h"
#include "MainLayer.h"

using namespace GLCore;

class FractalApp : public Application
{
public:
	FractalApp()
		: Application("Fractal Visualization")
	{
		PushLayer(new MainLayer());
	}
};

int main()
{
	std::unique_ptr<FractalApp> app = std::make_unique<FractalApp>();
	app->Run();
}