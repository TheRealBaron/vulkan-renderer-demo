#include "scene/Mesh.hpp"
#include "tiny_obj_loader.h"
#include "resources/vertex_data.h"


Mesh::Mesh(GraphicsContext *const gc, CommandManager *const cmdmg, size_t bufcnt, std::filesystem::path obj_path)
        : vertex_buffer(gc, cmdmg), index_buffer(gc, cmdmg), transform_buffer(gc, bufcnt) {

    
    std::vector<uint8_t> vertices, indices;
    load_data(obj_path, vertices, indices);
    
    vertex_buffer.load(
        static_cast<void*>(vertices.data()),
        vertices.size(),
        ReadonlyBuffer::Usage::VERTEX
    );
    index_buffer.load(
        static_cast<void*>(indices.data()),
        indices.size(),
        ReadonlyBuffer::Usage::INDEX
    );
    transform_buffer.load();
    
}

Mesh::~Mesh() {
    transform_buffer.unload();
    index_buffer.unload();
    vertex_buffer.unload();
}

void Mesh::load_data(
    std::filesystem::path obj_path, 
    std::vector<uint8_t>& vertex_data, 
    std::vector<uint8_t>& index_data) {
    

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    
    // Load the OBJ file
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, 
                                obj_path.string().c_str());
    
    if (!err.empty()) {
        throw std::runtime_error("could not load .obj file: " + err);
    }
    
    if (!ret) {
        throw std::runtime_error("could not load .obj file");
    }
    
    vertex_data.clear();
    index_data.clear();
    
    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            size_t fv = shape.mesh.num_face_vertices[f];
            
            
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                
                float vx = attrib.vertices[3 * idx.vertex_index + 0];
                float vy = attrib.vertices[3 * idx.vertex_index + 1];
                float vz = attrib.vertices[3 * idx.vertex_index + 2];
                
                float nx = 0.0f, ny = 0.0f, nz = 0.0f;
                if (idx.normal_index >= 0) {
                    nx = attrib.normals[3 * idx.normal_index + 0];
                    ny = attrib.normals[3 * idx.normal_index + 1];
                    nz = attrib.normals[3 * idx.normal_index + 2];
                }
                
                // Pack vertex data: position (3 floats) + normal (3 floats) = 24 bytes
                const float* vertex_attrs[] = {&vx, &vy, &vz, &nx, &ny, &nz};
                for (const float* attr : vertex_attrs) {
                    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(attr);
                    vertex_data.insert(vertex_data.end(), bytes, bytes + sizeof(float));
                }
                
                // Pack index data (assuming 32-bit indices)
                uint32_t current_index = static_cast<uint32_t>(vertex_data.size() / 24 - 1);
                const uint8_t* index_bytes = reinterpret_cast<const uint8_t*>(&current_index);
                index_data.insert(index_data.end(), index_bytes, index_bytes + sizeof(uint32_t));
            }
            index_offset += fv;
        }
    }
}


