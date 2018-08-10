#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Interpolated Inputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
layout (location = 0) in vec3 lightPosition;
layout (location = 1) in vec3 eyePosition;

layout (location = 2) in vec3 worldPosition;
layout (location = 3) in vec3 worldNormal;
layout (location = 4) flat in vec4 material;
layout (location = 5) in vec3 color;
layout (location = 6) in vec2 uvs;

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Per-Fragment Outputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
layout (location = 0) out vec4 outColor;

// 2D white noise function
float random (vec2 co)
	{ // rand
    return 0.5 + (abs(fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453)) * 0.5); 
    } // rand

void main () 
    { // main

    vec3 l = normalize(lightPosition - worldPosition);
    vec3 n = normalize(worldNormal + vec3(
                random(vec2(worldNormal.x)) * material.y,
                random(vec2(worldNormal.y)) * material.y,
                random(vec2(worldNormal.z)) * material.y));

    float d = dot (n, l);

    float diffuse = max (d, 0.0) * material.x;

    float metallic = pow (max(d, 0.0), 256) * material.z;
        if (d <= 1.0) metallic = metallic * (d);
        if (d <  0.0) metallic = 0.0;

    float noise = random (uvs) * material.w;
        if (d <= 1.0) noise = noise * (d);
        if (d <  0.0) noise = 0.0;

    outColor = vec4((color * diffuse) + metallic + noise, 1.0);

    } // main