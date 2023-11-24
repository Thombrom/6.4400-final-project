#pragma once

#include "gloo/Application.hpp"

#include <unordered_set>

namespace GLOO {

class MeshSimplifier {
  std::unique_ptr<VertexObject> vo;
  std::vector<std::pair<glm::vec3, glm::mat4>> vertices;
  std::unordered_map<size_t, size_t> vertex_mapping;        // Allows us to reconstruct the normals in the end
  std::unordered_map<size_t, std::unordered_set<size_t>> edges; // Adjacency list
  
  void remove_edge(size_t first, size_t second);
  void insert_edge(size_t first, size_t second);

  std::pair<size_t, size_t> find_min_contraction_edge();
  std::pair<glm::vec3, float> contraction_error(std::pair<size_t, size_t> vertex_pair);
  void contract_edge(std::pair<size_t, size_t> vertex_pair);

  size_t follow_remappend_vertices(size_t original);
  std::unique_ptr<VertexObject> reconstruct_vo();

public:
  MeshSimplifier(std::unique_ptr<VertexObject> vo);
  std::unique_ptr<VertexObject> simplify(size_t target_vertex_count);
};

class MeshViewerApp : public Application {
private:
  std::shared_ptr<VertexObject> vo;

public:
  MeshViewerApp(const std::string& app_name,
                glm::ivec2 window_size, std::string mesh, size_t num_vertices);
  void SetupScene() override;
};
}  // namespace GLOO