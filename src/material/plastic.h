#ifndef PLASTIC_H
#define PLASTIC_H

namespace {

constexpr double k_plastic_surface_offset = 1e-3;
constexpr double k_plastic_dielectric_f0 = 0.04;
constexpr double k_plastic_eps = 1e-8;
constexpr int k_plastic_max_spec_tries = 32;
constexpr double k_plastic_n_diff = 0.3;
constexpr double k_plastic_n_spec = 0.7;
constexpr double k_plastic_n_sum = k_plastic_n_diff + k_plastic_n_spec;

HOST_DEVICE inline double plastic_pdf_diffuse(double ndotwo) { return ndotwo / PI; }

HOST_DEVICE inline double plastic_ggx(const Material& mat, const vec3& h, const vec3& n) {
  const double c = dot(h, n);
  const double c2 = c * c;
  const double d = (c2 * (mat.a2 - 1.0) + 1.0);
  return mat.a2 / (PI * d * d);
}

HOST_DEVICE inline double plastic_fresnel(double cos_theta) {
  cos_theta = clamp(cos_theta, -1.0, 1.0);
  const double t = 1.0 - fabs(cos_theta);
  return k_plastic_dielectric_f0
    + (1.0 - k_plastic_dielectric_f0) * (t * t * t * t * t);
}

HOST_DEVICE inline double plastic_smith_g1(const Material& mat, const vec3& v, const vec3& n) {
  const double ndotv = dot(n, v);
  if (ndotv <= 0.0) {
    return 0.0;
  }
  return 2.0 * ndotv / (ndotv + sqrt(mat.a2 + (1.0 - mat.a2) * ndotv * ndotv));
}

HOST_DEVICE inline vec3 plastic_reflect(const vec3& wi, const vec3& m) {
  return 2.0 * dot(wi, m) * m - wi;
}

HOST_DEVICE inline vec3 plastic_sample_ggx(const Material& mat, const vec3& n, RNG& rng) {
  const double xi1 = rng.next();
  const double xi2 = rng.next();
  const double cos_theta =
    sqrt(fmax(0.0, (1.0 - xi2) / (1.0 + (mat.a2 - 1.0) * xi2)));
  const double sin_theta = sqrt(fmax(0.0, 1.0 - cos_theta * cos_theta));
  const double phi = 2.0 * PI * xi1;
  vec3 t, b;
  orthonormal_basis(n, t, b);
  const double cos_p = cos(phi);
  const double sin_p = sin(phi);
  return unit_vector(t * (sin_theta * cos_p) + b * (sin_theta * sin_p) + n * cos_theta);
}

HOST_DEVICE inline double plastic_pdf_specular(const Material& mat, const vec3& wi, const vec3& wo,
  const vec3& n) {
  const double ndotwo = dot(n, wo);
  if (ndotwo <= 0.0) {
    return 0.0;
  }
  const vec3 h = unit_vector(wi + wo);
  const double ndoth = dot(n, h);
  const double wih = dot(wi, h);
  if (ndoth <= 0.0 || wih <= 0.0) {
    return 0.0;
  }
  const double D = plastic_ggx(mat, h, n);
  return (D * ndoth) / (4.0 * fabs(wih));
}

HOST_DEVICE inline color plastic_eval_brdf(const Material& mat, const vec3& wi, const vec3& wo,
  const vec3& n, double cos_i) {
  const color f_diff = mat.albedo / PI;
  const double ndotwo = dot(n, wo);
  double f_spec = 0.0;
  if (ndotwo > 0.0 && cos_i > 0.0) {
    const vec3 h = unit_vector(wi + wo);
    const double ndoth = fmax(0.0, dot(n, h));
    const double wih = fmax(0.0, dot(wi, h));
    if (ndoth > 0.0 && wih > 0.0) {
      const double D = plastic_ggx(mat, h, n);
      const double F_micro = plastic_fresnel(wih);
      const double G = plastic_smith_g1(mat, wi, n) * plastic_smith_g1(mat, wo, n);
      f_spec = (D * G * F_micro) / (4.0 * cos_i * ndotwo);
    }
  }
  return f_diff + color(f_spec, f_spec, f_spec);
}

}  // namespace

HOST_DEVICE inline bool plastic_scatter(const Material& mat, const ray& r_in, const intersection& rec,
  color& attenuation, ray& scattered, RNG& rng) {
  vec3 n = unit_vector(rec.normal);
  if (dot(r_in.direction(), n) > 0.0) {
    n = -n;
  }

  const vec3 wi = -unit_vector(r_in.direction());
  const double cos_i = fmax(0.0, dot(n, wi));

  if (rng.next() * k_plastic_n_sum < k_plastic_n_spec) {
    for (int attempt = 0; attempt < k_plastic_max_spec_tries; ++attempt) {
      const vec3 h = plastic_sample_ggx(mat, n, rng);
      const double ndoth = dot(n, h);
      const double wih = dot(wi, h);
      if (ndoth <= k_plastic_eps || wih <= k_plastic_eps) {
        continue;
      }

      const vec3 wo = unit_vector(plastic_reflect(wi, h));
      const double ndotwo = dot(n, wo);
      if (ndotwo <= k_plastic_eps) {
        continue;
      }

      const color f = plastic_eval_brdf(mat, wi, wo, n, cos_i);
      const double pdf_d = plastic_pdf_diffuse(ndotwo);
      const double pdf_s = plastic_pdf_specular(mat, wi, wo, n);
      const double mis_pdf = (k_plastic_n_diff * pdf_d + k_plastic_n_spec * pdf_s) / k_plastic_n_sum;
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
  const double ndotwo = dot(n, wo);
  if (ndotwo <= k_plastic_eps) {
    return false;
  }

  const color f = plastic_eval_brdf(mat, wi, wo, n, cos_i);
  const double pdf_d = plastic_pdf_diffuse(ndotwo);
  const double pdf_s = plastic_pdf_specular(mat, wi, wo, n);
  const double mis_pdf = (k_plastic_n_diff * pdf_d + k_plastic_n_spec * pdf_s) / k_plastic_n_sum;
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
  if (dot(r_in.direction(), n) > 0.0) {
    n = -n;
  }
  const vec3 wi = -unit_vector(r_in.direction());
  const double cos_i = fmax(0.0, dot(n, wi));
  const vec3 wou = unit_vector(wo);
  return plastic_eval_brdf(mat, wi, wou, n, cos_i);
}

HOST_DEVICE inline double plastic_pdf(const Material& mat, const ray& r_in, const intersection& rec,
  const vec3& wo) {
  vec3 n = unit_vector(rec.normal);
  if (dot(r_in.direction(), n) > 0.0) {
    n = -n;
  }
  const vec3 wi = -unit_vector(r_in.direction());
  const vec3 wou = unit_vector(wo);
  const double ndotwo = dot(n, wou);
  if (ndotwo <= k_plastic_eps) {
    return 0.0;
  }
  const double pdf_d = plastic_pdf_diffuse(ndotwo);
  const double pdf_s = plastic_pdf_specular(mat, wi, wou, n);
  return (k_plastic_n_diff * pdf_d + k_plastic_n_spec * pdf_s) / k_plastic_n_sum;
}

#endif
