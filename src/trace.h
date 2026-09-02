#ifndef TRACE_H
#define TRACE_H

#include "world.h"

#ifndef CAMERA_H
#include "camera.h"
#endif

/** Power heuristic MIS weight for strategy with pdf_self vs the other; β = 2 (Veach). */
HOST_DEVICE inline double mis_weight_power(double pdf_self, double pdf_other) {
  const double a = pdf_self * pdf_self;
  const double b = pdf_other * pdf_other;
  const double denom = a + b;
  return denom > 0.0 ? a / denom : 0.0;
}

HOST_DEVICE inline color ray_colour(const ray& r, const camera& cam, const world& scene, RNG& rng) {
  color L(0, 0, 0);
  color throughput(1, 1, 1);
  ray curr_ray = r;

  // Owned storage so prev_* stay valid across iterations (unlike &isect on the stack).
  intersection prev_isect_storage;
  ray prev_ray_storage;
  const intersection* prev_isect = nullptr;
  const ray* prev_ray = nullptr;

  for (int i = 0; i < cam.max_depth(); i++) {
    intersection isect;
    if (!scene.hit(curr_ray, &cam.ray_t(), isect)) {
      // Primary miss: plain env. After a BSDF bounce: MIS-weighted env (old post-scatter miss).
      if (prev_isect != nullptr && scene.has_material(prev_isect->mat_id) && prev_ray != nullptr) {
        const vec3 Le = scene.get_env(curr_ray.direction());
        const double pdf_env = scene.env_pdf(curr_ray.direction());
        const double pdf_mat =
          scene.material(prev_isect->mat_id).pdf(*prev_ray, *prev_isect, curr_ray.direction());
        const double mis_w = mis_weight_power(pdf_mat, pdf_env);
        L += throughput * mis_w * Le;
      } else {
        L += throughput * scene.get_env(curr_ray.direction());
      }
      break;
    }

    color current(0, 0, 0);

    vec3 n = unit_vector(isect.normal);
    if (dot(curr_ray.direction(), n) > 0.0) {
      n = -n;
    }

    if (scene.has_material(isect.mat_id) && scene.material(isect.mat_id).is_emissive()) {
      color Le = scene.material(isect.mat_id).emitted(curr_ray, isect);
      if (prev_isect != nullptr && prev_ray != nullptr && scene.has_area_lights()) {
        const vec3 wo = unit_vector(curr_ray.direction());
        const double pdf_nee =
          scene.area_light_pdf_nee_at_receiver(prev_isect->point, wo, isect.geom_id, isect.point);
        if (pdf_nee > 0.0 && scene.has_material(prev_isect->mat_id)) {
          const double pdf_mat = scene.material(prev_isect->mat_id).pdf(*prev_ray, *prev_isect, wo);
          const double w_bsdf = nee_mis_weight(pdf_mat, pdf_nee);
          Le *= w_bsdf;
        }
      }
      current += Le;
    }

    if (scene.has_material(isect.mat_id)) {
      vec3 wo_env;
      double pdf_env = 0.0;
      scene.sample_env(wo_env, pdf_env, rng);
      if (pdf_env > 0.0 && dot(n, wo_env) > 0.0) {
        ray env_ray(isect.point + n * 1e-3, wo_env);
        intersection shadow_isect;
        if (!scene.hit(env_ray, &cam.ray_t(), shadow_isect)) {
          const Material& mat = scene.material(isect.mat_id);
          const color f_env = mat.eval(curr_ray, isect, wo_env);
          const double pdf_mat = mat.pdf(curr_ray, isect, wo_env);
          const double mis_w = mis_weight_power(pdf_env, pdf_mat);
          const vec3 Le = scene.get_env(wo_env);
          current += mis_w * f_env * Le * dot(n, wo_env) / pdf_env;
        }
      }
    }

    if (scene.has_area_lights() && scene.has_material(isect.mat_id)
      && !scene.material(isect.mat_id).is_emissive()) {
      current += scene.area_light_nee(curr_ray, isect, n, rng);
    }

    L += throughput * current;

    ray scattered;
    color attenuation;
    if (!scene.has_material(isect.mat_id)
      || !scene.material(isect.mat_id).scatter(curr_ray, isect, attenuation, scattered, rng)) {
      break;
    }

    prev_isect_storage = isect;
    prev_ray_storage = curr_ray;
    prev_isect = &prev_isect_storage;
    prev_ray = &prev_ray_storage;
    curr_ray = scattered;
    throughput *= attenuation;
  }

  return L;
}

#endif
