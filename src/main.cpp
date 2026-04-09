#include "global.h"
#include <fstream>
#include "camera.h"
#include "material/lambertian.h"
#include "plane_patch.h"
#include "sphere.h"
#include "world.h"

int main() {
    camera cam(800, 16.0 / 9.0, 500);

    const auto ground_mat = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    const auto sphere_mat = make_shared<lambertian>(color(0.5, 0.5, 0.5));

    world scene;
    scene.add(make_shared<plane_patch>(
        vec3(-50, 0, -50),
        vec3(100, 0, 0),
        vec3(0, 0, 100),
        ground_mat));
    scene.add(make_shared<sphere>(vec3(0, 3, 6), 3, sphere_mat));

    std::ofstream ppm_out("image.ppm");
    cam.render(scene, &ppm_out);
    ppm_out.close();

    return 0;
}
