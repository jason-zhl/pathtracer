#ifndef WORLD_H
#define WORLD_H

#include "scene.h"
#include <utility>
#include <vector>

class World {
  public:
    World() : env_(Environment::solid(vec3(1.0f, 1.0f, 1.0f))) {}

    int add_material(const Material& mat) {
      materials_.push_back(mat);
      return static_cast<int>(materials_.size()) - 1;
    }

    int add(const Geometry& object) {
      geometries_.push_back(object);
      return static_cast<int>(geometries_.size()) - 1;
    }

    void add_area_light(int geom_id) {
      if (geom_id >= 0 && static_cast<std::size_t>(geom_id) < geometries_.size()) {
        area_lights_.push_back(geom_id);
      }
    }

    void clear() {
      geometries_.clear();
      area_lights_.clear();
      materials_.clear();
      env_ = Environment::solid(vec3(1.0f, 1.0f, 1.0f));
    }

    void set_environment(Environment env) { env_ = std::move(env); }

    Scene view() const {
      Scene scene;
      scene.geometries = geometries_.empty() ? nullptr : geometries_.data();
      scene.n_geometries = static_cast<int>(geometries_.size());
      scene.materials = materials_.empty() ? nullptr : materials_.data();
      scene.n_materials = static_cast<int>(materials_.size());
      scene.area_lights = area_lights_.empty() ? nullptr : area_lights_.data();
      scene.n_area_lights = static_cast<int>(area_lights_.size());
      scene.env = &env_;
      scene.env_colour = env_.colour;
      return scene;
    }

  private:
    std::vector<Geometry> geometries_;
    std::vector<int> area_lights_;
    std::vector<Material> materials_;
    Environment env_;
};

#endif
