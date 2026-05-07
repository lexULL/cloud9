#define D3D11CreateDeviceAndSwapChain xxx
#define D3DCompile yyy

#include "render.h"

#undef D3D11CreateDeviceAndSwapChain
#undef D3DCompile
#undef WINAPI
#define WINAPI __declspec(dllimport) __stdcall

extern "C"
{
	HRESULT WINAPI D3D11CreateDeviceAndSwapChain(
		__in_opt IDXGIAdapter* pAdapter,
		D3D_DRIVER_TYPE DriverType,
		HMODULE Software,
		UINT Flags,
		__in_ecount_opt(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
		UINT FeatureLevels,
		UINT SDKVersion,
		__in_opt CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
		__out_opt IDXGISwapChain** ppSwapChain,
		__out_opt ID3D11Device** ppDevice,
		__out_opt D3D_FEATURE_LEVEL* pFeatureLevel,
		__out_opt ID3D11DeviceContext** ppImmediateContext);

	HRESULT WINAPI
		D3DCompile(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData,
			_In_ SIZE_T SrcDataSize,
			_In_opt_ LPCSTR pSourceName,
			_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* pDefines,
			_In_opt_ ID3DInclude* pInclude,
			_In_opt_ LPCSTR pEntrypoint,
			_In_ LPCSTR pTarget,
			_In_ UINT Flags1,
			_In_ UINT Flags2,
			_Out_ ID3DBlob** ppCode,
			_Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorMsgs);
}

extern "C" int _fltused = 0;

#pragma function(memset)
extern "C" void* memset(void* dst, int c, size_t n) {
	unsigned char* p = (unsigned char*)dst;
	while (n--) *p++ = c;
	return dst;
}

ID3D11Device* dvc = nullptr;
ID3D11DeviceContext* ctx = nullptr;
IDXGISwapChain* sc = nullptr;
ID3D11Texture2D* fb = nullptr;
ID3D11RenderTargetView* rtv = nullptr;
ID3D11RasterizerState* rs = nullptr;
ID3D11VertexShader* vs = nullptr;
ID3D11PixelShader* ps = nullptr;

void r_init(HWND hwnd)
{
	D3D_FEATURE_LEVEL fls[]={D3D_FEATURE_LEVEL_11_0};
	DXGI_SWAP_CHAIN_DESC scd = {};
	scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	scd.SampleDesc.Count = 1u;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.BufferCount = 2u;
	scd.OutputWindow = hwnd;
	scd.Windowed = 1;
	scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, 
		fls, ARRAYSIZE(fls), D3D11_SDK_VERSION, &scd, &sc, &dvc, nullptr, &ctx
	);
	sc->GetDesc(&scd);
	sc->GetBuffer(0u, __uuidof(ID3D11Texture2D), (void**)&fb);
	dvc->CreateRenderTargetView(fb, nullptr, &rtv);

	// LPCVOID = const void*
	// LPCSTR = const char*

	ID3DBlob* vsb = nullptr;
	D3DCompile(shader, lstrlenA(shader), nullptr, 0, 0, "VsMain", "vs_5_0", 0u, 0u, &vsb, 0);

	ID3DBlob* psb = nullptr;
	D3DCompile(shader, lstrlenA(shader), nullptr, 0, 0, "PsMain", "ps_5_0", 0u, 0u, &psb, 0);

	dvc->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), 0, &vs);

	dvc->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), 0, &ps);

	D3D11_RASTERIZER_DESC rsd = { D3D11_FILL_SOLID, D3D11_CULL_NONE };
	dvc->CreateRasterizerState(&rsd, &rs);
}

void r_loop()
{
	D3D11_VIEWPORT vp = { 0,0, XRES, YRES, 0, 1 };
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ctx->VSSetShader(vs, nullptr, 0u);
	ctx->RSSetViewports(1, &vp);
	ctx->RSSetState(rs);
	ctx->PSSetShader(ps, nullptr, 0u);
	ctx->OMSetRenderTargets(1u, &rtv, nullptr);
	ctx->Draw(3u, 0u);
	sc->Present(1u, 0u);
}

void r_exit()
{
	rs->Release();
	rtv->Release();
	fb->Release();
	ctx->Release();
	dvc->Release();
}
