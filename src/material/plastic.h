#ifndef PLASTIC_H
#define PLASTIC_H

namespace {

constexpr float k_plastic_surface_offset = 1e-3f;
constexpr float k_plastic_dielectric_f0 = 0.04f;
constexpr float k_plastic_eps = 1e-8f;
constexpr int k_plastic_max_spec_tries = 32;
constexpr float k_plastic_n_diff = 0.3f;
constexpr float k_plastic_n_spec = 0.7f;
constexpr float k_plastic_n_sum = k_plastic_n_diff + k_plastic_n_spec;

HOST_DEVICE inline float plastic_pdf_diffuse(float ndotwo) { return ndotwo / PI; }

HOST_DEVICE inline float plastic_ggx(const Material& mat, const vec3& h, const vec3& n) {
  const float c = dot(h, n);
  const float c2 = c * c;
  const float d = (c2 * (mat.a2 - 1.0f) + 1.0f);
  return mat.a2 / (PI * d * d);
}

HOST_DEVICE inline float plastic_fresnel(float cos_theta) {
  cos_theta = clamp(cos_theta, -1.0f, 1.0f);
  const float t = 1.0f - fabsf(cos_theta);
  return k_plastic_dielectric_f0
    + (1.0f - k_plastic_dielectric_f0) * (t * t * t * t * t);
}

HOST_DEVICE inline float plastic_smith_g1(const Material& mat, const vec3& v, const vec3& n) {
  const float ndotv = dot(n, v);
  if (ndotv <= 0.0f) {
    return 0.0f;
  }
  return 2.0f * ndotv / (ndotv + sqrtf(mat.a2 + (1.0f - mat.a2) * ndotv * ndotv));
}

HOST_DEVICE inline vec3 plastic_reflect(const vec3& wi, const vec3& m) {
  return 2.0f * dot(wi, m) * m - wi;
}

HOST_DEVICE inline vec3 plastic_sample_ggx(const Material& mat, const vec3& n, RNG& rng) {
  const float xi1 = rng.next();
  const float xi2 = rng.next();
  const float cos_theta =
    sqrtf(fmaxf(0.0f, (1.0f - xi2) / (1.0f + (mat.a2 - 1.0f) * xi2)));
  const float sin_theta = sqrtf(fmaxf(0.0f, 1.0f - cos_theta * cos_theta));
  const float phi = 2.0f * PI * xi1;
  vec3 t, b;
  orthonormal_basis(n, t, b);
  const float cos_p = cosf(phi);
  const float sin_p = sinf(phi);
  return unit_vector(t * (sin_theta * cos_p) + b * (sin_theta * sin_p) + n * cos_theta);
}

HOST_DEVICE inline float plastic_pdf_specular(const Material& mat, const vec3& wi, const vec3& wo,
  const vec3& n) {
  const float ndotwo = dot(n, wo);
  if (ndotwo <= 0.0f) {
    return 0.0f;
  }
  const vec3 h = unit_vector(wi + wo);
  const float ndoth = dot(n, h);
  const float wih = dot(wi, h);
  if (ndoth <= 0.0f || wih <= 0.0f) {
    return 0.0f;
  }
  const float D = plastic_ggx(mat, h, n);
  return (D * ndoth) / (4.0f * fabsf(wih));
}

HOST_DEVICE inline color plastic_eval_brdf(const Material& mat, const vec3& wi, const vec3& wo,
  const vec3& n, float cos_i) {
  const color f_diff = mat.albedo / PI;
  const float ndotwo = dot(n, wo);
  float f_spec = 0.0f;
  if (ndotwo > 0.0f && cos_i > 0.0f) {
    const vec3 h = unit_vector(wi + wo);
    const float ndoth = fmaxf(0.0f, dot(n, h));
    const float wih = fmaxf(0.0f, dot(wi, h));
    if (ndoth > 0.0f && wih > 0.0f) {
      const float D = plastic_ggx(mat, h, n);
      const float F_micro = plastic_fresnel(wih);
      const float G = plastic_smith_g1(mat, wi, n) * plastic_smith_g1(mat, wo, n);
      f_spec = (D * G * F_micro) / (4.0f * cos_i * ndotwo);
    }
  }
  return f_diff + color(f_spec, f_spec, f_spec);
}

}  // namespace

