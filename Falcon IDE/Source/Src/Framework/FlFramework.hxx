#pragma once

// <Window:ウィンドウ>
#include "System/Window/Window.h"

// <Utility:便利機能>
#include "Utility/Utility.hxx"
#include "Utility/FlUtilityDefault.hxx"
#include "Utility/FlUtilityMath.hxx"
#include "Utility/FlUtilityString.hxx"
#include "Utility/FlUtilityContainer.hxx"
#include "Utility/FlUtilityJson.hxx"
#include "Utility/FlUtilityFilePath.hpp"
#include "Utility/FlUtilityEasingAnimator.hpp"

// <Multithread:並列処理>
#include "System/Multithread/FlMultithreadController.h"

// <Math:計算処理関連>
#include "Math/FlTransform.hpp"

// <Debug:開発支援関連>
#include "System/Debugger/Console/Console.hpp"
#include "System/Debugger/Logger/FlDebugLogger.hpp"

// <FramePerSecondController:フレーム制御>
#include "System/FrameControl/FlFrameRateController.h"

// <Watcher:監視関連>
#include "System/Watcher/FlFileWatcher.h"

// <Graphics:描画関連>
#include "Graphics/Graphics.hxx"

// <GUID:グローバルユニークID>
#include "System/GUID/FlGUID.h"

// <AllResource:素材管理関連>
#include "Resource/FlResourceAdministrator.h"

// <Crypter:秘匿化>
#pragma comment(lib,"FlCrypter.lib")
#include "FlCrypter/Src/FlCrypter.h"

// <Scene:シーン管理関連>
#include "Application/Scene/FlScene.h"

// <Editor:エディタ関連>
#include "ImGui/Editor/FlEditorAdministrator.h"