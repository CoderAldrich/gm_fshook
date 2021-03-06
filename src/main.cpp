#include <vector>
#include <mutex>
#include "filesystem.h"
#include "interface.h"
#include "threadtools.h"
#include "vhook.h"
#include "vfuncs.h"


IFileSystem *g_pFullFileSystem = nullptr;
static CDllDemandLoader filesystem_stdio_factory( "filesystem_stdio" );
VirtualFunctionHooks *FunctionHooks = nullptr;

static std::vector<std::string> logs;

void FSLog(std::string msg) {
	logs.push_back(msg);
}

static int FSLogNotify(lua_State *state) {
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "hook");
	LUA->GetField(-1, "Run");
	for (std::string &msg : logs) {
		LUA->Push(-1);
		LUA->PushString("FSLogNotify");
		LUA->PushString(msg.c_str());
		LUA->Call(2, 0);
	}
	LUA->Pop(3);
	return 0;
}

GMOD_MODULE_OPEN() {
	FunctionHooks = new VirtualFunctionHooks;
	FunctionHooks->mainthread = ThreadGetCurrentId();
	FunctionHooks->lua = LUA;

	CreateInterfaceFn factory = filesystem_stdio_factory.GetFactory();
	if(factory == nullptr) {
		LUA->ThrowError("failed to get filesystem_stdio interface factory");
	}

	g_pFullFileSystem = (IFileSystem *)factory(FILESYSTEM_INTERFACE_VERSION, NULL);
	if(g_pFullFileSystem == nullptr) {
		LUA->ThrowError("failed to get IFileSystem interface");
	}

	IBaseFileSystem *pBaseFileSystem = (IBaseFileSystem *)g_pFullFileSystem;
	FunctionHooks->BaseFileSystemReplacer = new VirtualReplacer<IBaseFileSystem>(g_pFullFileSystem);
	void *fn = GetVirtualAddress(FunctionHooks, &VirtualFunctionHooks::IBaseFileSystem__Open);
	FunctionHooks->IBaseFileSystem__Open__index = GetVirtualIndex(pBaseFileSystem, &IBaseFileSystem::Open);
	FunctionHooks->BaseFileSystemReplacer->Hook(FunctionHooks->IBaseFileSystem__Open__index, fn);


	FunctionHooks->FileSystemReplacer = new VirtualReplacer<IFileSystem>(g_pFullFileSystem);
	FunctionHooks->IFileSystem__FindFirstEx__index = GetVirtualIndex(g_pFullFileSystem, &IFileSystem::FindFirstEx);
	FunctionHooks->IFileSystem__FindNext__index = GetVirtualIndex(g_pFullFileSystem, &IFileSystem::FindNext);
	FunctionHooks->IFileSystem__FindClose__index = GetVirtualIndex(g_pFullFileSystem, &IFileSystem::FindClose);
	FunctionHooks->IFileSystem__FindIsDirectory__index = GetVirtualIndex(g_pFullFileSystem, &IFileSystem::FindIsDirectory);

	FunctionHooks->FileSystemReplacer->Hook(FunctionHooks->IFileSystem__FindFirstEx__index, GetVirtualAddress(FunctionHooks, &VirtualFunctionHooks::IFileSystem__FindFirstEx));
	FunctionHooks->FileSystemReplacer->Hook(FunctionHooks->IFileSystem__FindNext__index, GetVirtualAddress(FunctionHooks, &VirtualFunctionHooks::IFileSystem__FindNext));
	FunctionHooks->FileSystemReplacer->Hook(FunctionHooks->IFileSystem__FindClose__index, GetVirtualAddress(FunctionHooks, &VirtualFunctionHooks::IFileSystem__FindClose));

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "hook");
	LUA->GetField(-1, "Add");
	LUA->PushString("Think");
	LUA->PushString("gm_fs");
	LUA->PushCFunction(FSLogNotify);
	LUA->Call(3, 0);
	LUA->Pop(2);

	return 0;
}

GMOD_MODULE_CLOSE() {

	delete FunctionHooks->BaseFileSystemReplacer;
	delete FunctionHooks;

	return 0;
}
