#include "render.h"

#ifndef _DEBUG
void WinMainCRTStartup()
{
	ShowCursor( 0 );
	HWND  hWnd = CreateWindowExA( 0, ( LPCSTR )0xC018, 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0 );
	if (!hWnd) ExitProcess(2u);
#else
#include <stdio.h>
int WinMain( HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nFunsterStil )
{
	HWND hWnd = CreateWindowA( "edit", 0, WS_POPUP | WS_VISIBLE, 0, 0, XRES, YRES, 0, 0, 0, 0 );
#endif

	r_init( hWnd );

	do
	{
		PeekMessageA( 0, 0, 0, 0, PM_REMOVE );

		r_loop();
	} 
	while ( !GetAsyncKeyState( VK_ESCAPE ) );

	// r_exit();

	ExitProcess( 0 );
}