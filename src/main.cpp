#include "global.h"
#include <fstream>
#include "camera.h"
#include "plane_patch.h"
#include "sphere.h"
#include "world.h"

int main() {
    camera cam(800, 16.0 / 9.0);

    world scene;
    scene.add(make_shared<plane_patch>(
        vec3(-20, 0, -20),
        vec3(40, 0, 0),
        vec3(0, 0, 40)));
    scene.add(make_shared<sphere>(vec3(4, 1, 10), 2));
    scene.add(make_shared<sphere>(vec3(-4, 2, 20), 6));

    std::ofstream ppm_out("image.ppm");
    cam.render(scene, &ppm_out);
    ppm_out.close();

    return 0;
}
