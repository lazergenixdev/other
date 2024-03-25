#include "Graphics.h"

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR lpCmdLine, INT nCmdShow)
{
	Window MainWindow( 1000u, 700u, L"Σlectrîc ƒiεlδ" );
	Graphics gfx( MainWindow );

	static constexpr float delta = 5.0f;
	bool spaceHold = false;

	while( MainWindow.Run() )
	{
		float dx = 0, dy = 0;
		gfx.BeginFrame( 0xFF );

		if( (GetKeyState(VK_UP) & 0x80) )
		{
			dy -= delta;
		}
		if ((GetKeyState(VK_DOWN) & 0x80))
		{
			dy += delta;
		}
		if ((GetKeyState(VK_LEFT) & 0x80))
		{
			dx -= delta;
		}
		if ((GetKeyState(VK_RIGHT) & 0x80))
		{
			dx += delta;
		}
		if ((GetKeyState(VK_SPACE) & 0x80))
		{
			if (spaceHold == false)
			{
				spaceHold = true;
				gfx.TestSwitchSelected();
			}
		}
		else
		{
			if (spaceHold == true)
			{
				spaceHold = false;
			}
		}
		gfx.TestMoveSelected(dx, dy);
		gfx.EndFrame();
	}
	return 0;
}