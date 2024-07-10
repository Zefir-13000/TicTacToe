#include <Application.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	Application app;
	bool result;

	result = app.Initialize();
	if (!result) {
		OutputDebugString("Failed to init app.\n");
		return 1;
	}

	app.Run();
	app.Shutdown();

	return 0;
}
