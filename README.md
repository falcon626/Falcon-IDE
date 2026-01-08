# Falcon-IDE

## 概要

`Falcon-IDE` は、ネイティブ開発向けに設計された IDE/ツールセットの C++ ベース実装です。本リポジトリはレンダリング、エディタ拡張、スクリプトモジュールなどのサブシステムを含みます。主要な GUI やユーティリティの一部にサードパーティライブラリ（例: Dear ImGui、stb 系）を組み込んでいます。

## 主なディレクトリ構成

- `Src/` - ソースコード本体。
  - `Src/Framework/ImGui/` - Dear ImGui 関連のラッパーや patched stb ヘッダ。
  - `ScriptModule/` - スクリプト関連モジュール。
- `ThirdParty/` - 外部ライブラリ（サブモジュールまたは同梱）。
- `Build/` - ビルド成果物（通常は `.gitignore` で除外）。

※ リポジトリ内のヘッダにライセンス注記が含まれているため、個々のファイルのライセンスを必ず確認してください。

## 必要環境

- OS: Windows 10/11
- 開発環境: Visual Studio 2026（MSVC toolset）
- C++: C++20 を有効にしてください
- 追加: Windows SDK（最新推奨）、必要に応じて DirectX 12 開発キット

## ビルド手順（Visual Studio）

1. `Falcon-IDE.sln` を Visual Studio で開くか、直接プロジェクトファイル（例: `.vcxproj`）を開いてください。
2. __Solution Explorer__ でビルド構成を選択（`Debug` / `Release`）します。必要なら __Build > Configuration Manager__ でプラットフォームや設定を調整してください。
3. 各プロジェクトのプロパティを確認し、__Project Properties__ > __C/C++__ > __Language__ で `C++20` が選択されていることを確認してください。
4. ビルドは __Build > Build Solution__ を実行します。

コマンドラインから MSBuild を使う場合:
