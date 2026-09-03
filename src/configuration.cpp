#include "camera.h"
#include "world.h"

#include <stdexcept>
#include <string>
#include <unordered_map>

#include <yaml-cpp/yaml.h>

namespace {

YAML::Node load_root(const std::string& path) {
  try {
    return YAML::LoadFile(path);
  } catch (const YAML::Exception& e) {
    throw std::runtime_error("Failed to load '" + path + "': " + e.what());
  }
}

YAML::Node require(const YAML::Node& parent, const char* key, const std::string& context) {
  const YAML::Node node = parent[key];
  if (!node) {
    throw std::runtime_error(context + "." + key + " is required");
  }
  return node;
}

vec3 read_vec3(const YAML::Node& node, const std::string& field) {
  if (!node || !node.IsSequence() || node.size() != 3) {
    throw std::runtime_error(field + " must be [x, y, z]");
  }
  return vec3(node[0].as<float>(), node[1].as<float>(), node[2].as<float>());
}

camera camera_from_yaml(const YAML::Node& root, const std::string& path) {
  const YAML::Node cam = root["camera"];
  if (!cam || !cam.IsMap()) {
    throw std::runtime_error("'" + path + "': missing 'camera' map");
  }

  try {
    const std::string ctx = "camera";
    const vec3 up = cam["up"] ? read_vec3(cam["up"], "camera.up") : vec3(0.0f, 1.0f, 0.0f);
    const float focus_dist = cam["focus_dist"] ? cam["focus_dist"].as<float>() : 5.5f;
    const float aperture = cam["aperture"] ? cam["aperture"].as<float>() : 0.0f;
    const float exposure = cam["exposure"] ? cam["exposure"].as<float>() : 1.0f;

    return camera(
      require(cam, "width", ctx).as<int>(),
      require(cam, "aspect", ctx).as<float>(),
      require(cam, "samples", ctx).as<int>(),
      read_vec3(require(cam, "look_from", ctx), "camera.look_from"),
      read_vec3(require(cam, "look_at", ctx), "camera.look_at"),
      up,
      require(cam, "vfov", ctx).as<float>(),
      focus_dist,
      aperture,
      exposure);
  } catch (const YAML::Exception& e) {
    throw std::runtime_error("'" + path + "': " + e.what());
  }
}

Environment environment_from_yaml(const YAML::Node& node, const std::string& path) {
  const std::string ctx = "environment";
  const std::string type = require(node, "type", ctx).as<std::string>();
  if (type == "solid") {
    return Environment::solid(read_vec3(require(node, "colour", ctx), "environment.colour"));
  }
  if (type == "ibl") {
    return Environment::ibl(require(node, "file", ctx).as<std::string>());
  }
  throw std::runtime_error("'" + path + "': unknown environment type '" + type + "'");
}

Material material_from_yaml(const YAML::Node& node, const std::string& name) {
  const std::string ctx = "materials." + name;
  const std::string type = require(node, "type", ctx).as<std::string>();
  if (type == "lambertian") {
    return Material::lambertian(read_vec3(require(node, "albedo", ctx), ctx + ".albedo"));
  }
  if (type == "plastic") {
    return Material::plastic(
      read_vec3(require(node, "albedo", ctx), ctx + ".albedo"),
      require(node, "roughness", ctx).as<float>());
  }
  if (type == "diffuse_light") {
    return Material::diffuse_light(
      read_vec3(require(node, "emission", ctx), ctx + ".emission"));
  }
  throw std::runtime_error(ctx + ": unknown material type '" + type + "'");
}

int resolve_material(const YAML::Node& node, const std::string& ctx,
  const std::unordered_map<std::string, int>& materials) {
  const std::string name = require(node, "material", ctx).as<std::string>();
  const auto it = materials.find(name);
  if (it == materials.end()) {
    throw std::runtime_error(ctx + ": unknown material '" + name + "'");
  }
  return it->second;
}

void add_object_from_yaml(World& world, const YAML::Node& node, const std::string& ctx,
  const std::unordered_map<std::string, int>& materials) {
  const std::string type = require(node, "type", ctx).as<std::string>();
  const int mat_id = resolve_material(node, ctx, materials);

  int geom_id = -1;
  if (type == "sphere") {
    geom_id = world.add(Geometry::sphere(
      read_vec3(require(node, "center", ctx), ctx + ".center"),
      require(node, "radius", ctx).as<float>(),
      mat_id));
  } else if (type == "plane_patch") {
    geom_id = world.add(Geometry::plane_patch(
      read_vec3(require(node, "corner", ctx), ctx + ".corner"),
      read_vec3(require(node, "u", ctx), ctx + ".u"),
      read_vec3(require(node, "v", ctx), ctx + ".v"),
      mat_id));
  } else {
    throw std::runtime_error(ctx + ": unknown object type '" + type + "'");
  }

  if (node["area_light"] && node["area_light"].as<bool>()) {
    world.add_area_light(geom_id);
  }
}

void fill_world_from_yaml(World& world, const YAML::Node& root, const std::string& path) {
  if (const YAML::Node env = root["environment"]) {
    world.set_environment(environment_from_yaml(env, path));
  }

  std::unordered_map<std::string, int> material_ids;
  if (const YAML::Node materials = root["materials"]) {
    if (!materials.IsMap()) {
      throw std::runtime_error("'" + path + "': materials must be a map of name -> material");
    }
    for (const auto& it : materials) {
      const std::string name = it.first.as<std::string>();
      material_ids[name] = world.add_material(material_from_yaml(it.second, name));
    }
  }

  if (const YAML::Node objects = root["objects"]) {
    if (!objects.IsSequence()) {
      throw std::runtime_error("'" + path + "': objects must be a list");
    }
    for (std::size_t i = 0; i < objects.size(); ++i) {
      add_object_from_yaml(
        world, objects[i], "objects[" + std::to_string(i) + "]", material_ids);
    }
  }
}

}  // namespace

camera::camera(const std::string& config_path)
  : camera(camera_from_yaml(load_root(config_path), config_path)) {}

World::World(const std::string& config_path) : World() {
  try {
    fill_world_from_yaml(*this, load_root(config_path), config_path);
  } catch (const YAML::Exception& e) {
    throw std::runtime_error("'" + config_path + "': " + e.what());
  }
}
