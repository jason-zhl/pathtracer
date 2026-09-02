#include "global.h"

#include <fstream>
#include <memory>
#include "camera.h"
#include "environment/ibl.h"
#include "environment/solid.h"
#include "geometry/plane_patch.h"
#include "geometry/sphere.h"
#include "material/material.h"
#include "world.h"

int main() {
    // camera cam(800, 1.0, 2000);
    camera cam(800, 1.0, 10);

    world scene;
    const int ground_mat = scene.add_material(Material::lambertian(color(1.0, 1.0, 1.0)));
    const int sphere_mat_red = scene.add_material(Material::plastic(color(0.85, 0.15, 0.12), 0.12));
    const int sphere_mat_green = scene.add_material(Material::plastic(color(0.314, 0.784, 0.12), 0.12));
    const int light_mat = scene.add_material(Material::diffuse_light(color(50.0, 50.0, 50.0)));

    // scene.set_environment(make_unique<ibl>("assets/studio_small_08_4k.hdr"));
    // scene.set_environment(make_unique<solid>(vec3(0.7, 0.8, 1.0)));
    scene.add(make_shared<plane_patch>(
        vec3(-50, 0, -50),
        vec3(100, 0, 0),
        vec3(0, 0, 100),
        ground_mat));
    scene.add(make_shared<sphere>(vec3(0, 3, 15), 3, sphere_mat_red));
    scene.add(make_shared<sphere>(vec3(-2, 0.5, 8), 0.5, sphere_mat_green));
    const auto light_sphere = make_shared<sphere>(vec3(-20, 40, 0), 6, light_mat);
    scene.add(light_sphere);
    scene.add_area_light(light_sphere);
    // scene.add(make_shared<sphere>(vec3(5, 3, 9), 3, sphere_mat_green));

    std::ofstream ppm_out("image.ppm");
    cam.render(scene, &ppm_out);
    ppm_out.close();

    return 0;
}
