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
    mat4 model = mesh_ubo.pos * mesh_ubo.rot * mesh_ubo.scl;
    vec4 global_pos = model * vec4(pos, 1.0);
    
    mat3 normal_matrix = transpose(inverse(mat3(mesh_ubo.rot * mesh_ubo.scl)));
    vec3 global_norm = normal_matrix * norm;
    
    gl_Position = cam_ubo.proj * cam_ubo.view * global_pos;
    pos_to_frag = global_pos.xyz;
    norm_to_frag = global_norm;
}
