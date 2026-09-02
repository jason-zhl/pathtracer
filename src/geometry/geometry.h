#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "global.h"

enum class GeometryType {
  Sphere,
  PlanePatch
};

struct intersection {
  vec3 point;
  vec3 normal;
  double t = 0;
  int mat_id = -1;
  int geom_id = -1;
};

struct Geometry {
  GeometryType type = GeometryType::Sphere;
  int mat_id = -1;

  vec3 center;
  double radius = 0.0;

  vec3 corner;
  vec3 u;
  vec3 v;
  vec3 n;
  double n_len_sq = 0.0;

  static Geometry sphere(const vec3& center, double radius, int mat_id);
  static Geometry plane_patch(const vec3& corner, const vec3& u, const vec3& v, int mat_id);

  HOST_DEVICE bool hit(const ray& r, const interval* t_range, intersection& isect) const;
  HOST_DEVICE vec3 normal(const vec3& point) const;
  HOST_DEVICE bool sample_emitter_point(vec3& p, vec3& n, double& pdf_area, RNG& rng) const;
  HOST_DEVICE double surface_area() const;
};

inline Geometry Geometry::sphere(const vec3& center, double radius, int mat_id) {
  Geometry g;
  g.type = GeometryType::Sphere;
  g.mat_id = mat_id;
  g.center = center;
  g.radius = fabs(radius);
  return g;
}

inline Geometry Geometry::plane_patch(const vec3& corner, const vec3& u, const vec3& v,
  int mat_id) {
  Geometry g;
  g.type = GeometryType::PlanePatch;
  g.mat_id = mat_id;
  g.corner = corner;
  g.u = u;
  g.v = v;
  g.n = cross(u, v);
  g.n_len_sq = g.n.length_squared();
  return g;
}

#include "sphere.h"
#include "plane_patch.h"

HOST_DEVICE inline bool Geometry::hit(const ray& r, const interval* t_range, intersection& isect) const {
  switch (type) {
    case GeometryType::Sphere:
      return sphere_hit(*this, r, t_range, isect);
    case GeometryType::PlanePatch:
      return plane_hit(*this, r, t_range, isect);
  }
  return false;
}

HOST_DEVICE inline vec3 Geometry::normal(const vec3& point) const {
  switch (type) {
    case GeometryType::Sphere:
      return sphere_normal(*this, point);
    case GeometryType::PlanePatch:
      return plane_normal(*this, point);
  }
  return vec3();
}

HOST_DEVICE inline bool Geometry::sample_emitter_point(vec3& p, vec3& n, double& pdf_area, RNG& rng) const {
  switch (type) {
    case GeometryType::Sphere:
      return sphere_sample_emitter_point(*this, p, n, pdf_area, rng);
    case GeometryType::PlanePatch:
      return plane_sample_emitter_point(*this, p, n, pdf_area, rng);
  }
  return false;
}

HOST_DEVICE inline double Geometry::surface_area() const {
  switch (type) {
    case GeometryType::Sphere:
      return sphere_surface_area(*this);
    case GeometryType::PlanePatch:
      return plane_surface_area(*this);
  }
  return 0.0;
}

#endif
