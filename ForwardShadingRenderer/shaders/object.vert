#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Uniforms
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define MAX_OBJECTS 64
layout (binding = 0) uniform UniformBuffer {
    mat4 model [MAX_OBJECTS];
    mat4 view;
    mat4 proj;

    vec3 lightPosition;
    vec3 eyePosition;

    vec4 materials[MAX_OBJECTS];

} uniforms;

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Per Vertex Inputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 color;
layout (location = 3) in vec2 uvs;
layout (location = 4) in int  id;

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  PerVertex Outputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
 out gl_PerVertex
	{ vec4 gl_Position; };

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Interpolated Outputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
layout (location = 0) out vec3 frag_lightPosition;
layout (location = 1) out vec3 frag_eyePosition;

layout (location = 2) out vec3 frag_worldPosition;
layout (location = 3) out vec3 frag_worldNormal;
layout (location = 4) flat out vec4 frag_material;
layout (location = 5) out vec3 frag_color;
layout (location = 6) out vec2 frag_uvs;

void main () 
    { // main

    vec4 worldPosition = uniforms.model[id] * vec4(position, 1.0);
    vec4 worldNormal   = vec4(normal, 0.0);

  //  gl_Position = vec4(-1.0 + (uvs.s * 2.0), 1.0 - (uvs.t * 2.0), 0.0, 1.0);
    
    gl_Position = uniforms.proj * uniforms.view * worldPosition;
    
    frag_lightPosition = ( vec4(uniforms.lightPosition.xyz, 1.0)).xyz;
    frag_eyePosition   = ( vec4(uniforms.eyePosition.xyz, 1.0)).xyz;

    frag_worldPosition = worldPosition.xyz;
    frag_worldNormal   = mat3(transpose(inverse(uniforms.model[gl_InstanceIndex]))) * worldNormal.xyz;
    frag_material      = uniforms.materials[id];
    frag_color         = color;
    frag_uvs           = uvs;

    } // main