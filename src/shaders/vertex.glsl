#version 450

layout(location = 0) in vec3 pos;

layout(binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
} cam_ubo;

layout(binding = 1) uniform TransformBuffer {
    mat4 pos;
    mat4 rot;
} mesh_ubo;


// vec3 vertColors[3] = {
// 	vec3(0.075,0.094,0.259),
// 	vec3(0.902,0.514,0.412),
// 	vec3(0.925,0.808,0.682)
// };

vec3 vertColors[3] = {
	vec3(1.,0.,0.),
	vec3(0.,1.,0.),
	vec3(0.,0.,1.)
};

layout(location = 0) out vec3 outColor;

void main() {
    gl_Position = cam_ubo.proj * cam_ubo.view * mesh_ubo.pos * mesh_ubo.rot * vec4(pos, 1.0);
    outColor = vertColors[(gl_VertexIndex) % 3];
}
