#pragma once

namespace Wankel::Noise {
float Hash(float x, float y);

float ValueNoise(float x, float y);

float PerlinNoise(float x, float y);

float FBM(float x, float y, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f);

float PerlinFBM(float x, float y, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f);

// 3D gradient (Perlin) noise, for volumetric density fields (voxel terrain etc).
// Unlike the 2D PerlinNoise above, this returns the natural signed ~[-1,1]
// range instead of remapping to [0,1] - callers building an SDF-style density
// field (negative=empty, positive=solid, see VoxelDensityField) want that
// range directly with no extra rescale.
float PerlinNoise(float x, float y, float z);

// Amplitude-normalized (divides by the sum of octave amplitudes), unlike the
// 2D PerlinFBM above - keeps output in a consistent ~[-1,1] range regardless
// of octave count, which matters when Octaves is a live-tunable parameter.
float PerlinFBM(float x, float y, float z, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f);
} // namespace Wankel::Noise
