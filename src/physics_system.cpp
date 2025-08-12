#include "physics_system.hpp"
#include "component_storage.hpp"
#include "rigidbody_component.hpp"
#include "transform_component.hpp"

#include <algorithm>
#include <iostream>

namespace PhysicsSystem {

struct CollisionInfo {
  bool is_colliding;
  float x_overlap;
  float y_overlap;
  vec2 normal;
};

CollisionInfo GetAABBCollision(const AABB &a, const AABB &b, const vec2 &pos_a,
                               const vec2 &pos_b) {
  AABB wa = a.to_world_space(pos_a);
  AABB wb = b.to_world_space(pos_b);

  // From the POV of A
  // >[B][A]<
  float intersect_left = wb.bottom_right.x - wa.top_left.x;
  // >[A][B]<
  float intersect_right = wa.bottom_right.x - wb.top_left.x;
  //  V
  // [B]
  // [A]
  //  A
  float intersect_top = wb.bottom_right.y - wa.top_left.y;
  //  V
  // [A]
  // [B]
  //  A
  float intersect_bottom = wa.bottom_right.y - wb.top_left.y;

  bool is_colliding = (intersect_left > 0.0f && intersect_right > 0.0f &&
                       intersect_top > 0.0f && intersect_bottom > 0.0f);

  float x_overlap = 0.0f;
  float y_overlap = 0.0f;
  vec2 x_normal(0.0f, 0.0f);
  vec2 y_normal(0.0f, 0.0f);
  vec2 normal(0.0f, 0.0f);

  if (is_colliding) {
    if (intersect_left < intersect_right) {
      x_overlap = intersect_left;
      x_normal = vec2(1.0f, 0.0f);
    } else {
      x_overlap = intersect_right;
      x_normal = vec2(-1.0f, 0.0f);
    }
    if (intersect_top < intersect_bottom) {
      y_overlap = intersect_top;
      y_normal = vec2(0.0f, 1.0f);
    } else {
      y_overlap = intersect_bottom;
      y_normal = vec2(0.0f, -1.0f);
    }
    if (x_overlap < y_overlap) {
      normal = -x_normal;
    } else {
      normal = -y_normal;
    }
  }
  return {is_colliding, x_overlap, y_overlap, normal};
}

void ResolveAABBCollision(TransformComponent &t1, TransformComponent &t2,
                          const CollisionInfo &info, bool static_body = false) {
  if (!info.is_colliding)
    return;
  float push =
      (info.x_overlap < info.y_overlap) ? info.x_overlap : info.y_overlap;
  if (static_body) {
    // Only move t1
    if (info.x_overlap < info.y_overlap) {
      t1.position.x += (info.normal.x > 0 ? -push : push);
    } else {
      t1.position.y += (info.normal.y > 0 ? -push : push);
    }
  } else {
    // Move both
    float half_push = push * 0.5f;
    if (info.x_overlap < info.y_overlap) {
      t1.position.x += (info.normal.x > 0 ? -half_push : half_push);
      t2.position.x += (info.normal.x > 0 ? half_push : -half_push);
    } else {
      t1.position.y += (info.normal.y > 0 ? -half_push : half_push);
      t2.position.y += (info.normal.y > 0 ? half_push : -half_push);
    }
  }
}

void ApplyImpulseAndFriction(RigidBodyComponent &rb1, RigidBodyComponent &rb2,
                             const vec2 &normal, float restitution,
                             float friction) {
  vec2 relative_velocity = rb1.velocity - rb2.velocity;
  float velocity_along_normal = relative_velocity.dot(normal);

  if (velocity_along_normal > 0.0f) {
    float e = restitution;
    float j = (1.0f + e) * velocity_along_normal;
    j /= (1.0f / rb1.mass) + (1.0f / rb2.mass);

    vec2 impulse = normal * -j;
    rb1.velocity += impulse / rb1.mass;
    rb2.velocity -= impulse / rb2.mass;

    vec2 velocity_along_surface1 =
        rb1.velocity - normal * rb1.velocity.dot(normal);
    vec2 velocity_along_surface2 =
        rb2.velocity - normal * rb2.velocity.dot(normal);

    if (velocity_along_surface1.length() > 0.0f)
      rb1.velocity -= velocity_along_surface1.normalized() * j * friction;
    if (velocity_along_surface2.length() > 0.0f)
      rb2.velocity -= velocity_along_surface2.normalized() * j * friction;
  }
}

void ApplyImpulseAndFrictionStatic(RigidBodyComponent &rb, const vec2 &normal,
                                   float restitution, float friction) {
  float impulse = rb.velocity.dot(normal.normalized()) * (1.0f + restitution);
  rb.velocity -= normal * impulse;

  vec2 velocity_along_surface = rb.velocity - normal * rb.velocity.dot(normal);
  if (velocity_along_surface.length() > 0.0f)
    rb.velocity -=
        velocity_along_surface.normalized() *
        std::min(impulse * friction, velocity_along_surface.length());
}

void Update(float delta_time) {
  float coefficient_of_restitution = 0.5f;
  float coefficient_of_friction = 0.5f;

  for (auto it1 = rigidbody_components.begin();
       it1 != rigidbody_components.end(); ++it1) {
    auto &entity1 = it1->first;
    auto &rigidbody1 = it1->second;

    auto transform_it1 = transform_components.find(entity1);
    if (transform_it1 == transform_components.end())
      continue;
    TransformComponent &transform1 = transform_it1->second;

    // Integrate velocity and position
    rigidbody1.velocity = rigidbody1.velocity + rigidbody1.gravity * delta_time;
    transform1.position =
        transform1.position + rigidbody1.velocity * delta_time;

    // Collide with other rigid bodies
    for (auto it2 = std::next(it1); it2 != rigidbody_components.end(); ++it2) {
      auto &entity2 = it2->first;
      auto &rigidbody2 = it2->second;

      auto transform_it2 = transform_components.find(entity2);
      if (transform_it2 == transform_components.end())
        continue;
      TransformComponent &transform2 = transform_it2->second;

      CollisionInfo info =
          GetAABBCollision(rigidbody1.collision_aabb, rigidbody2.collision_aabb,
                           transform1.position, transform2.position);
      if (!info.is_colliding)
        continue;

      ApplyImpulseAndFriction(rigidbody1, rigidbody2, info.normal,
                              coefficient_of_restitution,
                              coefficient_of_friction);
      ResolveAABBCollision(transform1, transform2, info, false);
    }

    // Collide with static bodies
    for (auto &[entity2, staticbody2] : staticbody_components) {
      auto transform_it2 = transform_components.find(entity2);
      if (transform_it2 == transform_components.end())
        continue;
      TransformComponent &transform2 = transform_it2->second;

      CollisionInfo info = GetAABBCollision(
          rigidbody1.collision_aabb, staticbody2.collision_aabb,
          transform1.position, transform2.position);
      if (!info.is_colliding)
        continue;

      ResolveAABBCollision(transform1, transform2, info, true);
      ApplyImpulseAndFrictionStatic(rigidbody1, info.normal,
                                    coefficient_of_restitution,
                                    coefficient_of_friction);
    }
  }
}

} // namespace PhysicsSystem
