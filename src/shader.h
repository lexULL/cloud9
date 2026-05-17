#pragma once

static char wrl[] =
R"(
RWTexture3D<float4> wrl;

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

float perlin(float3 p, float freq, int octaveCount) {
    const float octaveFrequencyFactor = 2.;
    float sum = 0., weightSum=0., weight=.5;
    for(int oct = 0; oct++ < octaveCount;) {
        float val = perlinTile(p * freq, freq);
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

[numthreads(8,8,8)]
void cs_5_0(uint3 i : SV_DispatchThreadID) {
    float3 uvw = float3(i) / 256.;
    float p = perlin(uvw, 8., 4.);
    // high-freq worley FBM for blending with perlin
    float pw0 = 1. - worley(uvw, 4. * 2.);
    float pw1 = 1. - worley(uvw, 4. * 8.);
    float pw2 = 1. - worley(uvw, 4. * 14.);
    float pwFBM = pw0*.625 + pw1*.25 + pw2*.125;

    float perlinWorley = remap(p, 0., 1., pwFBM, 1.);

    // low-freq worley FBMs (GBA channels)
    float w0 = 1. - worley(uvw, 4.);
    float w1 = 1. - worley(uvw, 8.);
    float w2 = 1. - worley(uvw, 16.);
    float w3 = 1. - worley(uvw, 32.);
    float w4 = 1. - worley(uvw, 64.);

    float wFBM0 = w1*.625 + w2*.25 + w3*.125;
    float wFBM1 = w2*.625 + w3*.25 + w4*.125;
    float wFBM2 = w3*.75  + w4*.25;  // 2 octaves only

    wrl[i] = float4(
        saturate(perlinWorley),
        saturate(wFBM0),
        saturate(wFBM1),
        saturate(wFBM2)
    );
}
)";

static char wsh[] =
R"(
RWTexture2D<float4> weather;

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
   coverage = saturate(coverage * 2. - .4);  // was * 3. - .1, much more aggressive cutoff  // was * 2. - .3  // controls how much sky is cloudy
    
    float cloudType = saturate(perlin(uvw + float2(.3, .7), 2., 4.) * 2. - .3);
    
    weather[i.xy] = float4(coverage, cloudType, 0., 1.);
}
)";

static char esh[] =
R"(
RWTexture3D<float4> ers;

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
)";

static char shader[] =
R"(
Texture3D<float4> wrl;
Texture3D<float4> ers;
Texture2D<float4> wth;
SamplerState Smp;
RWTexture2D<float3> y;

float3 f3(float x) { return float3(x,x,x); }

static const float3 cloudsBoundsMin = float3(-2e3, -10., -2e3);
static const float3 cloudsBoundsMax = float3(2e3, 10., 2e3);
static const float3 sigmaS = f3(.5);
static const float3 sigmaA = f3(.05);
static const float3 sigmaE = max(sigmaS + sigmaA, f3(1e-6));
static const float3 skyColour = .7 * float3(.09, .33, .81);
static const float3 tileSize = float3(90., 150., 240.);  // bigger = fewer, larger clouds

float2 hash2(float2 p) {
    p = float2(dot(p, float2(2127.1, 81.17)), dot(p, float2(1269.5, 283.37)));
    return frac(sin(p) * 43758.5453);
}

float filmGrain(float2 uv) {
    return length(hash2(uv));
}

float linePlaneIntersection(float3 rayOrigin, float3 rayDirection, 
                           float3 normal, float3 coord) {
    // get d value
    float d = dot(normal, coord);
    if (dot(normal, rayDirection) == 0.) {
        return 0.; // No intersection, the line is parallel to the plane
    }
    // Compute the X value for the directed line ray intersecting the plane
    float x = (d - dot(normal, rayOrigin)) / dot(normal, rayDirection);
    // output contact point
    return x; //Make sure your ray vector is normalized
}


float2 rayCloudDist(float3 rayOrigin, float3 rayDirection) {
    // bias controls how much the horizon extends
    // 0 = flat planes, higher = more horizon wrap
    rayDirection.y += (1.-((rayDirection.y + 1.) * .5)) * .15;  // was .3, halved
    rayDirection = normalize(rayDirection);
    
    float toCloud = linePlaneIntersection(rayOrigin, rayDirection, float3(0,1,0), float3(0, cloudsBoundsMin.y, 0));
    float inCloud = linePlaneIntersection(rayOrigin, rayDirection, float3(0,1,0), float3(0, cloudsBoundsMax.y, 0));
    if(inCloud > 0.) inCloud = inCloud * rayDirection.y;
    return float2(toCloud, inCloud);
}

