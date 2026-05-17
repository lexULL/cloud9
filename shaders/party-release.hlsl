Texture3D<float4> wrl;
SamplerState Smp;
RWTexture2D<float3> y;

float3 f3(float x) { return float3(x,x,x); }

static const float3 cloudsBoundsMin = float3(-2e2, -4., -2e2);
static const float3 cloudsBoundsMax = float3(2e2, 4., 2e2);
static const float3 sigmaS = f3(1.);
static const float3 sigmaA = f3(0.1);
static const float3 sigmaE = max(sigmaS + sigmaA, f3(1e-6));
static const float3 skyColour = .7 * float3(.09, .33, .81);
static const float3 tileSize = float3(20., 8., 20.);

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
    float tempToCloud = linePlaneIntersection(rayOrigin, rayDirection, float3(0,1,0),float3(0,199,0));
    
    rayDirection.y += (1.-((rayDirection.y + 1.) * .5))*.3;
    rayDirection = normalize(rayDirection);
    
    float toCloud = linePlaneIntersection(rayOrigin, rayDirection, float3(0,1,0),float3(0,199,0));
    float inCloud = linePlaneIntersection(rayOrigin, rayDirection, float3(0,1,0),float3(0,200,0));

    // toCloud = toCloud > 200. ? 0. : toCloud;
    // dot(rayDirection,vec3(0,1,0))
    if(inCloud > 0.) inCloud = min(inCloud,10.)*rayDirection.y;
    return float2(toCloud,inCloud);
}

float remap(float v, float lo, float hi, float newLo, float newHi) {
    return newLo + (v - lo) / (hi - lo) * (newHi - newLo);
}

float sampleDensity(float3 p) {
    float3 uvw = (p - cloudsBoundsMin) / (cloudsBoundsMax - cloudsBoundsMin);
    float cloudMap = saturate((wrl.SampleLevel(Smp, uvw * .3 + float3(.5,.5,.5), 0).x - .7) * 3.);
    float cloudMap2 = saturate((p.y - cloudsBoundsMin.y) / (cloudsBoundsMax.y - cloudsBoundsMin.y));
    float cloudMap1 = pow(cloudMap, .75);
    cloudMap *= saturate(remap(cloudMap2, 0., .25*(1.-cloudMap), 0., 1.))
              * saturate(remap(cloudMap2, .75*cloudMap1, cloudMap1, 1., 0.));
    if(cloudMap <= 0.) return 0.;
    float3 uvw2 = uvw * .9941 + float3(.3, .1, .7);
    float4 noise = wrl.SampleLevel(Smp, uvw2, 0);
    cloudMap1 = saturate(remap(cloudMap, .01*noise.x, 1., 0., 1.));
    if(cloudMap1 <= 0.) return 0.;
    float3 uvw3 = uvw + float3(.3, .1, .7);
    noise = wrl.SampleLevel(Smp, uvw3, 0);
    cloudMap1 = saturate(remap(cloudMap1, .5*(noise.y*.625 + noise.z*.25 + noise.w*.125), 1., 0., 1.));
    return .99 * cloudMap1;
}

float getGlow(float dist) {
    dist = max(dist, 1e-6);
    return pow(15e-5/dist, .9);
}

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
    float stepSize = lightDist.y / float(4);
    float totalDensity = 0.;
    for(int i = 0; i < 4; i++) {
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

    float3 rayOrigin = float3(0, -16, -10);
    float3 forward = normalize(float3(.3, .6, 1.));
    float3 right = normalize(cross(forward, float3(0,1,0)));
    forward = normalize(forward + right*uv.x*.95 + cross(right, forward)*uv.y*.95);

    float distTravelled = 0.;
    float3 transmittance = f3(1.);

    float2 uv2 = rayCloudDist(rayOrigin, forward);
    float distInsideClouds = uv2.y;
    float step_size = distInsideClouds / 32.;
    float ign = frac(52.9829189 * frac(.06711056*float(i.x) + .00583715*float(i.y)));
    float3 step_vec = step_size * forward;
    float3 ray_pos = rayOrigin + forward * uv2.x + step_vec * ign;

    float3 sunDirection = normalize(float3(cos(1.), .6, sin(1.)));
    float mu = .5 + .5007 * dot(forward, sunDirection);

    float3 lightColor = f3(1.);
    float3 backgroundColor = lerp(skyColour, .5*skyColour, .5+.5*forward.y);
    backgroundColor += lightColor * getGlow(1.-mu);

    float phaseFunction = lerp(hg(-.3, mu), hg(.3, mu), .7);
    float3 sunLight = lightColor * 50.;
    float3 cl = f3(0.);

    while(distTravelled < distInsideClouds) {
        ray_pos += step_vec;
        distTravelled += step_size;
        float density = sampleDensity(ray_pos) * step_size;
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
    color = clamp(color*(2.51*color+.03)/(color*(2.43*color+.59)+.14), 0., 1.);
    float grain = filmGrain(float2(i.xy) / float2(1920.,1080.));
    color -= grain *.08;
    color = saturate(color);
    y[i.xy] = color;
}