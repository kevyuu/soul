/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "core/aabb.h"
#include "core/path.h"
#include "core/type.h"
#include "core/vector.h"
#include "tiny_obj_loader.h"

#include "builtins.h"

// Structure holding the material
struct MaterialObj {
  soul::vec3f ambient = soul::vec3f(0.1f, 0.1f, 0.1f);
  soul::vec3f diffuse = soul::vec3f(0.7f, 0.7f, 0.7f);
  soul::vec3f specular = soul::vec3f(1.0f, 1.0f, 1.0f);
  soul::vec3f transmittance = soul::vec3f(0.0f, 0.0f, 0.0f);
  soul::vec3f emission = soul::vec3f(0.0f, 0.0f, 0.10);
  float shininess = 0.f;
  float ior = 1.0f;     // index of refraction
  float dissolve = 1.f; // 1 == opaque; 0 == fully transparent
  // illumination model (see http://www.fileformat.info/format/material/)
  int illum = 0;
  int texture_id = -1;
};

// OBJ representation of a vertex
// NOTE: BLAS builder depends on pos being the first member
struct VertexObj {
  soul::vec3f position;
  soul::vec3f normal;
  soul::vec3f color;
  soul::vec2f tex_coord;
};

struct ShapeObj {
  uint32_t offset;
  uint32_t index_count;
  uint32_t mat_index;
};

using IndexObj = u32;
using MaterialIndexObj = i32;

class ObjLoader
{
public:
  auto load_model(const soul::Path& filepath) -> void;

  soul::Vector<VertexObj> vertices;
  soul::Vector<IndexObj> indices;
  soul::Vector<MaterialObj> materials;
  soul::Vector<std::string> textures;
  soul::Vector<MaterialIndexObj> mat_indexes;
  soul::AABB bounding_box;
};
