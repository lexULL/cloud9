Texture3D<float4> wrl;
Texture3D<float4> ers;
Texture2D<float4> wth;
SamplerState Smp;
RWTexture2D<float3> y;

// helper function since HLSL does not support single value construction of >=2 component vectors like GLSL
float3 f3(float x) { return float3(x,x,x); }

static const float3 cloudsBoundsMin = float3(-2e3, -10., -2e3);
static const float3 cloudsBoundsMax = float3(2e3, 10., 2e3);
static const float3 sigmaS = f3(.5);  // scattering coefficient
static const float3 sigmaA = f3(.05); // absorption coefficient
static const float3 sigmaE = max(sigmaS + sigmaA, f3(1e-6)); // extinction coefficient
static const float3 skyColour = .7 * float3(.09, .33, .81);
static const float3 tileSize = float3(90., 150., 240.);  // bigger = fewer, larger clouds

float2 hash2(float2 p) {
    p = float2(dot(p, float2(2127.1, 81.17)), dot(p, float2(1269.5, 283.37)));
    return frac(sin(p) * 43758.5453);
}

float filmGrain(float2 uv) {
    return length(hash2(uv));
}

// // Source - https://stackoverflow.com/a/35396994
// Posted by Jomi
// Retrieved 2026-05-16, License - CC BY-SA 3.0
float linePlaneIntersection(float3 rayOrigin, float3 rayDirection, 
                           float3 normal, float3 coord) {
    float d = dot(normal, coord);
    if (dot(normal, rayDirection) == 0.) {
        return 0.;
    }

    float x = (d - dot(normal, rayOrigin)) / dot(normal, rayDirection);
    return x;
}


float2 rayCloudDist(float3 rayOrigin, float3 rayDirection) {
    // bias controls how much the horizon extends
    // 0 = flat planes, higher = more horizon wrap
    static const float HORIZON_WRAP_STRENGTH = .15;
    rayDirection.y += (1.-((rayDirection.y + 1.) * .5)) * HORIZON_WRAP_STRENGTH; 
    rayDirection = normalize(rayDirection);
    
    float toCloud = linePlaneIntersection(rayOrigin, rayDirection, float3(0,1,0), float3(0, cloudsBoundsMin.y, 0));
    float inCloud = linePlaneIntersection(rayOrigin, rayDirection, float3(0,1,0), float3(0, cloudsBoundsMax.y, 0));
    if(inCloud > 0.) inCloud = inCloud * rayDirection.y; // projects distance to vertical thickness
    return float2(toCloud, inCloud);
}

float remap(float v, float lo, float hi, float newLo, float newHi) {
    return newLo + (v - lo) / (hi - lo) * (newHi - newLo);
}

// implements the Schneider-Vos Perlin-Worley remap technique from Horizon Zero Dawn's cloud system
// https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/s2016-pbs-frostbite-sky-clouds-new.pdf
// https://www.guerrilla-games.com/read/the-real-time-volumetric-cloudscapes-of-horizon-zero-dawn
float sampleDensity(float3 p) {
    // sample weather texture - .r = coverage, .g = cloud type (0=stratus, 1=cumulus)
    float2 weatherUV = p.xz / 600.; // 600. = hard-coded value that I played with for too long to try to get better looking results
    float4 weatherSample = wth.SampleLevel(Smp, weatherUV, 0);
    float coverage = weatherSample.r;
    float cloudType = weatherSample.g;
    // smaller clouds that end up looking as single random points on the screen get ignored
    static const float COVERAGE_BOUNDS = .15;
    if(coverage < COVERAGE_BOUNDS) return 0.; 

    // tile the 3D texture with cross-fade blending to hide seams
    float3 pLocal = fmod(p, tileSize);
    pLocal += tileSize * step(pLocal, f3(-1e-6));
    float3 tileBlend = smoothstep(0., .5, pLocal/tileSize) * smoothstep(1., .5, pLocal/tileSize);
    float3 pLocal2 = fmod(p + tileSize * .5, tileSize);
    pLocal2 += tileSize * step(pLocal2, f3(-1e-6));

    // sample both tile positions with all 4 channels
    float4 wrlSample1 = wrl.SampleLevel(Smp, pLocal/tileSize*.3 + .5, 0);
    float4 wrlSample2 = wrl.SampleLevel(Smp, pLocal2/tileSize*.3 + .5, 0);

    // remap perlin-worley (R) using low-freq worley FBM (GBA) as minimum
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
    static const float EROSION_STRENGTH = .2;
    float cloudMapOut = saturate(remap(cloudMap, erosionFBM * EROSION_STRENGTH, 1., 0., 1.));

    return .99 * cloudMapOut;
}

float getGlow(float dist) {
    dist = max(dist, 1e-6);
    return pow(3e-5/dist, .9);
}

// henyey-greenstein phase function
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

