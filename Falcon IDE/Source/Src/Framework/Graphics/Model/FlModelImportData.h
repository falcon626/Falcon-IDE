#pragma once

#include "Model.h"
#include "../Animation/Animation.h"
#include "../Mesh/MeshData/MeshData.h"
#include <assimp/mesh.h>

inline void BindAnimationChannel(AnimationData::Channel& channel, const std::map<std::string, int32_t>& originalNodes)
{
    const auto node = originalNodes.find(channel.m_name);
    channel.m_nodeOffset = node != originalNodes.end() ? node->second : -1;
}

inline void CalculateNodeWorldMatricesRecursive(
    const std::vector<ModelData::Node>& nodes, int nodeIndex,
    const Math::Matrix& parentWorld, std::vector<Math::Matrix>& outWorldMatrices)
{
    const auto& node = nodes[nodeIndex];
    const auto world = node.m_mLocal * parentWorld;
    outWorldMatrices[nodeIndex] = world;
    for (const auto childIndex : node.m_children)
        CalculateNodeWorldMatricesRecursive(nodes, childIndex, world, outWorldMatrices);
}

inline void CalculateNodeWorldMatrices(
    const std::vector<ModelData::Node>& nodes, const size_t nodeSize,
    std::vector<Math::Matrix>& outWorldMatrices)
{
    outWorldMatrices.resize(nodes.size());
    for (size_t index = 0; index < nodeSize; ++index)
        if (nodes[index].m_parentIndex == -Def::IntOne)
            CalculateNodeWorldMatricesRecursive(nodes, static_cast<int>(index), Def::Mat, outWorldMatrices);
}

inline MeshVertex ParseMeshVertices(
    const aiMesh& mesh, const ModelData& model,
    const std::map<std::string, int32_t>& nodeNameToIndex)
{
    MeshVertex vertices;
    vertices.Position.resize(mesh.mNumVertices);
    if (mesh.HasTextureCoords(0)) vertices.UV.resize(mesh.mNumVertices);
    if (mesh.HasNormals()) vertices.Normal.resize(mesh.mNumVertices);
    if (mesh.HasTangentsAndBitangents()) vertices.Tangent.resize(mesh.mNumVertices);
    if (mesh.HasVertexColors(0)) vertices.Color.resize(mesh.mNumVertices);

    for (unsigned int index = 0; index < mesh.mNumVertices; ++index) {
        const auto& position = mesh.mVertices[index];
        vertices.Position[index] = Math::Vector3(position.x, position.y, position.z);
        if (mesh.HasTextureCoords(0))
            vertices.UV[index] = Math::Vector2(mesh.mTextureCoords[0][index].x, mesh.mTextureCoords[0][index].y);
        if (mesh.HasNormals())
            vertices.Normal[index] = Math::Vector3(mesh.mNormals[index].x, mesh.mNormals[index].y, mesh.mNormals[index].z);
        if (mesh.HasTangentsAndBitangents())
            vertices.Tangent[index] = Math::Vector3(mesh.mTangents[index].x, mesh.mTangents[index].y, mesh.mTangents[index].z);
        if (mesh.HasVertexColors(0)) {
            const auto& color = mesh.mColors[0][index];
            vertices.Color[index] = Math::Color(color.r, color.g, color.b, color.a).RGBA().v;
        }
    }

    if (mesh.HasBones()) {
        vertices.SkinWeightList.resize(mesh.mNumVertices, { 0, 0, 0, 0 });
        vertices.SkinIndexList.resize(mesh.mNumVertices, { 0, 0, 0, 0 });
        for (unsigned int bone = 0; bone < mesh.mNumBones; ++bone) {
            const auto& sourceBone = *mesh.mBones[bone];
            const auto found = nodeNameToIndex.find(sourceBone.mName.C_Str());
            if (found == nodeNameToIndex.end()) continue;
            const auto boneIndex = model.GetNodes()[found->second].m_boneIndex;
            if (boneIndex < 0) continue;

            for (unsigned int weight = 0; weight < sourceBone.mNumWeights; ++weight) {
                const auto& sourceWeight = sourceBone.mWeights[weight];
                if (sourceWeight.mVertexId >= mesh.mNumVertices || sourceWeight.mWeight <= 0) continue;
                auto& weights = vertices.SkinWeightList[sourceWeight.mVertexId];
                auto& indices = vertices.SkinIndexList[sourceWeight.mVertexId];
                for (size_t slot = 0; slot < weights.size(); ++slot) {
                    if (weights[slot] == 0) {
                        weights[slot] = sourceWeight.mWeight;
                        indices[slot] = static_cast<short>(boneIndex);
                        break;
                    }
                }
            }
        }
        for (auto& weights : vertices.SkinWeightList) {
            const auto sum = weights[0] + weights[1] + weights[2] + weights[3];
            if (sum > 0)
                for (auto& weight : weights) weight /= sum;
        }
    }
    return vertices;
}
