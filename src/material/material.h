#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "geometry/geometry.h"
#include "ray.h"

enum class MaterialType {
  Lambertian,
  Plastic,
  DiffuseLight
};

struct Material {
  MaterialType type = MaterialType::Lambertian;
  vec3 albedo;
  double roughness = 0.0;
  double alpha = 0.0;
  double a2 = 0.0;
  vec3 emission;

  static Material lambertian(const vec3& albedo);
  static Material plastic(const vec3& albedo, double roughness);
  static Material diffuse_light(const vec3& emit);

  HOST_DEVICE bool is_emissive() const { return type == MaterialType::DiffuseLight; }

  HOST_DEVICE bool scatter(const ray& r_in, const intersection& rec, color& attenuation,
    ray& scattered, RNG& rng) const;
  HOST_DEVICE color eval(const ray& r_in, const intersection& rec, const vec3& wo) const;
  HOST_DEVICE double pdf(const ray& r_in, const intersection& rec, const vec3& wo) const;
  HOST_DEVICE color emitted(const ray& r_in, const intersection& rec) const;
};

inline Material Material::lambertian(const vec3& albedo) {
  Material m;
  m.type = MaterialType::Lambertian;
  m.albedo = albedo;
  return m;
}

inline Material Material::plastic(const vec3& albedo, double roughness) {
  Material m;
  m.type = MaterialType::Plastic;
  m.albedo = albedo;
  m.roughness = roughness;
  m.alpha = roughness * roughness;
  m.a2 = m.alpha * m.alpha;
  return m;
}

inline Material Material::diffuse_light(const vec3& emit) {
  Material m;
  m.type = MaterialType::DiffuseLight;
  m.emission = emit;
  return m;
}

#include "lambertian.h"
#include "plastic.h"
#include "diffuse_light.h"

HOST_DEVICE inline bool Material::scatter(const ray& r_in, const intersection& rec, color& attenuation,
  ray& scattered, RNG& rng) const {
  switch (type) {
    case MaterialType::Lambertian:
      return lambertian_scatter(*this, r_in, rec, attenuation, scattered, rng);
    case MaterialType::Plastic:
      return plastic_scatter(*this, r_in, rec, attenuation, scattered, rng);
    case MaterialType::DiffuseLight:
      return diffuse_light_scatter(*this, r_in, rec, attenuation, scattered, rng);
  }
  return false;
}

HOST_DEVICE inline color Material::eval(const ray& r_in, const intersection& rec, const vec3& wo) const {
  switch (type) {
    case MaterialType::Lambertian:
      return lambertian_eval(*this, r_in, rec, wo);
    case MaterialType::Plastic:
      return plastic_eval(*this, r_in, rec, wo);
    case MaterialType::DiffuseLight:
      return color(0, 0, 0);
  }
  return color(0, 0, 0);
}

HOST_DEVICE inline double Material::pdf(const ray& r_in, const intersection& rec, const vec3& wo) const {
  switch (type) {
    case MaterialType::Lambertian:
      return lambertian_pdf(*this, r_in, rec, wo);
    case MaterialType::Plastic:
      return plastic_pdf(*this, r_in, rec, wo);
    case MaterialType::DiffuseLight:
      return 0.0;
  }
  return 0.0;
}

HOST_DEVICE inline color Material::emitted(const ray& r_in, const intersection& rec) const {
  if (type != MaterialType::DiffuseLight) {
    return color(0, 0, 0);
  }
  return diffuse_light_emitted(*this, r_in, rec);
}

#endif
