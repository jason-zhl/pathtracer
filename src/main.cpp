#include "global.h"

#include <fstream>
#include "camera.h"
#include "environment/environment.h"
#include "geometry/geometry.h"
#include "material/material.h"
#include "world.h"

int main() {
    // camera cam(800, 1.0, 2000);
    camera cam(800, 1.0f, 2000);

    World world;
    const int ground_mat = world.add_material(Material::lambertian(color(1.0f, 1.0f, 1.0f)));
    const int sphere_mat_red = world.add_material(Material::plastic(color(0.85f, 0.15f, 0.12f), 0.12f));
    const int sphere_mat_green = world.add_material(Material::plastic(color(0.314f, 0.784f, 0.12f), 0.12f));
    const int light_mat = world.add_material(Material::diffuse_light(color(50.0f, 50.0f, 50.0f)));

    // world.set_environment(Environment::ibl("assets/studio_small_08_4k.hdr"));
    // world.set_environment(Environment::solid(vec3(0.7, 0.8, 1.0)));
    world.add(Geometry::plane_patch(
        vec3(-50, 0, -50),
        vec3(100, 0, 0),
        vec3(0, 0, 100),
        ground_mat));
    world.add(Geometry::sphere(vec3(0, 3, 15), 3, sphere_mat_red));
    world.add(Geometry::sphere(vec3(-2, 0.5, 8), 0.5, sphere_mat_green));
    const int light_sphere = world.add(Geometry::sphere(vec3(-20, 40, 0), 6, light_mat));
    world.add_area_light(light_sphere);
    // world.add(Geometry::sphere(vec3(5, 3, 9), 3, sphere_mat_green));

    std::ofstream ppm_out("image.ppm");
    cam.render(world, &ppm_out);
    ppm_out.close();

    return 0;
}
