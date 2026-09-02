#ifndef SCENE_H
#define SCENE_H

#include "environment/environment.h"
#include "geometry/geometry.h"
#include "material/material.h"

struct Scene {
  const Geometry* geometries = nullptr;
  int n_geometries = 0;
  const Material* materials = nullptr;
  int n_materials = 0;
  const int* area_lights = nullptr;
  int n_area_lights = 0;
  const Environment* env = nullptr;
  vec3 env_colour;

  HOST_DEVICE bool has_material(int id) const {
    return id >= 0 && id < n_materials;
  }

  HOST_DEVICE const Material& material(int id) const {
    return materials[id];
  }

  HOST_DEVICE bool has_geometry(int id) const {
    return id >= 0 && id < n_geometries;
  }

  HOST_DEVICE const Geometry& geometry(int id) const {
    return geometries[id];
  }

  HOST_DEVICE bool has_area_lights() const { return n_area_lights > 0; }

  HOST_DEVICE bool hit(const ray& r, const interval* t_range, intersection& isect) const;
  HOST_DEVICE vec3 get_env(const vec3& direction) const {
#ifdef __CUDA_ARCH__
    (void)direction;
    return env_colour;
#else
    return env->value(direction);
#endif
  }
  HOST_DEVICE void sample_env(vec3& out_direction, float& out_pdf, RNG& rng) const {
#ifdef __CUDA_ARCH__
    (void)rng;
    out_pdf = 0.0f;
    out_direction = vec3(0, 1, 0);
#else
    env->sample_direction(out_direction, out_pdf, rng);
#endif
  }
  HOST_DEVICE float env_pdf(const vec3& direction) const {
#ifdef __CUDA_ARCH__
    (void)direction;
    return 0.0f;
#else
    return env->pdf(direction);
#endif
  }
  HOST_DEVICE color area_light_nee(const ray& r_in, const intersection& isect,
    const vec3& n_shade, RNG& rng) const;
  HOST_DEVICE float area_light_pdf_nee_at_receiver(const vec3& shading_point,
    const vec3& wo_toward_light, int light_geom_id, const vec3& light_point) const;
};

HOST_DEVICE inline float nee_mis_weight(float pdf_nee, float pdf_mat) {
  const float a = pdf_nee * pdf_nee;
  const float b = pdf_mat * pdf_mat;
  const float d = a + b;
  return d > 0.0f ? a / d : 0.0f;
}

HOST_DEVICE inline bool Scene::hit(const ray& r, const interval* t_range,
  intersection& isect) const {
  if (t_range == nullptr) {
    return false;
  }

  intersection closest;
  float closest_t = t_range->max;
  bool hit_anything = false;

  for (int i = 0; i < n_geometries; ++i) {
    intersection temp;
    if (geometries[i].hit(r, t_range, temp)) {
      if (!hit_anything || temp.t < closest_t) {
        temp.geom_id = i;
        closest = temp;
        closest_t = temp.t;
        hit_anything = true;
      }
    }
  }

  if (hit_anything) {
    isect = closest;
  }
  return hit_anything;
}

HOST_DEVICE inline color Scene::area_light_nee(const ray& r_in, const intersection& isect,
  const vec3& n_shade, RNG& rng) const {
  if (n_area_lights <= 0 || !has_material(isect.mat_id)) {
    return color(0, 0, 0);
  }

  const int idx = static_cast<int>(rng.next() * static_cast<float>(n_area_lights));
  const int pick = idx >= n_area_lights ? n_area_lights - 1 : idx;
  const int geom_id = area_lights[pick];
  if (!has_geometry(geom_id)) {
    return color(0, 0, 0);
  }

  const Geometry& geom = geometry(geom_id);
  if (!has_material(geom.mat_id)) {
    return color(0, 0, 0);
  }

  vec3 pL;
  vec3 nL;
  float pdf_a = 0.0f;
  if (!geom.sample_emitter_point(pL, nL, pdf_a, rng) || pdf_a <= 0.0f) {
    return color(0, 0, 0);
  }

  const vec3 d = pL - isect.point;
  const float dist2 = d.length_squared();
  if (dist2 < 1e-20f) {
    return color(0, 0, 0);
  }
  const float dist = sqrtf(dist2);
  const vec3 wo = d / dist;

  const float cos_sh = dot(n_shade, wo);
  if (cos_sh <= 0.0f) {
    return color(0, 0, 0);
  }

  const float cos_light = dot(nL, -wo);
  if (cos_light <= 0.0f) {
    return color(0, 0, 0);
  }

  const interval shadow_range(1e-3f, dist - 1e-3f);
  if (shadow_range.min >= shadow_range.max) {
    return color(0, 0, 0);
  }

  const ray shadow_ray(isect.point + n_shade * 1e-3f, wo);
  intersection occ;
  if (hit(shadow_ray, &shadow_range, occ)) {
    return color(0, 0, 0);
  }

  intersection light_isect;
  light_isect.point = pL;
  light_isect.normal = nL;
  light_isect.mat_id = geom.mat_id;
  light_isect.geom_id = geom_id;

  const ray toward_light(isect.point, wo);
  const color Le = material(geom.mat_id).emitted(toward_light, light_isect);
  const color f = material(isect.mat_id).eval(r_in, isect, wo);

  const float pdf_nee =
    (pdf_a / static_cast<float>(n_area_lights)) * dist2 / fmaxf(cos_light, 1e-20f);
  const float pdf_mat = material(isect.mat_id).pdf(r_in, isect, wo);
  const float mis_w = nee_mis_weight(pdf_nee, pdf_mat);

  return mis_w * f * Le * (cos_sh / fmaxf(pdf_nee, 1e-30f));
}

HOST_DEVICE inline float Scene::area_light_pdf_nee_at_receiver(const vec3& shading_point,
  const vec3& wo_toward_light, int light_geom_id, const vec3& light_point) const {
  if (n_area_lights <= 0 || !has_geometry(light_geom_id)) {
    return 0.0f;
  }
  bool registered = false;
  for (int i = 0; i < n_area_lights; ++i) {
    if (area_lights[i] == light_geom_id) {
      registered = true;
      break;
    }
  }
  if (!registered) {
    return 0.0f;
  }

  const Geometry& light_geom = geometry(light_geom_id);
  const vec3 wo = unit_vector(wo_toward_light);
  const vec3 nL = unit_vector(light_geom.normal(light_point));
  const float cos_light = dot(nL, -wo);
  if (cos_light <= 1e-20f) {
    return 0.0f;
  }

  const vec3 delta = light_point - shading_point;
  const float dist2 = delta.length_squared();
  if (dist2 < 1e-20f) {
    return 0.0f;
  }

  const float A = light_geom.surface_area();
  if (A <= 0.0f) {
    return 0.0f;
  }
  const float pdf_a = 1.0f / A;
  return (pdf_a / static_cast<float>(n_area_lights)) * dist2 / cos_light;
}

#endif