HOST_DEVICE inline bool plastic_scatter(const Material& mat, const ray& r_in, const intersection& rec,
  color& attenuation, ray& scattered, RNG& rng) {
  vec3 n = unit_vector(rec.normal);
  if (dot(r_in.direction(), n) > 0.0f) {
    n = -n;
  }

  const vec3 wi = -unit_vector(r_in.direction());
  const float cos_i = fmaxf(0.0f, dot(n, wi));

  if (rng.next() * k_plastic_n_sum < k_plastic_n_spec) {
    for (int attempt = 0; attempt < k_plastic_max_spec_tries; ++attempt) {
      const vec3 h = plastic_sample_ggx(mat, n, rng);
      const float ndoth = dot(n, h);
      const float wih = dot(wi, h);
      if (ndoth <= k_plastic_eps || wih <= k_plastic_eps) {
        continue;
      }

      const vec3 wo = unit_vector(plastic_reflect(wi, h));
      const float ndotwo = dot(n, wo);
      if (ndotwo <= k_plastic_eps) {
        continue;
      }

      const color f = plastic_eval_brdf(mat, wi, wo, n, cos_i);
      const float pdf_d = plastic_pdf_diffuse(ndotwo);
      const float pdf_s = plastic_pdf_specular(mat, wi, wo, n);
      const float mis_pdf = (k_plastic_n_diff * pdf_d + k_plastic_n_spec * pdf_s) / k_plastic_n_sum;
      if (mis_pdf <= k_plastic_eps) {
        continue;
      }

      attenuation = f * ndotwo / mis_pdf;
      scattered = ray(rec.point + n * k_plastic_surface_offset, wo);
      return true;
    }
    return false;
  }

  const vec3 wo = unit_vector(lambertian_random(n, rng));
  const float ndotwo = dot(n, wo);
  if (ndotwo <= k_plastic_eps) {
    return false;
  }

  const color f = plastic_eval_brdf(mat, wi, wo, n, cos_i);
  const float pdf_d = plastic_pdf_diffuse(ndotwo);
  const float pdf_s = plastic_pdf_specular(mat, wi, wo, n);
  const float mis_pdf = (k_plastic_n_diff * pdf_d + k_plastic_n_spec * pdf_s) / k_plastic_n_sum;
  if (mis_pdf <= k_plastic_eps) {
    return false;
  }

  attenuation = f * ndotwo / mis_pdf;
  scattered = ray(rec.point + n * k_plastic_surface_offset, wo);
  return true;
}

HOST_DEVICE inline color plastic_eval(const Material& mat, const ray& r_in, const intersection& rec,
  const vec3& wo) {
  vec3 n = unit_vector(rec.normal);
  if (dot(r_in.direction(), n) > 0.0f) {
    n = -n;
  }
  const vec3 wi = -unit_vector(r_in.direction());
  const float cos_i = fmaxf(0.0f, dot(n, wi));
  const vec3 wou = unit_vector(wo);
  return plastic_eval_brdf(mat, wi, wou, n, cos_i);
}

HOST_DEVICE inline float plastic_pdf(const Material& mat, const ray& r_in, const intersection& rec,
  const vec3& wo) {
  vec3 n = unit_vector(rec.normal);
  if (dot(r_in.direction(), n) > 0.0f) {
    n = -n;
  }
  const vec3 wi = -unit_vector(r_in.direction());
  const vec3 wou = unit_vector(wo);
  const float ndotwo = dot(n, wou);
  if (ndotwo <= k_plastic_eps) {
    return 0.0f;
  }
  const float pdf_d = plastic_pdf_diffuse(ndotwo);
  const float pdf_s = plastic_pdf_specular(mat, wi, wou, n);
  return (k_plastic_n_diff * pdf_d + k_plastic_n_spec * pdf_s) / k_plastic_n_sum;
}

#endif
