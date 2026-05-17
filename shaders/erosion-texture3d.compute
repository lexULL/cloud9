RWTexture3D<float4> ers;

// HLSL compute adaptation of CPU-side GLM code found in: https://github.com/sebh/TileableVolumeNoise

float3 f3(float n) { return float3(n,n,n); }

float hash(float n) {
    uint u = asuint(n);
    u ^= u >> 16;
    u *= 0x45d9f3bu;
    u ^= u >> 16;
    return float(u) * (1./4294967296.);
}
// hash-based 3d value noise
float noise(float3 x) {
    float3 p = floor(x);
    float3 f = frac(x);
    f = f * f * (f3(3.)-f3(2.)*f);
    float n = p.x+p.y*57.+113.*p.z;
    return lerp(
        lerp(
            lerp(hash(n),hash(n+1.),f.x),
            lerp(hash(n+57.),hash(n+58.),f.x),
            f.y
        ),
        lerp(
            lerp(hash(n+113.),hash(n+114.),f.x),
            lerp(hash(n+170.),hash(n+171.),f.x),
            f.y
        ),
        f.z
    );
}

float worley(float3 p, float cellCount) {
    const float3 pCell = p*cellCount;
    float d = 1.0e10;
    for(int xo = -1; xo++ <= 1;) {
        for(int yo = -1; yo++<=1;) {
            for(int zo=-1; zo++<=1;) {
                float3 tp = floor(pCell)+float3(xo,yo,zo);
                tp = pCell - tp - noise(fmod(tp, cellCount));
                d = min(d, dot(tp,tp));
            }
        }
    }
    d = min(d, 1.);
    d = max(d, 0.);
    return d;
}

[numthreads(8,8,8)]
void cs_5_0(uint3 i : SV_DispatchThreadID) {
    float3 uvw = float3(i) / 32.;
    float w0 = 1. - worley(uvw, 2.);
    float w1 = 1. - worley(uvw, 4.);
    float w2 = 1. - worley(uvw, 8.);
    float w3 = 1. - worley(uvw, 16.);
    ers[i] = float4(
        saturate(w0*.625 + w1*.25 + w2*.125),
        saturate(w1*.625 + w2*.25 + w3*.125),
        saturate(w2*.75  + w3*.25),
        1.
    );
}