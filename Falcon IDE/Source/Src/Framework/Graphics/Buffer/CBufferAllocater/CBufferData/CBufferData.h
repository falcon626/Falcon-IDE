#pragma once

namespace CBufferData
{
	struct Camera
	{
		Math::Matrix mView;
		Math::Matrix mProj;
	};

	struct WorldMatrix
	{
		Math::Matrix world;
	};

	struct BoneTransforms 
	{
		std::array<Math::Matrix, 64>boneTransforms;
	};

	struct IsSkinMesh
	{
		uint32_t isSkin;
	};

	struct MaterialCBData
	{
		Math::Vector4 baseColor;
	};
}