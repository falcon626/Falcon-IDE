#pragma once

namespace FlThumbnailGenerator
{
    // サムネイル生成の設定
    struct ThumbnailConfig
    {
        std::string modelPath; // glTFまたはFBXファイルのパス
        std::string outputPath; // 出力PNGファイルのパス
        uint32_t width = 256; // サムネイルの幅
        uint32_t height = 256; // サムネイルの高さ
        float cameraDistance = 5.0f; // モデルからのカメラ距離
    };

    // サムネイル生成結果
    struct ThumbnailResult
    {
        bool success;
        std::string errorMessage;
    };

    // サムネイルを生成するメイン関数
    ThumbnailResult GenerateThumbnail(const ThumbnailConfig& config);
}