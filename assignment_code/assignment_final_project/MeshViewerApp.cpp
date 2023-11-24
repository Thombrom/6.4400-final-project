#include "MeshViewerApp.h"
#include "helpers.hpp"
#include "glm/gtx/string_cast.hpp"

#include "gloo/shaders/PhongShader.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/components/CameraComponent.hpp"
#include "gloo/components/LightComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/MeshLoader.hpp"
#include "gloo/lights/PointLight.hpp"
#include "gloo/lights/AmbientLight.hpp"
#include "gloo/cameras/ArcBallCameraNode.hpp"
#include "gloo/debug/AxisNode.hpp"

namespace GLOO {
MeshViewerApp::MeshViewerApp(const std::string& app_name, glm::ivec2 window_size, std::string obj_path, size_t target_vertex_count)
    : Application(app_name, window_size) 
{
    MeshData mesh_data = MeshLoader::Import(obj_path);
    MeshSimplifier simplifier(std::move(mesh_data.vertex_obj));
    vo = simplifier.simplify(target_vertex_count);
    vo->UpdateNormals(CalculateNormals(vo->GetPositions(), vo->GetIndices()));
}

void MeshViewerApp::SetupScene() {
    SceneNode& root = scene_->GetRootNode();
    auto shader = std::make_shared<PhongShader>();

    auto camera_node = make_unique<ArcBallCameraNode>(45.f, 0.75f, 5.0f);
    scene_->ActivateCamera(camera_node->GetComponentPtr<CameraComponent>());
    root.AddChild(std::move(camera_node));

    root.AddChild(make_unique<AxisNode>('A'));

    auto ambient_light = std::make_shared<AmbientLight>();
    ambient_light->SetAmbientColor(glm::vec3(0.5f));
    root.CreateComponent<LightComponent>(ambient_light);

    auto point_light = std::make_shared<PointLight>();
    point_light->SetDiffuseColor(glm::vec3(0.8f, 0.8f, 0.8f));
    point_light->SetSpecularColor(glm::vec3(1.0f, 1.0f, 1.0f));
    point_light->SetAttenuation(glm::vec3(1.0f, 0.09f, 0.032f));
    
    auto point_light_node = make_unique<SceneNode>();
    point_light_node->CreateComponent<LightComponent>(point_light);
    point_light_node->GetTransform().SetPosition(glm::vec3(0.0f, 2.0f, 4.f));
    root.AddChild(std::move(point_light_node));

    auto mesh_node = make_unique<SceneNode>();
    mesh_node->CreateComponent<RenderingComponent>(vo);
    mesh_node->CreateComponent<MaterialComponent>(std::make_shared<Material>(Material::GetDefault()));
    mesh_node->CreateComponent<ShadingComponent>(shader);
    mesh_node->GetTransform().SetScale(glm::vec3(0.50f));
    root.AddChild(std::move(mesh_node));
}

MeshSimplifier::MeshSimplifier(std::unique_ptr<VertexObject> vo_) 
    : vo(std::move(vo_)) 
{
    for (size_t i = 0; i < vo->GetPositions().size(); i++) {
        edges.insert({ i, {} });
        vertices.push_back({ vo->GetPositions()[i], glm::mat4(0.0f) });
        vertex_mapping.insert({ i, i });
    }

    // Register all edges
    for (size_t i = 0; i < vo->GetIndices().size(); i += 3) {
        unsigned int a = vo->GetIndices()[i], b = vo->GetIndices()[i + 1], c = vo->GetIndices()[i + 2];
        insert_edge(a, b); insert_edge(b, c); insert_edge(c, a);
    }

    // Calculate the first Q values
    for (size_t i = 0; i < vo->GetIndices().size(); i += 3) {
        unsigned int a = vo->GetIndices()[i], b = vo->GetIndices()[i + 1], c = vo->GetIndices()[i + 2];

        glm::vec3 v1 = vo->GetPositions()[a], v2 = vo->GetPositions()[b], v3 = vo->GetPositions()[c];
        glm::vec3 n = glm::normalize(glm::cross(v2 - v1, v3 - v1));
        float offset = -1.0f * glm::dot(n, v1);
        glm::vec4 pvec =  glm::vec4(n, offset);

        glm::mat4 Q = glm::transpose(glm::mat4 { 
            pvec[0] * pvec[0], pvec[0] * pvec[1], pvec[0] * pvec[2], pvec[0] * pvec[3],
            pvec[1] * pvec[0], pvec[1] * pvec[1], pvec[1] * pvec[2], pvec[1] * pvec[3],
            pvec[2] * pvec[0], pvec[2] * pvec[1], pvec[2] * pvec[2], pvec[2] * pvec[3],
            pvec[3] * pvec[0], pvec[3] * pvec[1], pvec[3] * pvec[2], pvec[3] * pvec[3],
        });

        vertices[a].second += Q;
        vertices[b].second += Q;
        vertices[c].second += Q;
    }
}

void MeshSimplifier::insert_edge(size_t first, size_t second) {
    edges[first].insert(second);
    edges[second].insert(first);
}

void MeshSimplifier::remove_edge(size_t first, size_t second) {
    edges[first].erase(second);
    edges[second].erase(first);
}

std::pair<glm::vec3, float> MeshSimplifier::contraction_error(std::pair<size_t, size_t> vertex_pair) {
    glm::mat4 q = vertices[vertex_pair.first].second + vertices[vertex_pair.second].second;

    glm::mat4 system = glm::transpose(glm::mat4 {
        q[0][0], q[0][1], q[0][2], q[0][3],
        q[1][0], q[1][1], q[1][2], q[1][3],
        q[2][0], q[2][1], q[2][2], q[2][3],
        0.0f, 0.0f, 0.0f, 1.0f
    });

    // std::cout << "Matrix: " << glm::to_string(system) << std::endl;
    // std::cout << "Inverse: " << glm::to_string(glm::inverse(system)) << std::endl;
    // std::cout << "Position: " << glm::to_string(glm::transpose(glm::inverse(system)) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) << std::endl;

    glm::vec4 position = glm::inverse(system) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    float error = glm::dot(position, q * position);

    return { glm::vec3(position), error };
}

std::pair<size_t, size_t> MeshSimplifier::find_min_contraction_edge() {
    std::pair<size_t, size_t> min_edge; float min_error = std::numeric_limits<float>::max();

    for (auto& [first, adj] : edges) {
        for (auto second : adj) {
            auto [new_position, error] = contraction_error({ first, second });
            // std::cout << "Trying to contract vertices " << glm::to_string(vertices[first].first) << " and " << glm::to_string(vertices[second].first) << " into " << glm::to_string(new_position) << " - error: " << error << std::endl;
            if (error < min_error) {
                min_error = error;
                min_edge = { first, second };
            }
        }
    }

    // std::cout << "Found min edge: [" << min_edge.first << ", " << min_edge.second << "] - " << min_error << std::endl;
    return min_edge;
}

void MeshSimplifier::contract_edge(std::pair<size_t, size_t> vertex_pair) {
    auto [new_position, err] = contraction_error(vertex_pair);
    // std::cout << "Contracting vertices " << glm::to_string(vertices[vertex_pair.first].first) << " and " << glm::to_string(vertices[vertex_pair.second].first) << " into " << glm::to_string(new_position) << " - error: " << err << std::endl;

    vertex_mapping[vertex_pair.second] = vertex_pair.first;

    // Update position and Q
    vertices[vertex_pair.first].first = new_position;
    vertices[vertex_pair.first].second += vertices[vertex_pair.second].second;

    // Update edges
    for (auto endpoint : edges[vertex_pair.second]) {
        if (endpoint != vertex_pair.first)
            insert_edge(vertex_pair.first, endpoint);
        edges[endpoint].erase(vertex_pair.second);
    }

    edges.erase(vertex_pair.second);
}

size_t MeshSimplifier::follow_remappend_vertices(size_t original) {
    size_t vertex = original;

    while (vertex_mapping[vertex] != vertex) 
        vertex = vertex_mapping[vertex];

    return vertex;
}

std::unique_ptr<VertexObject> MeshSimplifier::reconstruct_vo() {
    auto positions = make_unique<PositionArray>();
    auto indices = make_unique<IndexArray>();

    // Copy over the positions and keep track of the mapping
    std::unordered_map<size_t, size_t> reconstructed_mapping;
    for (size_t i = 0; i < vertices.size(); i++) {
        if (follow_remappend_vertices(i) != i)
            continue;

        reconstructed_mapping[i] = positions->size();
        positions->push_back(vertices[i].first);
    }

    // Reconstruct the indices
    for (size_t i = 0; i < vo->GetIndices().size(); i += 3) {
        unsigned int a = vo->GetIndices()[i], b = vo->GetIndices()[i + 1], c = vo->GetIndices()[i + 2];

        // If any of the vertices have been contracted, the face is gone and we skip it
        unsigned int ra = follow_remappend_vertices(a), rb = follow_remappend_vertices(b), rc = follow_remappend_vertices(c);
        if (ra == rb || ra == rc || rb == rc)
            continue;

        indices->push_back(reconstructed_mapping[ra]);
        indices->push_back(reconstructed_mapping[rb]);
        indices->push_back(reconstructed_mapping[rc]);
    }

    auto vo = make_unique<VertexObject>();
    vo->UpdatePositions(std::move(positions));
    vo->UpdateIndices(std::move(indices));
    return vo;
}

std::unique_ptr<VertexObject> MeshSimplifier::simplify(size_t target_vertex_count) {
    size_t vertex_count = vertices.size();    
    std::cout << "Vertex count:  " << vertex_count << std::endl;

    while (vertex_count > target_vertex_count) {
        if (vertex_count % 100 == 0) std::cout << "Vertices: " << vertex_count << std::endl;
        auto min_edge = find_min_contraction_edge();
        contract_edge(min_edge);
        vertex_count--;
    }

    return reconstruct_vo();
}


} // namespace GLOO