float remap(float v, float lo, float hi, float newLo, float newHi) {
    return newLo + (v - lo) / (hi - lo) * (newHi - newLo);
}

float sampleDensity(float3 p) {
    // sample weather texture - .r = coverage, .g = cloud type (0=stratus, 1=cumulus)
    float2 weatherUV = p.xz / 600.;
    float4 weatherSample = wth.SampleLevel(Smp, weatherUV, 0);
    float coverage = weatherSample.r;
    float cloudType = weatherSample.g;
    if(coverage < .15) return 0.;

    // tile the 3D texture with cross-fade blending to hide seams
    float3 pLocal = fmod(p, tileSize);
    pLocal += tileSize * step(pLocal, f3(-1e-6));
    float3 tileBlend = smoothstep(0., .5, pLocal/tileSize) * smoothstep(1., .5, pLocal/tileSize);
    float3 pLocal2 = fmod(p + tileSize * .5, tileSize);
    pLocal2 += tileSize * step(pLocal2, f3(-1e-6));

    // sample both tile positions with all 4 channels
    float4 wrlSample1 = wrl.SampleLevel(Smp, pLocal/tileSize*.3 + .5, 0);
    float4 wrlSample2 = wrl.SampleLevel(Smp, pLocal2/tileSize*.3 + .5, 0);

    // Seb's packed formula: remap perlin-worley (.x) using low-freq worley FBM (GBA) as minimum
    // this is the core of the GPU Pro 7 base cloud shape
    float lowFreqFBM1 = wrlSample1.y*.625 + wrlSample1.z*.25 + wrlSample1.w*.125;
    float lowFreqFBM2 = wrlSample2.y*.625 + wrlSample2.z*.25 + wrlSample2.w*.125;
    float minVal = lerp(.65, -.1, coverage);  // high coverage = low minimum = more cloud passes through
    float cloudMap1 = saturate(remap(wrlSample1.x, max(-(1.-lowFreqFBM1), minVal), 1., 0., 1.));
    float cloudMap2 = saturate(remap(wrlSample2.x, max(-(1.-lowFreqFBM2), minVal), 1., 0., 1.));

    // cross-fade between two offset samples to hide tile seams
    float cloudMap = lerp(cloudMap2, cloudMap1, tileBlend.x * tileBlend.z);

    // coverage remap: low coverage = only densest parts survive
    // coverage*coverage makes falloff nonlinear (sharper edges at low coverage)
    cloudMap = saturate(remap(cloudMap, 1.-coverage, 1., 0., 1.));

    // vertical height gradient blended by cloud type
    float cloudMap2h = saturate((p.y - cloudsBoundsMin.y) / (cloudsBoundsMax.y - cloudsBoundsMin.y));

    // stratus: thin flat layer in lower 20-30% of the layer
    float stratusGradient = saturate(remap(cloudMap2h, 0., .1, 0., 1.))
                          * saturate(remap(cloudMap2h, .2, .3, 1., 0.));

    // cumulus: tall puffy clouds, density peaks in middle, tapers at top
    float cumulusGradient = saturate(remap(cloudMap2h, 0., .15, 0., 1.))
                          * saturate(remap(cloudMap2h, .7, 1., 1., 0.));

    
    cloudMap *= lerp(stratusGradient, cumulusGradient, cloudType);
    if(cloudMap <= 0.) return 0.;

    // erosion texture (Seb section 4.3): separate 32^3 high-freq Worley
    // sampled at 3x base frequency to add wispy fine detail
    float3 erosionUV = pLocal / tileSize * 3. + float3(.5, .5, .5);
    float4 erosionNoise = ers.SampleLevel(Smp, erosionUV, 0);
    float erosionFBM = erosionNoise.r*.625 + erosionNoise.g*.25 + erosionNoise.b*.125;

    // flip erosion at cloud base to create wispy undersides (Frostbite technique)
    // bottom: inverted = stringy wisps, top: normal = rounded tops
    erosionFBM = lerp(erosionFBM, 1.-erosionFBM, saturate(cloudMap2h * 5.));

    // apply erosion: erodes cloud edges, .2 controls strength
    float cloudMapOut = saturate(remap(cloudMap, erosionFBM * .2, 1., 0., 1.));

    return .99 * cloudMapOut;
}

float getGlow(float dist) {
    dist = max(dist, 1e-6);
    return pow(3e-5/dist, .9);
}

