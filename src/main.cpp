#include "global.h"

#include <exception>
#include <fstream>
#include <iostream>

#include "camera.h"
#include "world.h"

int main(int argc, char** argv) {
    try {
        const char* config = argc > 1 ? argv[1] : "scenes/default.yaml";
        camera cam(config);
        World world(config);

        std::ofstream ppm_out("image.ppm");
        cam.render(world, &ppm_out);
        ppm_out.close();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
