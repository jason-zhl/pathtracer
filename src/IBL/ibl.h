#ifndef IBL_H
#define IBL_H

#include "../global.h"

#include <string>
#include <vector>

class ibl {
  public:
    ibl(const std::string& file_name);
    ibl(const ibl&) = delete;
    ibl& operator=(const ibl&) = delete;
    ibl(ibl&& other) noexcept;
    ibl& operator=(ibl&& other) noexcept;
    ~ibl();

    vec3 value(const vec3& direction) const;

    /** Sample direction proportional to luminance × sin(θ) on the equirect grid; pdf w.r.t. solid angle. */
    void sample_direction(vec3& out_direction, double& out_pdf_solid_angle) const;

    /** Pdf for discrete env sampling at the texel covering `direction` (consistent with sample_direction). */
    double pdf(const vec3& direction) const;

  private:
    float* texture_;
    int width_;
    int height_;
    int channels_;

    std::vector<double> marginal_cdf_;
    std::vector<double> cond_cdf_;
    std::vector<double> pixel_weights_;
    double total_weight_ = 0.0;

    void build_sampling_distribution();

    static double luminance_rgb(double r, double g, double b);

    vec3 sample_equirectangular(double u, double v) const;
    vec3 bilinear_interpolation(double u, double v) const;
    void direction_from_uv(double u, double v, vec3& out) const;
    void uv_from_direction(const vec3& d, double& u, double& v) const;
};

#endif
