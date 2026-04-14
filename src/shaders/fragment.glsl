#version 450

const vec3 frag_color = vec3(0.25f, 0.25f, 1.f);
layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec3 frag_norm;

layout(location = 0) out vec4 out_color;
layout(binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
    vec3 pos;
} cam_ubo;


struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};



vec3 phong(in PointLight light, in float shine, in vec3 normal, in vec3 view_dir) {
    float d = length(frag_pos - light.position);
    float attitude = 1.f / (light.constant + light.linear * d + light.quadratic * d * d);

    vec3 light_dir = normalize(light.position - frag_pos);

    float diff = max(0, dot(normal, light_dir));
    
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(0, dot(reflect_dir, view_dir)), 128.f * shine);

    vec3 ambient = light.ambient * frag_color.rgb;

    vec3 diffuse = light.diffuse * diff * frag_color.rgb;

    vec3 specular = light.specular * spec * frag_color.rgb;

    return (ambient + diffuse + specular) * attitude;

}


vec3 blinn_phong(in PointLight light, in float shine, in vec3 normal, in vec3 view_dir) {
    float d = length(frag_pos - light.position);
    float attitude = 1.f / (light.constant + light.linear * d + light.quadratic * d * d);

    vec3 light_dir = normalize(light.position - frag_pos);

    float diff = max(0, dot(normal, light_dir));
    
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float spec = pow(max(0, dot(normal, halfway_dir)), 128.f * shine);

    vec3 ambient = light.ambient * frag_color.rgb;

    vec3 diffuse = light.diffuse * diff * frag_color.rgb;

    vec3 specular = light.specular * spec * frag_color.rgb;

    return (ambient + diffuse + specular) * attitude;
}


const PointLight light_source = PointLight(
    vec3(0.f, 15.f, -15.f),
    vec3(0.08, 0.08, 0.08),      
    vec3(1.4, 1.4, 1.4),         
    vec3(1.6, 1.6, 1.6),        
    1.0,
    0.03,                        
    0.01                         
);

void main() {
    
    vec3 final_col = blinn_phong(
        light_source,
        0.5f,
        normalize(frag_norm),
        normalize(cam_ubo.pos - frag_pos)
    );
    out_color = vec4(final_col, 1.f);
}

