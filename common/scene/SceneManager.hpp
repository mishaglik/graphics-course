#pragma once

#include <filesystem>

#include <glm/glm.hpp>
#include <tiny_gltf.h>
#include <etna/Buffer.hpp>
#include <etna/BlockingTransferHelper.hpp>
#include <etna/VertexInput.hpp>


// A single render element (relem) corresponds to a single draw call
// of a certain pipeline with specific bindings (including material data)
struct RenderElement
{
  std::uint32_t vertexOffset;
  std::uint32_t indexOffset;
  std::uint32_t indexCount;
  // Not implemented!
  // Material* material;
};


// A mesh is a collection of relems. A scene may have the same mesh
// located in several different places, so a scene consists of **instances**,
// not meshes.
struct Mesh
{
  std::uint32_t firstRelem;
  std::uint32_t relemCount;
};

class SceneManager
{
public:
  SceneManager();

  void selectScene(std::filesystem::path path);
  void selectSceneBaked(std::filesystem::path path);

  // Every instance is a mesh drawn with a certain transform
  // NOTE: maybe you can pass some additional data through unused matrix entries?
  std::span<const glm::mat4x4> getInstanceMatrices() { return instanceMatrices; }
  std::span<const std::uint32_t> getInstanceMeshes() { return instanceMeshes; }

  // Every mesh is a collection of relems
  std::span<const Mesh> getMeshes() { return meshes; }

  // Every relem is a single draw call
  std::span<const RenderElement> getRenderElements() { return renderElements; }
  std::span<const glm::mat2x3  > getBounds()         { return bounds; }

  vk::Buffer getVertexBuffer() { return unifiedVbuf.get(); }
  vk::Buffer getIndexBuffer() { return unifiedIbuf.get(); }
  
  etna::Buffer& getLightSourcesPosBuffer() { return unifiedLPbuf; }
  etna::Buffer& getLightSourcesColBuffer() { return unifiedLCbuf; }


  etna::VertexByteStreamFormatDescription getVertexFormatDescription();

  int nLights() const { return static_cast<int>(lightSourcesPoses.size()); }

private:

  std::optional<tinygltf::Model> loadModel(std::filesystem::path path);

  struct ProcessedInstances
  {
    std::vector<glm::mat4x4> matrices;
    std::vector<std::uint32_t> meshes;
  };

  ProcessedInstances processInstances(const tinygltf::Model& model) const;

  struct Vertex
  {
    // First 3 floats are position, 4th float is a packed normal
    glm::vec4 positionAndNormal;
    // First 2 floats are tex coords, 3rd is a packed tangent, 4th is padding
    glm::vec4 texCoordAndTangentAndPadding;
  };

  static_assert(sizeof(Vertex) == sizeof(float) * 8);
  static glm::mat2x3 getBounds(std::span<const Vertex>);

  struct ProcessedMeshes
  {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<RenderElement> relems;
    std::vector<Mesh> meshes;
    std::vector<glm::mat2x3> bounds;
  };
  ProcessedMeshes processMeshes(const tinygltf::Model& model) const;
  ProcessedMeshes processMeshesBaked(const tinygltf::Model& model) const;
  void uploadData(std::span<const Vertex> vertices, std::span<const std::uint32_t>);

  void setupLights();
private:
  tinygltf::TinyGLTF loader;
  std::unique_ptr<etna::OneShotCmdMgr> oneShotCommands;
  etna::BlockingTransferHelper transferHelper;

  std::vector<RenderElement> renderElements;
  std::vector<Mesh> meshes;
  std::vector<glm::mat4x4> instanceMatrices;
  std::vector<std::uint32_t> instanceMeshes;
  std::vector<glm::mat2x3> bounds;

  etna::Buffer unifiedVbuf;
  etna::Buffer unifiedIbuf;
  etna::Buffer unifiedLPbuf;
  etna::Buffer unifiedLCbuf;

  std::vector<glm::vec4> lightSourcesPoses;
  std::vector<glm::vec4> lightSourcesColors;
};
