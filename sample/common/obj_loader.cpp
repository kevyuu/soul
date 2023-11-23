#include "obj_loader.h"

#include <filesystem>

#include "core/log.h"
#include "math/math.h"

auto ObjLoader::load_model(const soul::Path& filepath) -> void
{
  tinyobj::ObjReader reader;
  reader.ParseFromFile(filepath.string());
  if (!reader.Valid()) {
    SOUL_LOG_INFO("{}", reader.Error().c_str());
    SOUL_LOG_ERROR("Cannot load: {}", filepath.string().c_str());
    assert(reader.Valid());
  }

  std::map<std::string, int> texture_name_index_map;

  // Collecting the material in the scene
  for (const auto& material : reader.GetMaterials()) {
    MaterialObj m;
    m.ambient = soul::vec3f(material.ambient[0], material.ambient[1], material.ambient[2]);
    m.diffuse = soul::vec3f(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
    m.specular = soul::vec3f(material.specular[0], material.specular[1], material.specular[2]);
    m.emission = soul::vec3f(material.emission[0], material.emission[1], material.emission[2]);
    m.transmittance =
      soul::vec3f(material.transmittance[0], material.transmittance[1], material.transmittance[2]);
    m.dissolve = material.dissolve;
    m.ior = material.ior;
    m.shininess = material.shininess;
    m.illum = material.illum;
    if (!material.diffuse_texname.empty()) {
      if (const auto it = texture_name_index_map.find(material.diffuse_texname);
          it != texture_name_index_map.end()) {
        m.texture_id = it->second;
      } else {
        const auto texture_id = soul::cast<int>(textures.size());
        texture_name_index_map[material.diffuse_texname] = texture_id;
        m.texture_id = texture_id;
        textures.emplace_back(material.diffuse_texname);
      }
    }

    materials.emplace_back(m);
  }

  // If there were none, add a default
  if (materials.empty()) {
    materials.emplace_back(MaterialObj());
  }

  const tinyobj::attrib_t& attrib = reader.GetAttrib();

  for (const auto& shape : reader.GetShapes()) {
    vertices.reserve(shape.mesh.indices.size() + vertices.size());
    indices.reserve(shape.mesh.indices.size() + indices.size());
    mat_indexes.append(shape.mesh.material_ids);

    for (const auto& index : shape.mesh.indices) {
      VertexObj vertex = {};
      const float* vp = &attrib.vertices[3 * index.vertex_index];
      vertex.position = {*(vp + 0), *(vp + 1), *(vp + 2)};

      if (!attrib.normals.empty() && index.normal_index >= 0) {
        const float* np = &attrib.normals[3 * index.normal_index];
        vertex.normal = {*(np + 0), *(np + 1), *(np + 2)};
      }

      if (!attrib.texcoords.empty() && index.texcoord_index >= 0) {
        const float* tp = &attrib.texcoords[2 * index.texcoord_index + 0];
        vertex.tex_coord = {*tp, 1.0f - *(tp + 1)};
      }

      if (!attrib.colors.empty()) {
        const float* vc = &attrib.colors[3 * index.vertex_index];
        vertex.color = {*(vc + 0), *(vc + 1), *(vc + 2)};
      }

      vertex.position *= 4;

      vertices.push_back(vertex);
      indices.push_back(static_cast<int>(indices.size()));
    }
  }

  // Fixing material indices
  for (auto& mi : mat_indexes) {
    if (mi < 0 || mi > soul::cast<i32>(materials.size())) {
      mi = 0;
    }
  }

  // Compute normal when no normal were provided.
  if (attrib.normals.empty()) {
    for (size_t i = 0; i < indices.size(); i += 3) {
      VertexObj& v0 = vertices[indices[i + 0]];
      VertexObj& v1 = vertices[indices[i + 1]];
      VertexObj& v2 = vertices[indices[i + 2]];

      soul::vec3f n = soul::math::normalize(
        soul::math::cross((v1.position - v0.position), (v2.position - v0.position)));
      v0.normal = n;
      v1.normal = n;
      v2.normal = n;
    }
  }

  for (const auto& vertex : vertices) {
    bounding_box = soul::math::combine(bounding_box, vertex.position);
  }
}
