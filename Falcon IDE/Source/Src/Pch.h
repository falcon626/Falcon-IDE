#pragma once
// <Precompilation Header>

// ******* //
// <Basic> //
// ******* //
#pragma comment(lib,"winmm.lib")

#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <cassert>

#include <wrl/client.h>

// ***** //
// <STL> //
// ***** //
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <array>
#include <vector>
#include <stack>
#include <list>
#include <iterator>
#include <queue>
#include <algorithm>
#include <memory>
#include <random>
#include <fstream>
#include <iostream>
#include <sstream>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>
#include <fileSystem>
#include <stdexcept>
#include <chrono>
#include <format>
#include <type_traits>
#include <set>
#include <numbers>
#include <cstdint>
#include <omp.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>

// ***** //
// <DPI> //
// ***** //
#if FL_EXECUTION
#include <ShellScalingAPI.h> 
#pragma comment(lib, "Shcore.lib") 
#endif

// ***************** //
// <CharCodeConvert> //
// ***************** //
#if FL_EXECUTION || FL_COMPONENT
#include <strconv.h>
#endif

// ****** //
// <UUID> //
// ****** //
#if FL_EXECUTION
#pragma comment(lib ,"rpcrt4.lib")
#include <initguid.h>
#endif

// ****** //
// <JSON> //
// ****** //
#include "Framework/Resource/Json/json.hpp"

// ***** // 
// <PIX> // 
// ***** // 
#if FL_EXECUTION
#include <pix3.h>
#endif

// ******** //
// <Assimp> //
// ******** //
#if FL_EXECUTION
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

// ****** //
// <FMOD> //
// ****** //
#if FL_EXECUTION || FL_COMPONENT
#include "Framework/Resource/Audio/studio/inc/fmod_studio.hpp"
#include "Framework/Resource/Audio/core/inc/fmod_errors.h"
#endif

// ********* //
// <DirectX> //
// ********* //
#if FL_EXECUTION
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

#include <d3d11.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#endif

#if FL_EXECUTION || FL_COMPONENT
#include <DirectXMath.h>
#include <DirectXCollision.h>
#endif

// XAudio
#if FL_EXECUTION
#pragma comment(lib, "xaudio2_9redist.lib")
#include <Audio.h>
#endif

// DirectX Tool Kit
#if FL_EXECUTION || FL_COMPONENT
#pragma comment(lib, "DirectXTK12.lib")
#include <SimpleMath.h>
#endif

// DirectX Tex
#if FL_EXECUTION
#pragma comment(lib, "DirectXTex.lib")
#include <DirectXTex.h>
#endif

#include <d2d1_1.h>
#include <dwrite.h>

// ***** //
// ImGui //
// ***** //
#if FL_EXECUTION || FL_COMPONENT
#define IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_PLACEMENT_NEW
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "imgui_stdlib.h"
#include "ImGuizmo.h"
#endif

// ********** //
// <Original> //
// ********** //
#if FL_EXECUTION
#include "Framework/FlFramework.hxx"
#elif FL_MODULE
#include "Framework/Module/FlRunTimeAndDLLsCommon.h++"
#include "../DynamicLib/Common/FlModuleFramework.hpp"
#endif