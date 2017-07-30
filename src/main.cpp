#include <vector>
#include <mutex>
#include "filesystem.h"
#include "interface.h"
#include "threadtools.h"
#include "vhook.h"
#include "vfuncs.h"
#include <Windows.h>

IFileSystem *g_pFullFileSystem = NULL;
static CDllDemandLoader filesystem_stdio_factory( "filesystem_stdio" );
VirtualFunctionHooks *FunctionHooks = nullptr;

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


	auto Open_real = &IBaseFileSystem::Open;
	auto Read_real = &IBaseFileSystem::Read;

	char bs[2015];
	snprintf(bs, sizeof(bs), "%p - %p", Open_real, Read_real);

	IBaseFileSystem *pBaseFileSystem = (IBaseFileSystem *)g_pFullFileSystem;
	FunctionHooks->FileSystemReplacer = new VirtualReplacer<IBaseFileSystem>(g_pFullFileSystem);
	void *fn = GetVirtualAddress(FunctionHooks, &VirtualFunctionHooks::IBaseFileSystem__Open);
	
	FunctionHooks->FileSystemReplacer->Hook(2, fn);

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "hook");
	LUA->GetField(-1, "Add");
	LUA->PushString("Think");
	LUA->PushString("gm_fs");
	LUA->PushCFunction(ThinkHook);
	LUA->Call(3, 0);
	LUA->Pop(1);

	return 0;
}

GMOD_MODULE_CLOSE() {

	delete FunctionHooks->FileSystemReplacer;
	delete FunctionHooks;

	return 0;
}
