#pragma once

#include <limits>

#include "core/boolean.h"
#include "core/builtins.h"
#include "core/compiler.h"
#include "core/deque.h"
#include "core/flag_set.h"
#include "core/hash_map.h"
#include "core/matrix.h"
#include "core/meta.h"
#include "core/soa_vector.h"
#include "core/tuple.h"
#include "core/type_traits.h"
#include "core/vector.h"

using namespace soul::builtin;

namespace renderlab
{

  class EntityId
  {
  private:
    u64 id_;

    static constexpr u64 NULLID = std::numeric_limits<u64>::max();

    static constexpr u64 INDEX_BITS = 48;
    static constexpr u64 INDEX_MASK = (1LLU << INDEX_BITS) - 1;

    static constexpr u64 GENERATION_BITS = 16;
    static constexpr u64 GENERATION_MASK = (1 << GENERATION_BITS) - 1;

    [[nodiscard]]
    auto index() const -> u64
    {
      return id_ & INDEX_MASK;
    }

    [[nodiscard]]
    auto generation() const -> u64
    {
      return (id_ >> INDEX_BITS) & GENERATION_MASK;
    }

    explicit EntityId(u64 id) : id_(id) {}

    [[nodiscard]]
    static auto Create(u64 index, u64 generation) -> EntityId
    {
      return EntityId((generation << INDEX_BITS) | index);
    }

    template <typename... ComponentTs>
    friend class EntityManager;

    friend void soul_op_hash_combine(auto& hasher, const EntityId& val)
    {
      hasher.combine(val.id_);
    }

  public:
    [[nodiscard]]
    static auto Null() -> EntityId
    {
      return EntityId(NULLID);
    }

    [[nodiscard]]
    auto is_null() const -> b8
    {
      return id_ == NULLID;
    }

    auto operator==(const EntityId& other) const -> b8 = default;

    [[nodiscard]]
    auto to_underlying() -> u64
    {
      return id_;
    }
  };

  struct EntityDesc
  {
    StringView name = ""_str;
    mat4f32 local_transform;
    EntityId parent_entity_id = EntityId::Null();
  };

  class EntityView
  {
  private:
    NotNull<String*> name_view_;
    NotNull<mat4f32*> local_transform_view_;
    NotNull<mat4f32*> world_transform_view_;

    template <typename... ComponentTs>
    friend class EntityManager;

    EntityView(
      NotNull<String*> name_view,
      NotNull<mat4f32*> local_transform_view,
      NotNull<mat4f32*> world_transform_view)
        : name_view_(name_view),
          local_transform_view_(local_transform_view),
          world_transform_view_(world_transform_view)
    {
    }

  public:
    auto name_ref() -> String&
    {
      return *name_view_;
    }

    [[nodiscard]]
    auto name_ref() const -> const String&
    {
      return *name_view_;
    }

    auto local_transform_ref() -> mat4f32&
    {
      return *local_transform_view_;
    }

    [[nodiscard]]
    auto local_transform_ref() const -> const mat4f32&
    {
      return *local_transform_view_;
    }

    auto world_transform_ref() -> mat4f32&
    {
      return *world_transform_view_;
    }

    [[nodiscard]]
    auto local_transfom_ref() const -> const mat4f32&
    {
      return *world_transform_view_;
    }
  };

  template <typename ComponentT>
  class ComponentManager
  {
  private:
    HashMap<EntityId, usize> map_;
    Vector<EntityId> entities_;
    Vector<ComponentT> components_;

  public:
    void add(EntityId entity_id, OwnRef<ComponentT> component)
    {
      map_.insert(entity_id, entities_.size());
      entities_.push_back(entity_id);
      components_.push_back(std::move(component));
    }

    void remove(EntityId entity_id)
    {
      const auto index = map_[entity_id];
      map_.remove(entity_id);
      entities_.remove(index);
      components_.remove(index);
      map_[entities_[index]] = index;
    }

    [[nodiscard]]
    auto has_component(EntityId entity_id) const -> b8
    {
      return map_.contains(entity_id);
    }

    auto component_ref(EntityId entity_id) -> ComponentT&
    {
      return components_[map_[entity_id]];
    }

    auto component_ref(EntityId entity_id) const -> const ComponentT&
    {
      return components_[map_[entity_id]];
    }

