#pragma once

// メッシュの頂点情報
struct MeshVertex
{
	std::vector<Math::Vector3> Position;
	std::vector<Math::Vector2> UV;
	std::vector<Math::Vector3> Normal;
	std::vector<uint32_t> Color;
	std::vector<Math::Vector3> Tangent;
	
	std::vector<std::array<short, 4>>	SkinIndexList{};	// スキンメッシュ対応
	std::vector<std::array<float, 4>>	SkinWeightList{};
};