#pragma once

// <Log:ログ>
#include "Editor/FlLogEditor.h"

// <File:ファイル>
#include "Editor/FlFileEditor.h"

// <List:列挙>
#include "Editor/FlListingEditor.h"

// <Terminal:ターミナル>
#include "Editor/FlTerminalEditor.h"

// <InspectorAndHierarchy:インスペクターとヒエラルキー>
#include "Editor/FlECSInspectorAndHierarchy.h"

// <Script:モジュールシステム>
#include "FlScriptModuleEditor.h"

// <Macro:マクロ>
#include "FlPythonMacroEditor.h"

// <DeveloperCommandPrompt:ビルドツール>
#include "FlDeveloperCommandPromptEditor.h"

// <Camera:カメラ>
#include "FlEditorCamera.h"

// <EditorStyle:エディタスタイル>
#include "../CascadingStyleSheets/FlEditorCascadingStyleSheets.h"

#ifndef PACKAGE

#endif // !PACKAGE
/// <summary> =Singleton= </summary>
class FlEditorAdministrator
{
public:
	/// <summary>
	/// 指定されたウィンドウとフレームレートコントローラーで初期化を行います。
	/// </summary>
	/// <param name="hwnd">初期化対象のウィンドウのハンドル。</param>
	/// <param name="windowW">ウィンドウの幅（ピクセル単位）。</param>
	/// <param name="windowH">ウィンドウの高さ（ピクセル単位）。</param>
	/// <param name="wpFrame">フレームレートコントローラーへの弱い参照。</param>
	/// <returns>初期化が成功した場合は true、失敗した場合は false を返します。</returns>
	bool Initialize(const HWND hwnd, const int32_t windowW, const int32_t windowH, const std::weak_ptr<FlFrameRateController>& wpFrame) noexcept;
	
	/// <summary>
	/// オブジェクトの状態を更新します。
	/// </summary>
	void Update() noexcept;

	void EditorCameraUpdate() noexcept;

	/// <summary>
	/// リソースを解放します。
	/// </summary>
	void Release() noexcept;

	/// <summary>
	/// #Getter ロガーオブジェクトへの参照を取得します。
	/// </summary>
	/// <returns>メンバー変数 m_upLogEditor への定数参照。</returns>
	const auto& GetLogger() const noexcept { return m_upLogEditor; }
	
	/// <summary>
	/// #Getter ImGuiContextへの参照を所得します。
	/// </summary>
	/// <returns>ImGuiContext*への定数参照。</returns>
	const auto GetContext() const noexcept { return m_pImGuiContext; }

	/// <summary>
	/// #Setter ドロップされたファイルのパスを設定します。
	/// </summary>
	/// <param name="path">設定するファイルのパス。</param>
	/// <returns>なし（void型）。</returns>
	auto SetDroppedFilePath(const std::filesystem::path& path) noexcept { m_upFileEditor->SetDroppedFile(path); }

	/// <summary>
	/// #Getter GameTimeの停止状態を所得
	/// </summary>
	/// <returns>bool型で止まっていればtrue</returns>
	const auto GetIsStop() const noexcept { return m_isStop; }

	/// <summary>
	/// #Getter を所得
	/// </summary>
	/// <returns>bool型でカメラが付いていればtrue</returns>
	const auto GetEditorCameraIsEnable() const noexcept { return m_upEditorCamera->IsEnable(); }

	/// <summary>
	/// HierarchyのEntityListを更新
	/// </summary>
	/// <returns>なし</returns>
	const auto RefreshHierarchy() const noexcept { m_upECSInspectorAndHierarchyEditor->RefreshEntityList(); }

	/// <summary>
	/// シングルトンインスタンスへの参照を返します。
	/// </summary>
	/// <returns>FlEditorAdministrator の唯一のインスタンスへの定数参照。</returns>
	static auto& Instance() noexcept
	{
		static auto instance{ FlEditorAdministrator{} };
		return instance;
	}
private:
	/// <summary>
	/// 処理を開始します。
	/// </summary>
	void Begin() noexcept;
	/// <summary>
	/// 処理を終了します。
	/// </summary>
	void End()	 noexcept;
	/// <summary>
	/// ドックレイアウトをセットアップします。
	/// </summary>
	void SetupDockLayout() const noexcept;

	void SetupThemes(const std::unique_ptr<FlEditorCascadingStyleSheets>& css);

	FlEditorAdministrator() = default;
	~FlEditorAdministrator() { Release(); }

	FlEditorAdministrator(const FlEditorAdministrator&)			   = delete; // コピーコンストラクタ削除
	FlEditorAdministrator& operator=(const FlEditorAdministrator&) = delete; // 代入演算子削除
	FlEditorAdministrator(FlEditorAdministrator&&)				   = delete; // ムーブコンストラクタ削除
	FlEditorAdministrator& operator=(FlEditorAdministrator&&)	   = delete; // ムーブ代入演算子削除

	ComPtr<ID3D12DescriptorHeap>				   m_pImGuiSrvHeap;				    // ImGuiのSRVヒープへのポインタ

	std::unique_ptr<Texture>					   m_sceneTex;					    // シーン用テクスチャ
	std::unique_ptr<FlLogEditor>				   m_upLogEditor;				    // ログエディタのインスタンス
	std::unique_ptr<FlFileEditor>				   m_upFileEditor;				    // ファイルエディタのインスタンス
	std::unique_ptr<FlListingEditor>			   m_upListingEditor;				// リスト列挙エディタのインスタンス
	std::unique_ptr<FlTerminalEditor>			   m_upTerminalEditor;			    // ターミナルエディタのインスタンス
	std::unique_ptr<FlScriptModuleEditor>		   m_upScriptModuleEditor;			// スクリプトモジュールエディタのインスタンス
	std::unique_ptr<FlPythonMacroEditor>		   m_upPythonMacroEditor;			// パイソンマクロエディタのインスタンス
	std::unique_ptr<FlDeveloperCommandPromptEditor>m_upDevCmdEditor;				// MSVS2022デベロパCmdのインスタンス
	std::unique_ptr<FlECSInspectorAndHierarchy>    m_upECSInspectorAndHierarchyEditor; // インスペクターとヒエラルキーエディタのインスタンス
	std::unique_ptr<FlEditorCamera>				   m_upEditorCamera;				// エディタ用カメラのインスタンス

	std::unique_ptr<FlEditorCascadingStyleSheets> m_upEditorStyle; // エディタスタイルのインスタンス

	std::weak_ptr<FlFrameRateController> m_wpFrameRateController;  // フレームレートコントローラのウィークポインタ

	D3D12_CPU_DESCRIPTOR_HANDLE m_gameViewCPUHandle{}; // ゲームビューのSRV用CPUハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE m_gameViewGPUHandle{}; // ゲームビューのSRV用GPUハンドル

	ImGuiContext* m_pImGuiContext{ nullptr }; // ImGuiコンテキストのポインタ
	D3D12_GPU_DESCRIPTOR_HANDLE m_sceneTexImGuiHandle{};

	int32_t m_windowW{};
	int32_t m_windowH{};

	bool m_firstRun{ true };                // 初回実行フラグ
	ImGuiID m_dockspaceID{ Def::UIntZero }; // DockSpace ID保持用

	int32_t m_targetFrameRate{ Def::BitMaskPos9 };

	bool m_isStop{ false };
};



