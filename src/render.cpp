#define D3D11CreateDeviceAndSwapChain xxx
#define D3DCompile yyy

#include "render.h"

#ifdef _DEBUG
#include <stdio.h>
#endif

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

//extern "C" int _fltused = 0;

ID3D11Device* dvc = nullptr;
ID3D11DeviceContext* ctx = nullptr;
IDXGISwapChain* sc = nullptr;
ID3D11Buffer* pc = nullptr;
ID3D11ComputeShader* wcs = nullptr;
ID3D11ComputeShader* wth = nullptr;
ID3D11ComputeShader* ers = nullptr;
ID3D11ComputeShader* pcs = nullptr;

// a fullscreen window with stretched content is more compatible than oldskool fullscreen mode these days
static int SwapChainDesc[] = {
	XRES, YRES, 0, 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0,
	1, 0,
	DXGI_USAGE_UNORDERED_ACCESS,
	1,
	0,
	1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0  // [15]-[22]
};

//UINT Width;
//UINT Height;
//UINT Depth;
//UINT MipLevels;
//DXGI_FORMAT Format;
//D3D11_USAGE Usage;
//UINT BindFlags;
//UINT CPUAccessFlags;
//UINT MiscFlags;

static D3D11_TEXTURE2D_DESC _td = {
    512u, 512u,  // Width, Height
    1u,          // MipLevels
    1u,          // ArraySize  <-- this, always 1 for a single texture
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    {1, 0},      // SampleDesc (Count, Quality) -- required for Texture2D, not in Texture3D
    D3D11_USAGE_DEFAULT,
    D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
    0u, 0u       // CPUAccessFlags, MiscFlags
};

static D3D11_TEXTURE3D_DESC td = {
	256u,256u,256u,1u,
	DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_USAGE_DEFAULT,D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE
};

static D3D11_TEXTURE3D_DESC ed = {
    32u,32u,32u,1u,
    DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_USAGE_DEFAULT,D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE
};


static char wrl[] = "RWTexture3D<float4> f;float3 p(float l){return float3(l,l,l);}float l(float f){return frac(sin(f+1.951)*43758.5453);}float l(float3 f){float3 m=floor(f);f=frac(f);f=f*f*(p(3.)-p(2.)*f);float x=m.x+m.y*57.+113.*m.z;return lerp(lerp(lerp(l(x),l(x+1.),f.x),lerp(l(x+57.),l(x+58.),f.x),f.y),lerp(lerp(l(x+113.),l(x+114.),f.x),lerp(l(x+170.),l(x+171.),f.x),f.y),f.z);}float l(float3 f,float x){const float3 m=f*x;float i=1e10;for(int f=-1;f++<=1;)for(int s=-1;s++<=1;)for(int y=-1;y++<=1;){float3 p=floor(m)+float3(f,s,y);p=m-p-l(fmod(p,x));i=min(i,dot(p,p));}return max(min(i,1.),0.);}float p(float3 f,float x){float3 i=fmod(floor(f),x);f=frac(f);f=f*f*f*(f*(f*6.-15.)+10.);float p=l(i+float3(0,0,0)),s=l(fmod(i+float3(1,0,0),x)),m=l(fmod(i+float3(0,1,0),x)),y=l(fmod(i+float3(1,1,0),x)),d=l(fmod(i+float3(0,0,1),x)),D=l(fmod(i+float3(1,0,1),x)),u=l(fmod(i+float3(0,1,1),x));x=l(fmod(i+float3(1,1,1),x));return lerp(lerp(lerp(p,s,f.x),lerp(m,y,f.x),f.y),lerp(lerp(d,D,f.x),lerp(u,x,f.x),f.y),f.z);}float p(float3 f){float x=8.,i=0.,l=0.,s=.5;for(int m=0;m++<4.;){float y=p(f*x,x);i+=y*s;l+=s;s*=s;x*=2.;}return saturate(i/l);}[numthreads(8,8,8)]void cs_5_0(uint3 x:SV_DispatchThreadID){float3 i=float3(x)/256.;float m=p(i),s=1.-l(i,8.),y=1.-l(i,32.),D=1.-l(i,56.),d=1.-l(i,4.),u=1.-l(i,8.),I=1.-l(i,16.),L=1.-l(i,32.);d=1.-l(i,64.);f[x]=float4(saturate(s*.625+y*.25+D*.125+m*(1.-s*.625-y*.25-D*.125)),saturate(u*.625+I*.25+L*.125),saturate(I*.625+L*.25+d*.125),saturate(L*.75+d*.25));}";

static char wsh[] = "RWTexture2D<float4> f;float3 p(float f){return float3(f,f,f);}float l(float f){return frac(sin(f+1.951)*43758.5453);}float l(float3 f){float3 s=floor(f);f=frac(f);f=f*f*(p(3.)-p(2.)*f);float x=s.x+s.y*57.+113.*s.z;return lerp(lerp(lerp(l(x),l(x+1.),f.x),lerp(l(x+57.),l(x+58.),f.x),f.y),lerp(lerp(l(x+113.),l(x+114.),f.x),lerp(l(x+170.),l(x+171.),f.x),f.y),f.z);}float l(float3 f,float x){const float3 s=f*x;float i=1e10;for(int f=-1;f++<=1;)for(int r=-1;r++<=1;)for(int y=-1;y++<=1;){float3 p=floor(s)+float3(f,r,y);p=s-p-l(fmod(p,x));i=min(i,dot(p,p));}return max(min(i,1.),0.);}float p(float3 f,float x){float3 i=fmod(floor(f),x);f=frac(f);f=f*f*f*(f*(f*6.-15.)+10.);float p=l(i+float3(0,0,0)),r=l(fmod(i+float3(1,0,0),x)),s=l(fmod(i+float3(0,1,0),x)),y=l(fmod(i+float3(1,1,0),x)),d=l(fmod(i+float3(0,0,1),x)),D=l(fmod(i+float3(1,0,1),x)),m=l(fmod(i+float3(0,1,1),x));x=l(fmod(i+float3(1,1,1),x));return lerp(lerp(lerp(p,r,f.x),lerp(s,y,f.x),f.y),lerp(lerp(d,D,f.x),lerp(m,x,f.x),f.y),f.z);}float l(float2 f,float x,int y){float i=0.,l=0.,s=.5;for(int r=0;r++<y;){float m=p(float3(f*x,0.),x);i+=m*s;l+=s;s*=s;x*=2.;}return saturate(i/l);}float l(float f,float x,float i,float y,float l){return y+(f-x)/(i-x)*(l-y);}[numthreads(8,8,1)]void cs_5_0(uint3 i:SV_DispatchThreadID){float2 x=float2(i.xy)/512.;float p=l(x,4.,6);p=saturate(p*2.-.4);float y=saturate(l(x+float2(.3,.7),2.,4)*2.-.3);f[i.xy]=float4(p,y,0.,1.);}";

static char esh[] = "RWTexture3D<float4> f;float3 p(float l){return float3(l,l,l);}float l(float l){return frac(sin(l+1.951)*43758.5453);}float l(float3 f){float3 m=floor(f);f=frac(f);f=f*f*(p(3.)-p(2.)*f);float x=m.x+m.y*57.+113.*m.z;return lerp(lerp(lerp(l(x),l(x+1.),f.x),lerp(l(x+57.),l(x+58.),f.x),f.y),lerp(lerp(l(x+113.),l(x+114.),f.x),lerp(l(x+170.),l(x+171.),f.x),f.y),f.z);}float l(float3 f,float x){const float3 m=f*x;float i=1e10;for(int f=-1;f++<=1;)for(int s=-1;s++<=1;)for(int y=-1;y++<=1;){float3 p=floor(m)+float3(f,s,y);p=m-p-l(fmod(p,x));i=min(i,dot(p,p));}return max(min(i,1.),0.);}[numthreads(8,8,8)]void cs_5_0(uint3 x:SV_DispatchThreadID){float3 i=float3(x)/32.;float m=1.-l(i,2.),p=1.-l(i,4.),s=1.-l(i,8.),y=1.-l(i,16.);f[x]=float4(saturate(m*.625+p*.25+s*.125),saturate(p*.625+s*.25+y*.125),saturate(s*.75+y*.25),1.);}";

static char shader[] = "Texture3D<float4> f,s;Texture2D<float4> l;SamplerState y;RWTexture2D<float3> r;float3 e(float f){return float3(f,f,f);}static const float3 d=float3(-2e3,-10.,-2e3),c=float3(2e3,10.,2e3),m=e(.5),x=e(.05),n=max(m+x,e(1e-6)),i=.7*float3(.09,.33,.81),z=float3(90.,150.,240.);float2 e(float2 f){f=float2(dot(f,float2(2127.1,81.17)),dot(f,float2(1269.5,283.37)));return frac(sin(f)*43758.5453);}float p(float2 f){return length(e(f));}float e(float3 f,float3 y,float3 c,float3 z){return dot(c,y)==0.?0.:(dot(c,z)-dot(c,f))/dot(c,y);}float2 e(float3 f,float3 y){y.y+=(1.-(y.y+1.)*.5)*.15;y=normalize(y);float i=e(f,y,float3(0,1,0),float3(0,d.y,0)),z=e(f,y,float3(0,1,0),float3(0,c.y,0));if(z>0.)z*=y.y;return float2(i,z);}float e(float f,float y,float c,float z,float e){return z+(f-y)/(c-y)*(e-z);}float e(float3 i){float4 n=l.SampleLevel(y,i.xz/6e2,0);float r=n.x;if(r<.15)return 0.;float3 m=fmod(i,z);m+=z*step(m,e(-1e-6));float3 x=smoothstep(0.,.5,m/z)*smoothstep(1.,.5,m/z),R=fmod(i+z*.5,z);R+=z*step(R,e(-1e-6));float4 p=f.SampleLevel(y,m/z*.3+.5,0),u=f.SampleLevel(y,R/z*.3+.5,0);float D=lerp(.65,-.1,r),w=saturate(e(p.x,max(-1.+p.y*.625+p.z*.25+p.w*.125,D),1.,0.,1.)),S=saturate(e(u.x,max(-1.+u.y*.625+u.z*.25+u.w*.125,D),1.,0.,1.));D=lerp(S,w,x.x*x.z);D=saturate(e(D,1.-r,1.,0.,1.));w=saturate((i.y-d.y)/(c.y-d.y));S=saturate(e(w,0.,.1,0.,1.))*saturate(e(w,.2,.3,1.,0.));r=saturate(e(w,0.,.15,0.,1.))*saturate(e(w,.7,1.,1.,0.));D*=lerp(S,r,n.y);if(D<=0.)return 0.;x=m/z*3.+float3(.5,.5,.5);p=s.SampleLevel(y,x,0);S=p.x*.625+p.y*.25+p.z*.125;S=lerp(S,1.-S,saturate(w*5.));D=saturate(e(D,S*.2,1.,0.,1.));return.99*D;}float p(float f){f=max(f,1e-6);return pow(3e-5/f,.9);}float e(float f,float y){float z=f*f;return 1./12.566*((1.-z)/pow(1.+z-2.*f*y,1.5));}float3 e(float f,float y,float z){float3 m=e(0);float D=1.,s=1.,x=1.,S;for(float r=0.;r<4.;r++)S=lerp(e(-.1*x,y),e(.3*x,y),.7),m+=s*S*exp(-z*f*n*D),D*=.2,s*=.5,x*=.5;return m;}float e(float3 f,float3 y,float z){float2 s=e(f,y);float x=s.y/float(8),S=0.;for(int r=0;r<8;r++)f+=y*x,S+=max(0.,e(f)*x);f=e(S,z,x);y=2.*(1.-exp(-S*2.*n));return lerp(f*y,f,.5+.5*z);}[numthreads(8,8,1)]void cs_5_0(uint3 f:SV_DispatchThreadID){float2 s=float2(f.xy)/float2(1920.,1080.)-.5;s/=float2(.5625,1.);s.y=-s.y;float3 y=float3(0,-80,-10),z=normalize(float3(.1,.6,1.)),x=normalize(cross(z,float3(0,1,0)));z=normalize(z+x*s.x*.7+cross(x,z)*s.y*.7);float D=0.;x=e(1.);s=e(y,z);float S=s.y,l=S/64.,w=frac(52.9829189*frac(.06711056*float(f.x)+.00583715*float(f.y)));float3 u=l*z,R=y+z*s.x+u*w,H=normalize(float3(cos(1.),.6,sin(1.)));w=.5+.5003*dot(z,H);float3 G=float3(1.,.95,.85),T=lerp(i,.5*i,.5+.5*z.y);T+=G*p(1.-w);w=lerp(e(-.3,w),e(.3,w),.7);float3 b=G*64.,C=e(0.);for(;D<S;){R+=u;D+=l;float3 f=R-y;float r=1.-saturate((length(f)-4e2)/3e2),s=e(R)*l*r;r=saturate((R.y-d.y)/(c.y-d.y));f=n*s;if(s>0.){float3 y=G*lerp(.02,.6,r);y=.1*y+b*w*e(R,H,dot(z,H));y*=m*s;float3 l=exp(-f);C+=x*(y-y*l)/f;x*=l;if(length(x)<=.001){x=e(0.);break;}}}b=C+T*x;S=saturate(1.-z.y*4.);b=lerp(b,T*1.2,S*.4);b=clamp(b*(2.51*b+.03)/(b*(2.43*b+.59)+.14),0.,1.);S=p(float2(f.xy)/float2(1920.,1080.));b-=S*.08;b=saturate(b);r[f.xy]=b;}";

void r_init(HWND hwnd)
{
	SwapChainDesc[11] = (int)hwnd;
	D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
		0, 0, D3D11_SDK_VERSION, (DXGI_SWAP_CHAIN_DESC*)&SwapChainDesc[0], &sc, &dvc, nullptr, &ctx
	);

	// get swapchain backbuffer as Texture2D and create UAV for fullscreen shader output
	sc->GetBuffer(0u, __uuidof(ID3D11Texture2D), (LPVOID*)&SwapChainDesc[15]);
	dvc->CreateUnorderedAccessView((ID3D11Texture2D*)SwapChainDesc[15], nullptr, (ID3D11UnorderedAccessView**)&SwapChainDesc[15]);

	// create 3D texture for worley noise
	ID3D11Texture3D* wt = nullptr;
	dvc->CreateTexture3D(&td, nullptr, &wt);
	dvc->CreateUnorderedAccessView(wt, nullptr, (ID3D11UnorderedAccessView**)&SwapChainDesc[16]);
	dvc->CreateShaderResourceView(wt, nullptr, (ID3D11ShaderResourceView**)&SwapChainDesc[17]);

    // 2D weather texture
    ID3D11Texture2D* _wt = nullptr;
    dvc->CreateTexture2D(&_td, nullptr, &_wt);
    dvc->CreateUnorderedAccessView(_wt, nullptr, (ID3D11UnorderedAccessView**)&SwapChainDesc[19]);
    dvc->CreateShaderResourceView(_wt, nullptr, (ID3D11ShaderResourceView**)&SwapChainDesc[20]);

    ID3D11Texture3D* et = nullptr;
    dvc->CreateTexture3D(&ed, nullptr, &et);
    dvc->CreateUnorderedAccessView(et, nullptr, (ID3D11UnorderedAccessView**)&SwapChainDesc[21]);
    dvc->CreateShaderResourceView(et, nullptr, (ID3D11ShaderResourceView**)&SwapChainDesc[22]);

	// linear wrap sampler for 3D texture
	D3D11_SAMPLER_DESC sd = { D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP };
	dvc->CreateSamplerState(&sd, (ID3D11SamplerState**)&SwapChainDesc[18]);

	ID3DBlob* b = nullptr;

	// compile worley shader, dispatch once to populate 3D texture
#ifdef _DEBUG
	ID3DBlob* e = nullptr;
	HRESULT hr = D3DCompile(wrl, sizeof(wrl), nullptr, 0, 0, "cs_5_0", "cs_5_0", D3D10_SHADER_DEBUG, 0u, &b, &e);
	if (FAILED(hr)) {
		char message[4096];
		sprintf(message, "unknown compiler error");
		if (e != NULL) sprintf(message, "%s", (char*)e->GetBufferPointer());
		MessageBoxA(hwnd, message, "Worley Error", MB_OK);
	}
	else {
		dvc->CreateComputeShader((DWORD*)b->GetBufferPointer(), b->GetBufferSize(), nullptr, &wcs);
	}
	if (e) e->Release();
	if (b) b->Release();
	b = nullptr;
#else
	D3DCompile(wrl, sizeof(wrl), nullptr, 0, 0, "cs_5_0", "cs_5_0", 0u, 0u, &b, 0);
	dvc->CreateComputeShader((void*)(((int*)b)[3]), ((int*)b)[2], nullptr, &wcs);
	b = nullptr;
#endif

	ctx->CSSetShader(wcs, nullptr, 0u);
	ctx->CSSetUnorderedAccessViews(0u, 1u, (ID3D11UnorderedAccessView**)&SwapChainDesc[16], nullptr);
	ctx->Dispatch(32u, 32u, 32u);

	// unbind worley UAV before binding as SRV
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	ctx->CSSetUnorderedAccessViews(0u, 1u, &nullUAV, nullptr);

    // weather texture compute shader
#ifdef _DEBUG
    e = nullptr;
    hr = D3DCompile(wsh, sizeof(wsh), nullptr, 0, 0, "cs_5_0", "cs_5_0", D3D10_SHADER_DEBUG, 0u, &b, &e);
    if (FAILED(hr)) {
        char message[4096];
        sprintf(message, "unknown compiler error");
        if (e != NULL) sprintf(message, "%s", (char*)e->GetBufferPointer());
        MessageBoxA(hwnd, message, "Shader Error", MB_OK);
    }
    else {
        dvc->CreateComputeShader((DWORD*)b->GetBufferPointer(), b->GetBufferSize(), nullptr, &wth);
    }
    if (e) e->Release();
    if (b) b->Release();
