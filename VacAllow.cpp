#include <stdio.h>
#include "VacAllow.h"
#include "metamod_oslink.h"
#include <funchook.h>

using namespace DynLibUtils;

VacAllow g_VacAllow;
PLUGIN_EXPOSE(VacAllow, g_VacAllow);
IVEngineServer2* engine = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;
CGlobalVars* gpGlobals = nullptr;

IUtilsApi* g_pUtils;

funchook_t* g_pSteamAuthHook = nullptr;

typedef void (*SteamAuthClientFail_t)(void* pThis, void* arg1, void* arg2);
SteamAuthClientFail_t UTIL_SteamAuthClientFail = nullptr;

// Signature from IDA: sub_6EF840
// push rbp; mov rbp,rsp; push r15; push r14; mov r14,rsi; push r13; push r12; push rbx; sub rsp,218h
// More unique - include full sub rsp, 218h
#define SIG_STEAM_AUTH_FAIL "55 48 89 E5 41 57 41 56 49 89 F6 41 55 41 54 53 48 81 EC 18 02 00 00"

void Hook_SteamAuthClientFail(void* pThis, void* arg1, void* arg2)
{
	uintptr_t val1 = (uintptr_t)arg1;
	uintptr_t val2 = (uintptr_t)arg2;
	return;
}

CGameEntitySystem* GameEntitySystem()
{
	return g_pUtils->GetCGameEntitySystem();
}

void StartupServer()
{
	g_pGameEntitySystem = GameEntitySystem();
	g_pEntitySystem = g_pUtils->GetCEntitySystem();
	gpGlobals = g_pUtils->GetCGlobalVars();
}

bool VacAllow::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	ConColorMsg(Color(0, 255, 255, 255), "[VacAllow] Loading VacAllow v1.0...\n");

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);

	g_SMAPI->AddListener(this, this);
	CModule libengine(engine);
	
	UTIL_SteamAuthClientFail = libengine.FindPattern(SIG_STEAM_AUTH_FAIL).RCast<decltype(UTIL_SteamAuthClientFail)>();
	
	if (!UTIL_SteamAuthClientFail)
	{
		ConColorMsg(Color(255, 0, 0, 255), "[VacAllow] Could not find SteamAuthClientFail signature!\n");
		snprintf(error, maxlen, "Could not find SteamAuthClientFail signature");
		return false;
	}
	
	g_pSteamAuthHook = funchook_create();
	if (!g_pSteamAuthHook)
	{
		ConColorMsg(Color(255, 0, 0, 255), "[VacAllow] Failed to create funchook!\n");
		snprintf(error, maxlen, "Failed to create funchook");
		return false;
	}
	
	int rv = funchook_prepare(g_pSteamAuthHook, (void**)&UTIL_SteamAuthClientFail, (void*)Hook_SteamAuthClientFail);
	if (rv != 0)
	{
		ConColorMsg(Color(255, 0, 0, 255), "[VacAllow] Failed to prepare hook! Error: %d\n", rv);
		funchook_destroy(g_pSteamAuthHook);
		g_pSteamAuthHook = nullptr;
		snprintf(error, maxlen, "Failed to prepare hook");
		return false;
	}
	
	rv = funchook_install(g_pSteamAuthHook, 0);
	if (rv != 0)
	{
		ConColorMsg(Color(255, 0, 0, 255), "[VacAllow] Failed to install hook! Error: %d\n", rv);
		funchook_destroy(g_pSteamAuthHook);
		g_pSteamAuthHook = nullptr;
		snprintf(error, maxlen, "Failed to install hook");
		return false;
	}
	
	ConColorMsg(Color(0, 255, 0, 255), "[VacAllow] Successfully hooked! VAC banned players can now join.\n");

	return true;
}

bool VacAllow::Unload(char *error, size_t maxlen)
{
	if (g_pSteamAuthHook)
	{
		funchook_uninstall(g_pSteamAuthHook, 0);
		funchook_destroy(g_pSteamAuthHook);
		g_pSteamAuthHook = nullptr;
	}
	
	ConColorMsg(Color(0, 255, 0, 255), "[VacAllow] Plugin unloaded.\n");
	
	return true;
}

void VacAllow::AllPluginsLoaded()
{
	int ret;
	g_pUtils = (IUtilsApi *)g_SMAPI->MetaFactory(Utils_INTERFACE, &ret, NULL);
	if (ret == META_IFACE_FAILED)
	{
		ConColorMsg(Color(255, 0, 0, 255), "[VacAllow] Missing Utils system plugin\n");
		std::string sBuffer = "meta unload "+std::to_string(g_PLID);
		engine->ServerCommand(sBuffer.c_str());
		return;
	}
	g_pUtils->StartupServer(g_PLID, StartupServer);
}

///////////////////////////////////////
const char* VacAllow::GetLicense()
{
	return "GPL";
}

const char* VacAllow::GetVersion()
{
	return "1.0";
}

const char* VacAllow::GetDate()
{
	return __DATE__;
}

const char *VacAllow::GetLogTag()
{
	return "[VacAllow]";
}

const char* VacAllow::GetAuthor()
{
	return "ABKAM";
}

const char* VacAllow::GetDescription()
{
	return "VacAllow";
}

const char* VacAllow::GetName()
{
	return "VacAllow";
}

const char* VacAllow::GetURL()
{
	return "https://discord.gg/ChYfTtrtmS";
}
