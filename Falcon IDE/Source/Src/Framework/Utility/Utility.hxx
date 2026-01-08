#pragma once

// <Inline Anonymous Namespace:インライン無名名前空間>
inline namespace
{
	using Microsoft::WRL::ComPtr;
	namespace Math = DirectX::SimpleMath;

	inline auto ConvertColor(const Math::Color& color) 
	{
		return color.RGBA().v;
	}

	inline auto ConvertBack(uint32_t packedColor) 
	{
		auto a{ ((packedColor >> 24) & 0xFF) / 255.0f };
		auto r{ ((packedColor >> 16) & 0xFF) / 255.0f };
		auto g{ ((packedColor >>  8) & 0xFF) / 255.0f };
		auto b{ ((packedColor)		 & 0xFF) / 255.0f };
		return Math::Color(r, g, b, a);
	}

	inline auto FloatArrayToMatrix(const float* array) 
	{
		auto element{ size_t{NULL} };
		return Math::Matrix(
			array[element++], array[element++], array[element++], array[element++],
			array[element++], array[element++], array[element++], array[element++],
			array[element++], array[element++], array[element++], array[element++],
			array[element++], array[element++], array[element++], array[element++]
		);
	}

	inline auto MatrixToFloatArray(const Math::Matrix& matrix, float* outArray)
	{
		const float* matrixArray{ reinterpret_cast<const float*>(&matrix) };
		std::copy(matrixArray, matrixArray + 16, outArray);
	}
};
// </Inline Anonymous Namespace:インライン無名名前空間>