// henyey greenstein phase function for light scattering angular distribution
float hg(float g, float mu) {
    float gg = g * g;
    return 1./12.566 * ((1.-gg) / pow(1.+gg-2.*g*mu, 1.5));
}

float3 multipleOctaves(float extinction, float mu, float stepL) {
    float3 luminance = f3(0);
    float a = 1., b = 1., c = 1., phase;
    for(float i = 0.; i < 4.; i++) {
        phase = lerp(hg(-.1*c, mu), hg(.3*c, mu), .7);
        luminance += b * phase * exp(-stepL * extinction * sigmaE * a);
        a *= .2;
        b *= .5;
        c *= .5;
    }
    return luminance;
}

float lightmarch(float3 p, float3 sunDirection, float mu) {
    float2 lightDist = rayCloudDist(p, sunDirection);
    float stepSize = lightDist.y / float(8);
    float totalDensity = 0.;
    for(int i = 0; i < 8; i++) {
        p += sunDirection * stepSize;
        totalDensity += max(0., sampleDensity(p) * stepSize);
    }
    float3 beersLaw = multipleOctaves(totalDensity, mu, stepSize);
    float3 powder = 2. * (1. - exp(-totalDensity * 2. * sigmaE));
    return lerp(beersLaw * powder, beersLaw, .5+.5*mu);
}

[numthreads(8,8,1)]
void cs_5_0(uint3 i : SV_DispatchThreadID) {
    float2 uv = float2(i.xy) / float2(1920., 1080.) - .5;
    uv /= float2(.5625, 1.);
    uv.y = -uv.y;

    float3 rayOrigin = float3(0, -80, -10);
    float3 forward = normalize(float3(.1, .6, 1.));
    float3 right = normalize(cross(forward, float3(0,1,0)));
    forward = normalize(forward + right*uv.x*.7 + cross(right, forward)*uv.y*.7);  // was .95

    float distTravelled = 0.;
    float3 transmittance = f3(1.);

    float2 uv2 = rayCloudDist(rayOrigin, forward);
    float distInsideClouds = uv2.y;
    float step_size = distInsideClouds / 64.;
    float ign = frac(52.9829189 * frac(.06711056*float(i.x) + .00583715*float(i.y)));
    float3 step_vec = step_size * forward;
    float3 ray_pos = rayOrigin + forward * uv2.x + step_vec * ign;

    float3 sunDirection = normalize(float3(cos(1.), .6, sin(1.)));
    float mu = .5 + .5003 * dot(forward, sunDirection);

    float3 lightColor = float3(1., .95, .85);
    float3 backgroundColor = lerp(skyColour, .5*skyColour, .5+.5*forward.y);
    backgroundColor += lightColor * getGlow(1.-mu);

    float phaseFunction = lerp(hg(-.3, mu), hg(.3, mu), .7);
    float3 sunLight = lightColor * 64.;
    float3 cl = f3(0.);

    while(distTravelled < distInsideClouds) {
        ray_pos += step_vec;
        distTravelled += step_size;
        float3 toSample = ray_pos - rayOrigin;
        float distFade = 1. - saturate((length(toSample) - 400.) / 300.);
        float density = sampleDensity(ray_pos) * step_size * distFade;
        float cloudHeight = saturate((ray_pos.y - cloudsBoundsMin.y) / (cloudsBoundsMax.y - cloudsBoundsMin.y));
        float3 sampleSigmaE = sigmaE * density;

        if(density > 0.) {
            float3 ambient = lightColor * lerp(.02, .6, cloudHeight);
            float3 luminance = .1*ambient + sunLight*phaseFunction*lightmarch(ray_pos, sunDirection, dot(forward, sunDirection));
            luminance *= sigmaS * density;
            float3 sampleTransmittance = exp(-sampleSigmaE);
            cl += transmittance * (luminance - luminance*sampleTransmittance) / sampleSigmaE;
            transmittance *= sampleTransmittance;
            if(length(transmittance) <= .001) { transmittance = f3(0.); break; }
        }
    }

    float3 color = cl + backgroundColor * transmittance;
    float horizonFade = saturate(1. - forward.y * 4.);
    color = lerp(color, backgroundColor * 1.2, horizonFade * .4);
    color = clamp(color*(2.51*color+.03)/(color*(2.43*color+.59)+.14), 0., 1.);
    float grain = filmGrain(float2(i.xy) / float2(1920.,1080.));
    color -= grain *.08;
    color = saturate(color);
    y[i.xy] = color;
}
)";