#version 450


layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec3 pos;
layout(location = 2) in vec3 norm;

layout(location = 0) out vec4 out_color;
layout(binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
    vec3 pos;
} cam_ubo;



void main() {
    float sc = dot(normalize(norm), normalize(pos - cam_ubo.pos)) + 2.f;
    out_color = vec4(frag_color * sc, 1.0f);
}

