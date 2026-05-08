#pragma once

namespace Wankel::Noise {
    float Hash(float x, float y);

    float ValueNoise(float x, float y);

	float PerlinNoise(float x, float y);

    float FBM(float x, float y,
              int octaves = 4,
              float lacunarity = 2.0f,
              float gain = 0.5f);

	float PerlinFBM(float x,
                    float y,
                    int octaves = 4,
                    float lacunarity = 2.0f,
                    float gain = 0.5f);
}
