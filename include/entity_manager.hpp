#pragma once

#include <cstdint>
#include <unordered_set>

using EntityID = uint32_t;

class EntityManager {
public:
  EntityManager() : next_id(1) {}

  // Create a new entity and return its unique ID
  EntityID create() {
    EntityID id = next_id++;
    active_entities.insert(id);
    return id;
  }

  // Optionally destroy an entity (removes from active set)
  void destroy(EntityID id) { active_entities.erase(id); }

  // Check if an entity is still active
  bool is_active(EntityID id) const {
    return active_entities.find(id) != active_entities.end();
  }

private:
  EntityID next_id;
  std::unordered_set<EntityID> active_entities;
};
