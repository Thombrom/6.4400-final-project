#pragma once

#include "gloo/Application.hpp"

namespace GLOO {
class MeshViewerApp : public Application {
 public:
  MeshViewerApp(const std::string& app_name,
                glm::ivec2 window_size);
  void SetupScene() override;
};
}  // namespace GLOO