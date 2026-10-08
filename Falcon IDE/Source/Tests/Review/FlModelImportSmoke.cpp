// Compile with the project's installed DirectXTK and Assimp include directories.
// This exercises the production CPU import calculations without a GPU/window.
#define NOMINMAX
#include <Windows.h>
#include <SimpleMath.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Math = DirectX::SimpleMath;
namespace Def
{
    constexpr int IntOne = 1;
    constexpr Math::Matrix Mat{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
}
class Mesh;
#include "../../Src/Framework/Graphics/Model/FlModelImportData.h"

#define CHECK(condition) do { if (!(condition)) { std::fprintf(stderr, "FAIL at %d: %s\n", __LINE__, #condition); std::exit(1); } } while (false)

bool Near(float left, float right) { return std::abs(left - right) < 0.0001f; }

int main()
{
    ModelData model;
    auto& nodes = model.WorkNodes();
    nodes.resize(3);
    nodes[0].m_nodeIndex = 0;
    nodes[0].m_mLocal = Math::Matrix::CreateScale(2) * Math::Matrix::CreateTranslation(10, 0, 0);
    nodes[0].m_children = { 1 };
    nodes[1].m_nodeIndex = 1;
    nodes[1].m_parentIndex = 0;
    nodes[1].m_mLocal = Math::Matrix::CreateTranslation(3, 0, 0);
    nodes[1].m_children = { 2 };
    nodes[2].m_nodeIndex = 2;
    nodes[2].m_parentIndex = 1;
    nodes[2].m_mLocal = Def::Mat; // Extra mesh child keeps its parent's transform.
    std::vector<Math::Matrix> worlds;
    CalculateNodeWorldMatrices(nodes, nodes.size(), worlds);
    CHECK(Near(worlds[0].Translation().x, 10)); // Root is applied exactly once.
    CHECK(Near(worlds[1].Translation().x, 16)); // Child translation sees parent scale.
    CHECK(Near(worlds[2].Translation().x, 16));
    CHECK(Near((worlds[1] * Math::Matrix::CreateTranslation(50, 0, 0)).Translation().x, 66));

    // A generated extra-mesh name must not steal an original animation target.
    nodes[0].m_name = "Body";
    nodes[0].m_children = { 1, 2 };
    nodes[1].m_name = "Body#mesh1";
    nodes[1].m_children.clear();
    nodes[2].m_name = "Body#mesh1";
    nodes[2].m_parentIndex = 0;
    const std::map<std::string, int32_t> originalNodeNames{ { "Body", 0 }, { "Body#mesh1", 1 } };
    AnimationData::Channel channel;
    channel.m_name = "Body#mesh1";
    BindAnimationChannel(channel, originalNodeNames);
    CHECK(channel.m_nodeOffset == 1);
    nodes[channel.m_nodeOffset].m_mLocal = Math::Matrix::CreateTranslation(5, 0, 0);
    CalculateNodeWorldMatrices(nodes, nodes.size(), worlds);
    CHECK(Near(worlds[1].Translation().x, 20));
    CHECK(Near(worlds[2].Translation().x, 10));
    channel.m_name = "missing";
    BindAnimationChannel(channel, originalNodeNames);
    CHECK(channel.m_nodeOffset == -1);

    nodes[0].m_boneIndex = 3;
    nodes[1].m_boneIndex = 7;
    const std::map<std::string, int32_t> boneNodes{ { "BoneA", 0 }, { "BoneB", 1 } };
    aiMesh mesh;
    mesh.mNumVertices = 3;
    mesh.mVertices = new aiVector3D[3]{ { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } };
    mesh.mNumBones = 2;
    mesh.mBones = new aiBone*[2]{ new aiBone, new aiBone };
    mesh.mBones[0]->mName.Set("BoneA");
    mesh.mBones[0]->mNumWeights = 1;
    mesh.mBones[0]->mWeights = new aiVertexWeight[1]{ { 0, 0.25f } };
    mesh.mBones[1]->mName.Set("BoneB");
    mesh.mBones[1]->mNumWeights = 2;
    mesh.mBones[1]->mWeights = new aiVertexWeight[2]{ { 0, 0.75f }, { 1, 0.5f } };
    const auto vertices = ParseMeshVertices(mesh, model, boneNodes);
    CHECK(vertices.Position.size() == 3 && Near(vertices.Position[1].x, 4));
    CHECK(vertices.SkinIndexList[0][0] == 3 && vertices.SkinIndexList[0][1] == 7);
    CHECK(Near(vertices.SkinWeightList[0][0], 0.25f) && Near(vertices.SkinWeightList[0][1], 0.75f));
    CHECK(vertices.SkinIndexList[1][0] == 7 && Near(vertices.SkinWeightList[1][0], 1));
    CHECK(vertices.SkinIndexList[2][0] == 0 && vertices.SkinWeightList[2][0] == 0);
    std::puts("FlModelImportSmoke: PASS");
}