#else
    D3DCompile(wsh, sizeof(wsh), nullptr, 0, 0, "cs_5_0", "cs_5_0", 0u, 0u, &b, 0);
    dvc->CreateComputeShader((void*)(((int*)b)[3]), ((int*)b)[2], nullptr, &wth);
#endif

    ctx->CSSetShader(wth, nullptr, 0u);
    ctx->CSSetUnorderedAccessViews(0u, 1u, (ID3D11UnorderedAccessView**)&SwapChainDesc[19], nullptr);
    ctx->Dispatch(64u,64u,1u);

    ctx->CSSetUnorderedAccessViews(0u, 1u, &nullUAV, nullptr);

    // compile erosion texture shader
#ifdef _DEBUG
    e = nullptr;
    hr = D3DCompile(esh, sizeof(esh), nullptr, 0, 0, "cs_5_0", "cs_5_0", D3D10_SHADER_DEBUG, 0u, &b, &e);
    if (FAILED(hr)) {
        char message[4096];
        sprintf(message, "unknown compiler error");
        if (e != NULL) sprintf(message, "%s", (char*)e->GetBufferPointer());
        MessageBoxA(hwnd, message, "Shader Error", MB_OK);
    }
    else {
        dvc->CreateComputeShader((DWORD*)b->GetBufferPointer(), b->GetBufferSize(), nullptr, &ers);
    }
    if (e) e->Release();
    if (b) b->Release();
#else
    D3DCompile(esh, sizeof(esh), nullptr, 0, 0, "cs_5_0", "cs_5_0", 0u, 0u, &b, 0);
    dvc->CreateComputeShader((void*)(((int*)b)[3]), ((int*)b)[2], nullptr, &ers);
#endif

    ctx->CSSetShader(ers, nullptr, 0u);
    ctx->CSSetUnorderedAccessViews(0u, 1u, (ID3D11UnorderedAccessView**)&SwapChainDesc[21], nullptr);
    ctx->Dispatch(4u,4u,4u);

    ctx->CSSetUnorderedAccessViews(0u, 1u, &nullUAV, nullptr);

	// compile fullscreen shader
#ifdef _DEBUG
	e = nullptr;
	hr = D3DCompile(shader, sizeof(shader), nullptr, 0, 0, "cs_5_0", "cs_5_0", D3D10_SHADER_DEBUG, 0u, &b, &e);
	if (FAILED(hr)) {
		char message[4096];
		sprintf(message, "unknown compiler error");
		if (e != NULL) sprintf(message, "%s", (char*)e->GetBufferPointer());
		MessageBoxA(hwnd, message, "Shader Error", MB_OK);
	}
	else {
		dvc->CreateComputeShader((DWORD*)b->GetBufferPointer(), b->GetBufferSize(), nullptr, &pcs);
	}
	if (e) e->Release();
	if (b) b->Release();
#else
	D3DCompile(shader, sizeof(shader), nullptr, 0, 0, "cs_5_0", "cs_5_0", 0u, 0u, &b, 0);
	dvc->CreateComputeShader((void*)(((int*)b)[3]), ((int*)b)[2], nullptr, &pcs);
#endif

	ctx->CSSetShader(pcs, nullptr, 0u);
	// bind backbuffer UAV to u0, worley SRV to t0, sampler to s0
	ctx->CSSetUnorderedAccessViews(0u, 1u, (ID3D11UnorderedAccessView**)&SwapChainDesc[15], nullptr);
	ctx->CSSetShaderResources(0u, 1u, (ID3D11ShaderResourceView**)&SwapChainDesc[17]);
    ctx->CSSetShaderResources(1u, 1u, (ID3D11ShaderResourceView**)&SwapChainDesc[22]);
    ctx->CSSetShaderResources(2u, 1u, (ID3D11ShaderResourceView**)&SwapChainDesc[20]);
	ctx->CSSetSamplers(0u, 1u, (ID3D11SamplerState**)&SwapChainDesc[18]);
}

void r_loop()
{
	ctx->Dispatch(XRES >> 3, YRES >> 3, 1u);
	sc->Present(0u, 0u);
}

void r_exit()
{
	pcs->Release();
	sc->Release();
	ctx->Release();
	dvc->Release();
}
