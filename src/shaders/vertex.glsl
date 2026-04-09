#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;

layout(binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
    vec3 pos;
} cam_ubo;

layout(binding = 1) uniform TransformBuffer {
    mat4 pos;
    mat4 rot;
    mat4 scl;
} mesh_ubo;


//vec3 vertColors[3] = {
//	vec3(0.075,0.094,0.259),
//	vec3(0.902,0.514,0.412),
//	vec3(0.925,0.808,0.682)
//};

// vec3 vertColors[3] = {
// 	vec3(1.,0.,0.),
// 	vec3(0.,1.,0.),
// 	vec3(0.,0.,1.)
// };

layout(location = 0) out vec3 pos_to_frag;
layout(location = 1) out vec3 norm_to_frag;

void main() {
    vec4 global_pos = mesh_ubo.rot * mesh_ubo.pos * vec4(pos, 1.0);
    vec4 global_norm = mesh_ubo.rot * vec4(norm, 1.f);
    
    gl_Position = cam_ubo.proj * cam_ubo.view * global_pos;
    pos_to_frag = global_pos.xyz;
    norm_to_frag = global_norm.xyz;
}
