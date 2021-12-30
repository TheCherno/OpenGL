#include "GLCore.h"
#include "FractalLayer.h"

using namespace GLCore;

class FractalApp : public Application
{
public:
	FractalApp()
		: Application("Fractal Visualization")
	{
		PushLayer(new FractalLayer());
	}
};

int main()
{
	std::unique_ptr<FractalApp> app = std::make_unique<FractalApp>();
	app->Run();
}