    template <typename Fn>
    void for_each_with_entity_id(Fn fn)
    {
      for (usize i = 0; i < entities_.size(); i++)
      {
        std::invoke(fn, components_[i], entities_[i]);
      }
    }

    template <typename Fn>
    void for_each_with_entity_id(Fn fn) const
    {
      for (usize i = 0; i < entities_.size(); i++)
      {
        std::invoke(fn, components_[i], entities_[i]);
      }
    }

    template <typename Fn>
    void for_each(Fn fn)
    {
      for (usize i = 0; i < entities_.size(); i++)
      {
        std::invoke(fn, components_[i]);
      }
    }

    template <typename Fn>
    void for_each(Fn fn) const
    {
      for (usize i = 0; i < entities_.size(); i++)
      {
        std::invoke(fn, components_[i]);
      }
    }
  };

  struct EntityHierarchyData
  {
    EntityId parent;
    EntityId first_child;
    EntityId prev_sibling;
    EntityId next_sibling;
  };

  enum EntityStructureTag
  {
    NAME,
    LOCAL_TRANSFORM,
    WORLD_TRANSFORM,
    NORMAL_TRANSFORM,
    ENTITY_HIERARCY_DATA,
    COUNT
  };

  using EntityStructure = soul::Tuple<String, mat4f32, mat4f32, mat4f32, EntityHierarchyData>;

  template <typename... ComponentTs>
  class EntityManager
  {
  public:
    using internal_index_type = u64;
    static_assert(!has_duplicate_type_v<ComponentTs...>);

  private:
    static constexpr u32 MINIMUM_FREE_INDICES = 1024;

    struct Metadata
    {
      internal_index_type internal_index;
      u8 generation;
    };

    Vector<Metadata> metadatas_;
    Deque<u64> free_indices_;
    SoaVector<EntityStructure> entities_;
    Tuple<ComponentManager<ComponentTs>...> component_managers_;
    EntityId root_entity_;

  public:
    EntityManager() : root_entity_(EntityId::Create(0, 0))
    {
      metadatas_.push_back(Metadata{
        .internal_index = 0,
        .generation     = 0,
      });

      entities_.push_back(
        String::From("Root Entity"_str),
        mat4f32::Identity(),
        mat4f32::Identity(),
        mat4f32::Identity(),
        EntityHierarchyData{
          .parent       = EntityId::Null(),
          .first_child  = EntityId::Null(),
          .prev_sibling = EntityId::Null(),
          .next_sibling = EntityId::Null(),
        });
    }

    [[nodiscard]]
    auto create(const EntityDesc& desc) -> EntityId
    {
      const u32 internal_index = entities_.size();

      const u32 external_id = [&]()
      {
        if (free_indices_.size() > MINIMUM_FREE_INDICES)
        {
          return free_indices_.pop_front();
        } else
        {
          const auto id = metadatas_.size();
          metadatas_.push_back(Metadata{
            .internal_index = internal_index,
            .generation     = 0,
          });
          return id;
        }
      }();
      const auto entity_id = EntityId::Create(external_id, metadatas_[external_id].generation);

      const auto parent_entity_id =
        desc.parent_entity_id.is_null() ? root_entity_ : desc.parent_entity_id;
      const auto parent_structure_index = get_internal_index(parent_entity_id);
      const auto parent_world_transform = entities_.ref<WORLD_TRANSFORM>(parent_structure_index);
      const auto next_sibling =
        entities_.ref<ENTITY_HIERARCY_DATA>(parent_structure_index).first_child;
      const auto world_transform = math::mul(parent_world_transform, desc.local_transform);
      entities_.push_back(
        String::From(desc.name),
        desc.local_transform,
        world_transform,
        math::transpose(math::inverse(world_transform)),
        EntityHierarchyData{
          .parent       = parent_entity_id,
          .first_child  = EntityId::Null(),
          .prev_sibling = EntityId::Null(),
          .next_sibling = next_sibling,
        });

      entities_.ref<ENTITY_HIERARCY_DATA>(parent_structure_index).first_child = entity_id;

      if (!next_sibling.is_null())
      {
        auto& next_sibling_hierarchy_data =
          entities_.ref<ENTITY_HIERARCY_DATA>(get_internal_index(next_sibling));
        next_sibling_hierarchy_data.prev_sibling = entity_id;
      }

      return entity_id;
    }