// separate ray-marching loop towards the sun to accumulate lighting samples
// inspired by Sebastian Lague's implementation from "Coding Adventure: Clouds" (https://www.youtube.com/watch?v=4QOcCGI6xOU)
float lightmarch(float3 p, float3 sunDirection, float mu) {
    static const int NUM_LIGHT_SAMPLES = 8;
    float2 lightDist = rayCloudDist(p, sunDirection);
    float stepSize = lightDist.y / float(NUM_LIGHT_SAMPLES);
    float totalDensity = 0.;
    for(int i = 0; i < NUM_LIGHT_SAMPLES; i++) {
        p += sunDirection * stepSize;
        totalDensity += max(0., sampleDensity(p) * stepSize);
    }
    float3 beersLaw = multipleOctaves(totalDensity, mu, stepSize);
    float3 powder = 2. * (1. - exp(-totalDensity * 2. * sigmaE));
    return lerp(beersLaw * powder, beersLaw, .5+.5*mu); // beer-powder
}

[numthreads(8,8,1)]
void cs_5_0(uint3 i : SV_DispatchThreadID) {
    // hard-coded uv coordinates calculation since output image will be 1920x1080 anyway
    float2 uv = float2(i.xy) / float2(1920., 1080.) - .5;
    uv /= float2(.5625, 1.); // .5625 = 1080./1920.
    uv.y = -uv.y; // I don't know why the y axis was flipped :?

    float3 rayOrigin = float3(0, -80, -10); // ray/camera pos
    // camera rotation code
    float3 forward = normalize(float3(.1, .6, 1.));
    float3 right = normalize(cross(forward, float3(0,1,0)));
    forward = normalize(forward + right*uv.x*.7 + cross(right, forward)*uv.y*.7);  // was .95

    float distTravelled = 0.;
    float3 transmittance = f3(1.);

    static const float NUM_SAMPLES = 64.;
    float2 uv2 = rayCloudDist(rayOrigin, forward);
    float distInsideClouds = uv2.y;
    float step_size = distInsideClouds / NUM_SAMPLES;
    float ign = frac(52.9829189 * frac(.06711056*float(i.x) + .00583715*float(i.y))); // blue-noise approximation
    float3 step_vec = step_size * forward;
    float3 ray_pos = rayOrigin + forward * uv2.x + step_vec * ign;

    static const float3 SUN_DIRECTION = normalize(float3(cos(1.), .6, sin(1.)));
    static const float SUN_SIZE = .5003;
    float mu = .5 + SUN_SIZE * dot(forward,SUN_DIRECTION); // sun

    static const float3 lightColor = float3(1., .95, .85);
    float3 backgroundColor = lerp(skyColour, .5*skyColour, .5+.5*forward.y);
    backgroundColor += lightColor * getGlow(1.-mu);

    float phaseFunction = lerp(hg(-.3, mu), hg(.3, mu), .7);
    static const float SUN_STRENGTH = 64.;
    float3 sunLight = lightColor * SUN_STRENGTH;
    float3 cl = f3(0.);

    // standard volumetric ray-marching loop
    // inspired by Sebastian Lague's implemntation from "Coding Adventure: Clouds" (https://www.youtube.com/watch?v=4QOcCGI6xOU)
    while(distTravelled < distInsideClouds) {
        ray_pos += step_vec;
        distTravelled += step_size;
        float3 toSample = ray_pos - rayOrigin;
        float distFade = 1. - saturate((length(toSample) - 400.) / 300.);
        float density = sampleDensity(ray_pos) * step_size * distFade;
        float cloudHeight = saturate((ray_pos.y - cloudsBoundsMin.y) / (cloudsBoundsMax.y - cloudsBoundsMin.y));
        float3 sampleSigmaE = sigmaE * density;

        if(density > 0.) {
            float3 ambient = lightColor * lerp(.02, .6, cloudHeight); // AO approximation based on cloud height, .02 and .6 are hard-coded values that can be tweaked freely for looks
            float3 luminance = .1*ambient + sunLight*phaseFunction*lightmarch(ray_pos, SUN_DIRECTION, dot(forward, SUN_DIRECTION));
            luminance *= sigmaS * density; 
            float3 sampleTransmittance = exp(-sampleSigmaE);
            cl += transmittance * (luminance - luminance*sampleTransmittance) / sampleSigmaE;
            transmittance *= sampleTransmittance;
            if(length(transmittance) <= .001) { transmittance = f3(0.); break; }
        }
    }

    float3 color = cl + backgroundColor * transmittance;
    static const float FADE_STRENGTH = .4;
    float horizonFade = saturate(1. - forward.y * 4.); 
    color = lerp(color, backgroundColor * 1.2, horizonFade * FADE_STRENGTH);
    color = clamp(color*(2.51*color+.03)/(color*(2.43*color+.59)+.14), 0., 1.); // inlined ACES tone-mapping (thanks Shader Minifier!)
    // apply grain effect to output color
    float grain = filmGrain(float2(i.xy) / float2(1920.,1080.));
    static const float GRAIN_STRENGTH = .08;
    color -= grain * GRAIN_STRENGTH;  
    color = saturate(color);
    y[i.xy] = color;
}