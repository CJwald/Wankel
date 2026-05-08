#version 330 core

in vec4 v_Color;
in vec3 v_WorldPos;

out vec4 FragColor;

uniform vec3 u_CameraPos;

uniform vec3 u_FogColor;
uniform float u_FogDensity;

uniform int u_FogNoiseEnabled;

uniform float u_Time;
uniform float u_FogNoiseScale;
uniform float u_FogNoiseStrength;
uniform float u_FogNoiseSpeed;
uniform int u_FogNoiseOctaves;

uniform float u_FogHeightFalloff;

float hash(vec2 p)
{
    return fract(
        sin(dot(p, vec2(127.1, 311.7)))
        * 43758.5453123
    );
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x)
         + (c - a) * u.y * (1.0 - u.x)
         + (d - b) * u.x * u.y;
}

float fbm(vec2 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for(int i = 0; i < u_FogNoiseOctaves; i++)
    {
        value += amplitude *
                 noise(p * frequency);

        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

void main()
{
    vec3 color = v_Color.rgb;

    float dist =
        length(u_CameraPos - v_WorldPos);

    float fogDensity = u_FogDensity;

    // =========================
    // NOISE FOG
    // =========================

    if(u_FogNoiseEnabled == 1)
    {
        vec2 fogUV =
            v_WorldPos.xz * u_FogNoiseScale;

        fogUV +=
            vec2(u_Time * u_FogNoiseSpeed);

        float n = fbm(fogUV);

        float noiseMod =
            mix(1.0,
                n,
                u_FogNoiseStrength);

        fogDensity *= noiseMod;
    }

    // =========================
    // HEIGHT FOG
    // =========================

    float heightFog =
        exp(-max(v_WorldPos.y, 0.0)
             * u_FogHeightFalloff);

    // =========================
    // DISTANCE FOG
    // =========================

    float fogFactor =
        exp(-pow(dist * fogDensity, 2.0));

    fogFactor *= heightFog;

    fogFactor =
        clamp(fogFactor, 0.0, 1.0);

    vec3 finalColor =
        mix(u_FogColor,
            color,
            fogFactor);

    FragColor =
        vec4(finalColor, v_Color.a);
}
