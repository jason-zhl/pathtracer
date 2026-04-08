#include <fstream>
#include <iostream>
#include <memory>
#include "color.h"
#include "plane_patch.h"
#include "ray.h"
#include "sphere.h"
#include "vec3.h"
#include "world.h"

color ray_colour(const ray& r) {
    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
}

int main() {
    int width = 800;
    auto aspect_ratio = 16.0 / 9.0;
    int height = static_cast<int>(width / aspect_ratio);

    auto viewport_height = 9.0;
    auto viewport_width = viewport_height * (double(width) / height);
    auto focal_length = 4.5;

    auto camera_center = vec3(0, 2, 0);

    auto viewport_horizontal = vec3(viewport_width, 0, 0);
    auto viewport_vertical = vec3(0, viewport_height, 0);

    auto pixel_delta_horizontal = viewport_horizontal / width;
    auto pixel_delta_vertical = viewport_vertical / height;

    auto viewport_upper_left = camera_center + vec3(0, 0, focal_length) - viewport_horizontal / 2 + viewport_vertical / 2;
    auto pixel00 = viewport_upper_left + 0.5 * (pixel_delta_horizontal + pixel_delta_vertical);

    world scene;
    scene.add(std::make_shared<plane_patch>(
        vec3(-20, 0, -20),
        vec3(40, 0, 0),
        vec3(0, 0, 40)));
    scene.add(std::make_shared<sphere>(vec3(4, 1, 10), 2));
    scene.add(std::make_shared<sphere>(vec3(-4, 2, 20), 6));

    std::ofstream ppm_out("image.ppm");
    ppm_out << "P3\n" << width << " " << height << "\n255\n";

    for (auto j{0}; j < height; j++) {
        for (auto i{0}; i < width; i++) {
            auto pixel_center = pixel00 + (i * pixel_delta_horizontal) - (j * pixel_delta_vertical);
            auto ray_direction = pixel_center - camera_center;
            ray r(camera_center, ray_direction);

            color pixel_color(0, 0, 0);
            intersection isect;
            if (scene.hit(r, isect)) {
                vec3 n = unit_vector(isect.surface->normal(isect.point));
                pixel_color = 0.5 * color(n.x() + 1, n.y() + 1, n.z() + 1);
            } else {
                pixel_color = ray_colour(r);
            }
            write_color(ppm_out, pixel_color);
        }
    }
    ppm_out.close();

    return 0;
}
