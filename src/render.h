#ifndef _RENDER_H_
#define _RENDER_H_

#include <d3d11.h>
#include <d3dcompiler.h>

extern ID3D11Device           *dvc;
extern ID3D11DeviceContext	  *ctx;
extern IDXGISwapChain		  *sc;
extern ID3D11Buffer           *pc;
extern ID3D11ComputeShader    *wcs;
extern ID3D11ComputeShader    *pcs;

void r_init(HWND hwnd);
void r_loop();
void r_exit();

#define XRES        1920
#define YRES        1080

#endif