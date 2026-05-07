#ifndef _RENDER_H_
#define _RENDER_H_

#include <d3d11.h>
#include <d3dcompiler.h>

extern ID3D11Device           *dvc;
extern ID3D11DeviceContext	  *ctx;
extern IDXGISwapChain		  *sc;
extern ID3D11Texture2D		  *fb;
extern ID3D11RenderTargetView *rtv;
extern ID3D11RasterizerState  *rs;
extern ID3D11VertexShader     *vs;
extern ID3D11PixelShader      *ps;

void r_init(HWND hwnd);
void r_loop();
void r_exit();

#define XRES        1920
#define YRES        1080

static char shader[] = "struct vs_out{float4 pos:SV_POSITION;float4 col:COL;};vs_out VsMain(uint vertexid:SV_VERTEXID){vs_out output;output.pos=float4(vertexid>>1,vertexid&1,0,.5)*4-1;output.col=float4(vertexid==0,(vertexid==1)*2,(vertexid==2)*2,1);return output;}float4 PsMain(vs_out input):SV_TARGET{return input.col;}";

#endif