    [[nodiscard]]
    auto root_entity_id() const -> EntityId
    {
      return root_entity_;
    }

    template <typename ComponentT>
      requires(get_type_count_v<ComponentT, ComponentTs...> == 1)
    void add_component(EntityId entity_id, OwnRef<ComponentT> component)
    {
      auto& component_manager =
        component_managers_.template ref<get_type_index_v<ComponentT, ComponentTs...>>();
      component_manager.add(entity_id, std::move(component));
    }

    template <typename ComponentT>
      requires(get_type_count_v<ComponentT, ComponentTs...> == 1)
    void remove_component(EntityId entity_id)
    {
      auto& component_manager =
        component_managers_.template ref<get_type_index_v<ComponentT, ComponentTs...>>();
      component_manager.remove(entity_id);
    }

    template <typename ComponentT>
    [[nodiscard]]
    auto has_component(EntityId entity_id) const -> b8
    {
      const auto& component_manager =
        component_managers_.template ref<get_type_index_v<ComponentT, ComponentTs...>>();
      return component_manager.has_component(entity_id);
    }

    template <typename ComponentT>
    [[nodiscard]]
    auto component_ref(EntityId entity_id) -> ComponentT&
    {
      auto& component_manager =
        component_managers_.template ref<get_type_index_v<ComponentT, ComponentTs...>>();
      return component_manager.component_ref(entity_id);
    }

    template <typename ComponentT>
    [[nodiscard]]
    auto component_ref(EntityId entity_id) const -> const ComponentT&
    {
      const auto& component_manager =
        component_managers_.template ref<get_type_index_v<ComponentT, ComponentTs...>>();
      return component_manager.component_ref(entity_id);
    }

    [[nodiscard]]
    auto is_alive(EntityId entity_id) const -> b8
    {
      return metadatas_[entity_id.index()].generation == entity_id.generation();
    }

    void destroy(EntityId entity_id)
    {
      const u32 idx = entity_id.index();
      metadatas_[idx].generation++;
      const auto internal_index                       = metadatas_[idx].internal_index;
      metadatas_[entities_.size() - 1].internal_index = internal_index;
      entities_.remove(internal_index);
      free_indices_.push_back(idx);
    }

    auto name_ref(EntityId entity_id) -> String&
    {
      return entities_.ref<NAME>(get_internal_index(entity_id));
    }

    [[nodiscard]]
    auto name_ref(EntityId entity_id) const -> const String&
    {
      return entities_.ref<NAME>(get_internal_index(entity_id));
    }

    auto world_transform_ref(EntityId entity_id) -> mat4f32&
    {
      return entities_.ref<WORLD_TRANSFORM>(get_internal_index(entity_id));
    }

    [[nodiscard]]
    auto world_transform_ref(EntityId entity_id) const -> const mat4f32&
    {
      return entities_.ref<WORLD_TRANSFORM>(get_internal_index(entity_id));
    }

    auto local_transform_ref(EntityId entity_id) -> mat4f32&
    {
      return entities_.ref<LOCAL_TRANSFORM>(get_internal_index(entity_id));
    }

    [[nodiscard]]
    auto local_transform_ref(EntityId entity_id) const -> const mat4f32&
    {
      return entities_.ref<LOCAL_TRANSFORM>(get_internal_index(entity_id));
    }

    void set_world_transform(EntityId entity_id, const mat4f32& world_transform)
    {
      const auto& hierarchy_data  = hierarchy_data_ref(entity_id);
      const auto parent_entity_id = hierarchy_data.parent;
      const auto parent_transform =
        parent_entity_id.is_null() ? mat4f32::Identity() : world_transform_ref(parent_entity_id);
      local_transform_ref(entity_id) = math::mul(math::inverse(parent_transform), world_transform);
      update_world_transform_recursive(entity_id, parent_transform);
    }

    void set_local_transform(EntityId entity_id, const mat4f32& local_transform)
    {
      local_transform_ref(entity_id) = local_transform;
      update_world_transform_recursive(entity_id);
    }

