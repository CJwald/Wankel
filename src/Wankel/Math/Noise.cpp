#include "Noise.h"
#include "Math.h"
#include <cmath>
#include <cstdint>
#include <utility>


static float Fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}


static float Grad(int hash, float x, float y) {
    switch (hash & 3) {
        case 0:
            return x + y;
        case 1:
            return -x + y;
        case 2:
            return x - y;
        case 3:
            return -x - y;
    }

    return 0.0f;
}


// 12-direction edge gradients for 3D Perlin noise (Ken Perlin's improved-noise
// formulation - the low bits of the hash pick one of the 12 cube-edge
// directions rather than an arbitrary vector, avoiding directional bias).
static float Grad3(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}


// Fixed-seed shuffle of 0..255, duplicated to 512 entries so lookups never
// need to wrap/mask mid-calculation (standard Perlin-noise permutation-table
// trick). Noise stays stateless/seedless by design - callers vary results by
// offsetting input coordinates, not by reseeding this table.
namespace {
struct PermutationTable {
    int Values[512];

    PermutationTable() {
        int p[256];
        for (int i = 0; i < 256; i++)
            p[i] = i;

        uint32_t state = 2166136261u;
        auto NextRand = [&state]() {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return state;
        };

        for (int i = 255; i > 0; i--) {
            int j = (int)(NextRand() % (uint32_t)(i + 1));
            std::swap(p[i], p[j]);
        }

        for (int i = 0; i < 512; i++)
            Values[i] = p[i & 255];
    }
};

const PermutationTable s_Perm;
} // namespace


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

    for (int i = 0; i < octaves; i++) {
        value += amplitude * ValueNoise(x * frequency, y * frequency);

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

    float x1 = Math::Lerp(Grad(aa, xf, yf), Grad(ba, xf - 1.0f, yf), u);

    float x2 = Math::Lerp(Grad(ab, xf, yf - 1.0f), Grad(bb, xf - 1.0f, yf - 1.0f), u);

    return Math::Lerp(x1, x2, v) * 0.5f + 0.5f;
}

float PerlinFBM(float x, float y, int octaves, float lacunarity, float gain) {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;

    for (int i = 0; i < octaves; i++) {
        value += amplitude * PerlinNoise(x * frequency, y * frequency);

        frequency *= lacunarity;
        amplitude *= gain;
    }

    return value;
}

float PerlinNoise(float x, float y, float z) {
    int X = (int)std::floor(x) & 255;
    int Y = (int)std::floor(y) & 255;
    int Z = (int)std::floor(z) & 255;

    float xf = x - std::floor(x);
    float yf = y - std::floor(y);
    float zf = z - std::floor(z);

    float u = Math::SmootherStep(xf);
    float v = Math::SmootherStep(yf);
    float w = Math::SmootherStep(zf);

    const int* perm = s_Perm.Values;

    int A = perm[X] + Y;
    int AA = perm[A] + Z;
    int AB = perm[A + 1] + Z;
    int B = perm[X + 1] + Y;
    int BA = perm[B] + Z;
    int BB = perm[B + 1] + Z;

    float x1 = Math::Lerp(Grad3(perm[AA], xf, yf, zf), Grad3(perm[BA], xf - 1.0f, yf, zf), u);
    float x2 = Math::Lerp(Grad3(perm[AB], xf, yf - 1.0f, zf), Grad3(perm[BB], xf - 1.0f, yf - 1.0f, zf), u);
    float y1 = Math::Lerp(x1, x2, v);

    float x3 = Math::Lerp(Grad3(perm[AA + 1], xf, yf, zf - 1.0f), Grad3(perm[BA + 1], xf - 1.0f, yf, zf - 1.0f), u);
    float x4 = Math::Lerp(Grad3(perm[AB + 1], xf, yf - 1.0f, zf - 1.0f),
                          Grad3(perm[BB + 1], xf - 1.0f, yf - 1.0f, zf - 1.0f), u);
    float y2 = Math::Lerp(x3, x4, v);

    return Math::Lerp(y1, y2, w);
}

float PerlinFBM(float x, float y, float z, int octaves, float lacunarity, float gain) {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    float maxAmplitude = 0.0f;

    for (int i = 0; i < octaves; i++) {
        value += amplitude * PerlinNoise(x * frequency, y * frequency, z * frequency);
        maxAmplitude += amplitude;

        frequency *= lacunarity;
        amplitude *= gain;
    }

    return maxAmplitude > 0.0f ? value / maxAmplitude : 0.0f;
}
} // namespace Wankel::Noise
