#include "Noise.h"
#include "Math.h"
#include <cmath>


static float Fade(float t) {

    return t * t * t *
           (t * (t*6.0f-15.0f) + 10.0f);
}


static float Grad(int hash, float x, float y) {

    switch(hash & 3) {
        case 0: return  x + y;
        case 1: return -x + y;
        case 2: return  x - y;
        case 3: return -x - y;
    }

    return 0.0f;
}


namespace Wankel::Noise {

    static float Fract(float x) {
        return x - std::floor(x);
    }

    static float Lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    static float Smooth(float t) {
        return t * t * (3.0f - 2.0f * t);
    }

    float Hash(float x, float y) {
        return Fract(std::sin(x * 127.1f + y * 311.7f) * 43758.5453123f);
    }

    float ValueNoise(float x, float y) {
        float ix = std::floor(x);
        float iy = std::floor(y);

        float fx = Fract(x);
        float fy = Fract(y);

        float a = Hash(ix, iy);
        float b = Hash(ix + 1.0f, iy);
        float c = Hash(ix, iy + 1.0f);
        float d = Hash(ix + 1.0f, iy + 1.0f);

        fx = Smooth(fx);
        fy = Smooth(fy);

        float u = Lerp(a, b, fx);
        float v = Lerp(c, d, fx);

        return Lerp(u, v, fy);
    }

    float FBM(float x, float y, int octaves, float lacunarity, float gain) {
        float value = 0.0f;
        float amplitude = 0.5f;
        float frequency = 1.0f;

        for(int i = 0; i < octaves; i++) {
            value += amplitude *
                     ValueNoise(x * frequency,
                                y * frequency);

            frequency *= lacunarity;
            amplitude *= gain;
        }

        return value;
    }

	float PerlinNoise(float x, float y) {

	    int xi = (int)std::floor(x) & 255;
	    int yi = (int)std::floor(y) & 255;
	
	    float xf = x - std::floor(x);
	    float yf = y - std::floor(y);
	
	    float u = Fade(xf);
	    float v = Fade(yf);
	
	    int aa = (int)(Hash(xi, yi) * 255.0f);
	    int ab = (int)(Hash(xi, yi + 1) * 255.0f);
	    int ba = (int)(Hash(xi + 1, yi) * 255.0f);
	    int bb = (int)(Hash(xi + 1, yi + 1) * 255.0f);
	
	    float x1 = Math::Lerp(
	        Grad(aa, xf, yf),
	        Grad(ba, xf - 1.0f, yf),
	        u
	    );
	
	    float x2 = Math::Lerp(
	        Grad(ab, xf, yf - 1.0f),
	        Grad(bb, xf - 1.0f, yf - 1.0f),
	        u
	    );
	
	    return Math::Lerp(x1, x2, v) * 0.5f + 0.5f;
	}

	float PerlinFBM(float x, float y, int octaves, float lacunarity, float gain) {

	    float value = 0.0f;
	    float amplitude = 0.5f;
	    float frequency = 1.0f;
	
	    for(int i = 0; i < octaves; i++) {
	        value += amplitude *
	                 PerlinNoise(
	                     x * frequency,
	                     y * frequency
	                 );
	
	        frequency *= lacunarity;
	        amplitude *= gain;
	    }
	
	    return value;
	}
}