    void update_world_transform_recursive(EntityId entity_id)
    {
      const auto& hierarchy_data  = hierarchy_data_ref(entity_id);
      const auto parent_entity_id = hierarchy_data.parent;
      const auto parent_transform =
        parent_entity_id.is_null() ? mat4f32::Identity() : world_transform_ref(parent_entity_id);
      update_world_transform_recursive(entity_id, parent_transform);
    }

    void update_world_transform_recursive(EntityId entity_id, const mat4f32& parent_transform)
    {
      world_transform_ref(entity_id) = math::mul(parent_transform, local_transform_ref(entity_id));
      const auto& hierarchy_data     = hierarchy_data_ref(entity_id);
      EntityId child                 = hierarchy_data.first_child;
      while (!child.is_null())
      {
        update_world_transform_recursive(child, world_transform_ref(entity_id));
        child = hierarchy_data_ref(child).next_sibling;
      }
    }

    [[nodiscard]]
    auto hierarchy_data_ref(EntityId entity_id) const -> const EntityHierarchyData&
    {
      return entities_.ref<ENTITY_HIERARCY_DATA>(get_internal_index(entity_id));
    }

    [[nodiscard]]
    auto name_cspan() const -> Span<const String*>
    {
      return entities_.cspan<NAME>();
    }

    [[nodiscard]]
    auto world_transform_cspan() const -> Span<const mat4f32*>
    {
      return entities_.cspan<WORLD_TRANSFORM>();
    }

    [[nodiscard]]
    auto local_transform_cspan() const -> Span<const mat4f32*>
    {
      return entities_.cspan<LOCAL_TRANSFORM>();
    }

    [[nodiscard]]
    auto normal_transform_cspan() const -> Span<const mat4f32*>
    {
      return entities_.cspan<NORMAL_TRANSFORM>();
    }

    [[nodiscard]]
    auto entity_view(EntityId entity_id) -> EntityView
    {
      const auto internal_index  = get_internal_index(entity_id);
      auto* name_view            = &entities_.ref<NAME>(internal_index);
      auto* local_transform_view = &entities_.ref<LOCAL_TRANSFORM>(internal_index);
      auto* world_transform_view = &entities_.ref<WORLD_TRANSFORM>(internal_index);
      return EntityView(name_view, local_transform_view, world_transform_view);
    }

    [[nodiscard]]
    auto entity_count() const -> usize
    {
      return entities_.size();
    }

    [[nodiscard]]
    auto is_empty() const -> b8
    {
      return entities_.empty();
    }

    [[nodiscard]]
    auto get_internal_index(EntityId entity_id) const -> internal_index_type
    {
      return metadatas_[entity_id.index()].internal_index;
    }

    template <typename ComponentT, typename Fn>
    void for_each_component(Fn fn)
    {
      static_assert(
        get_type_count_v<ComponentT, ComponentTs...> == 1,
        "Component is not registered withing EntityManager");

      auto& component_manager =
        component_managers_.template ref<get_type_index_v<ComponentT, ComponentTs...>>();
      component_manager.for_each(fn);
    }

    template <typename ComponentT, typename Fn>
    void for_each_component(Fn fn) const
    {
      static_assert(
        get_type_count_v<ComponentT, ComponentTs...> == 1,
        "Component is not registered withing EntityManager");

      const auto& component_manager =
        component_managers_.template ref<get_type_index_v<ComponentT, ComponentTs...>>();
      component_manager.for_each(fn);
    }

    template <typename ComponentT, typename Fn>
    void for_each_component_with_entity_id(Fn fn)
    {
      static_assert(
        get_type_count_v<ComponentT, ComponentTs...> == 1,
        "Component is not registered withing EntityManager");

      auto& component_manager =
        component_managers_.template ref<get_type_index_v<ComponentT, ComponentTs...>>();
      component_manager.for_each_with_entity_id(fn);
    }

    template <typename ComponentT, typename Fn>
    void for_each_component_with_entity_id(Fn fn) const
    {
      static_assert(
        get_type_count_v<ComponentT, ComponentTs...> == 1,
        "Component is not registered withing EntityManager");

      const auto& component_manager =
        component_managers_.template ref<get_type_index_v<ComponentT, ComponentTs...>>();
      component_manager.for_each_with_entity_id(fn);
    }
  };

} // namespace renderlab
