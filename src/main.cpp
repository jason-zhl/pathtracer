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
    camera cam(400, 1.0, 500);
    const auto ground_mat = make_shared<lambertian>(color(1.0, 1.0, 1.0));
    const auto sphere_mat_red = make_shared<plastic>(color(0.85, 0.15, 0.12), 0.05);
    const auto sphere_mat_green = make_shared<plastic>(color(0.314, 0.784, 0.12), 0.05);

    world scene;
    scene.add(make_shared<plane_patch>(
        vec3(-50, 0, -50),
        vec3(100, 0, 0),
        vec3(0, 0, 100),
        ground_mat));
    scene.add(make_shared<sphere>(vec3(0, 3, 6), 3, sphere_mat_red));
    // scene.add(make_shared<sphere>(vec3(5, 3, 9), 3, sphere_mat_green));

    std::ofstream ppm_out("image.ppm");
    cam.render(scene, &ppm_out);
    ppm_out.close();

    return 0;
}
