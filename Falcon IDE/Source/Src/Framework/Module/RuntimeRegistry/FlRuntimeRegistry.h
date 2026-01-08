#pragma once
#include "../FlRunTimeAndDLLsCommon.hxx"

extern "C" {

	/*
	 RegisterComponent:
	  - dllName: DLL の一意名 (const char*). 例 "PhysicsDLL"
	  - entityId: 該当エンティティの ID
	  - componentName: コンポーネントの識別名 (例 "BoxCollision")
	  - ptr: DLL が生成した実体のポインタ (void*)
	  - 戻り: 0 = OK, 非0 = エラー
	*/
	int Runtime_RegisterComponent(const char* dllName, uint32_t entityId, const char* componentName, void* ptr);

	/*
	 UnregisterComponent:
	  - 登録を解除 (dllName, entityId, componentName) の一致分だけ消す
	*/
	int Runtime_UnregisterComponent(const char* dllName, uint32_t entityId, const char* componentName);

	/*
	 GetComponent:
	  - 他DLLが参照する用。存在しない場合は nullptr を返す。
	*/
	void* Runtime_GetComponent(const char* dllName, uint32_t entityId, const char* componentName);

	/*
	 UnregisterAllForDll:
	  - DLL アンロード前に DLL 側が呼ぶ。ランタイムはその DLL による全登録を消す。
	*/
	int Runtime_UnregisterAllForDll(const char* dllName);

	/*
	 EnumerateComponentsForEntity:
	  - entityId に紐づく登録一覧を列挙するためのコールバック。
	  - callback は (const char* dllName, const char* compName, void* ptr, void* user)
	  - 戻り: 列挙に成功したら 0
	*/
	using RuntimeEnumCallback = void(*)(const char*, const char*, void*, void*);
	int Runtime_EnumerateComponentsForEntity(uint32_t entityId, RuntimeEnumCallback cb, void* user);

	/*
	 Runtime_LoadDll:
	  - dllName: DLLの一意名
	  - dllPath: DLLへのパス
	  - 戻り: Loadに成功したら true
	*/
	bool Runtime_LoadDll(const char* dllName, const char* dllPath);

	void* Runtime_CreateComponent(const char* dllName, const char* componentName, uint32_t entityId);
	int Runtime_DeleteComponent(const char* dllName, const char* componentName, uint32_t entityId);

	int Runtime_RegisterSystem(const char* dllName, void* instance, SystemLogics* funcs);
	int Runtime_UnregisterSystem(const char* dllName);
	SystemLogics* Runtime_GetSystem(const char* dllName);

} // extern "C"
