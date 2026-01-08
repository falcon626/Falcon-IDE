#pragma once

extern "C" struct SystemLogics {
	// ŠÖ”ƒ|ƒCƒ“ƒ^ 
	void (*Update)(void*, float, uint32_t);
	void (*RenderEditor)(void*, void*);
	size_t(*Serialize)(void*, void*);
	void (*Deserialize)(void*, const void*, size_t);
};