RWTexture2D<float4> weather;

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

float3 fade(float3 t) { return t*t*t*(t*(t*6.-15.)+10.);}
float perlinTile(float3 p, float freq) {
    float3 Pi = fmod(floor(p), freq);
    float3 Pf = frac(p);
    float3 f = fade(Pf);

    // tileability comes from mod(Pi, freq) so corners wrap
    float n000 = noise(Pi + float3(0,0,0));
    float n100 = noise(fmod(Pi + float3(1,0,0), freq));
    float n010 = noise(fmod(Pi + float3(0,1,0), freq));
    float n110 = noise(fmod(Pi + float3(1,1,0), freq));
    float n001 = noise(fmod(Pi + float3(0,0,1), freq));
    float n101 = noise(fmod(Pi + float3(1,0,1), freq));
    float n011 = noise(fmod(Pi + float3(0,1,1), freq));
    float n111 = noise(fmod(Pi + float3(1,1,1), freq));

    return lerp(
        lerp(lerp(n000,n100,f.x), lerp(n010,n110,f.x), f.y),
        lerp(lerp(n001,n101,f.x), lerp(n011,n111,f.x), f.y),
        f.z
    );
}

float perlin(float2 p, float freq, int octaveCount) {
    const float octaveFrequencyFactor = 2.;
    float sum = 0., weightSum=0., weight=.5;
    for(int oct = 0; oct++ < octaveCount;) {
        float val = perlinTile(float3(p * freq, 0.), freq);
        sum += val * weight;
        weightSum += weight;
        weight *= weight;
        freq *= octaveFrequencyFactor;
    }
    // float noise = (sum / weightSum) * .5 + .5;
    return saturate(sum / weightSum);
}

float remap(float v, float oMin, float oMax, float nMin, float nMax) {
    return nMin+((v-oMin)/(oMax-oMin)*(nMax-nMin));
}

[numthreads(8,8,1)]
void cs_5_0(uint3 i : SV_DispatchThreadID) {
    float2 uvw = float2(i.xy) / 512.;
    
    float coverage = perlin(uvw, 4., 6.);
   coverage = saturate(coverage * 2. - .4);  // controls how much sky is cloudy
    
    float cloudType = saturate(perlin(uvw + float2(.3, .7), 2., 4.) * 2. - .3);
    
    weather[i.xy] = float4(coverage, cloudType, 0., 1.);
}