#include "global.h"

#include <fstream>
#include <memory>
#include "camera.h"
#include "material/lambertian.h"
#include "material/plastic.h"
#include "plane_patch.h"
#include "sphere.h"
#include "world.h"

int main() {
    camera cam(800, 16.0 / 9.0, 500);
    const auto ground_mat = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    const auto sphere_mat = make_shared<plastic>(color(0.85, 0.15, 0.12), 0.12);

    world scene;
    scene.set_ibl(make_unique<ibl>("assets/studio_small_08_4k.hdr"));
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
