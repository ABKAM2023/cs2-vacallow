#include <netmessages.pb.h>
#include <network_connection.pb.h>
#include <connectionless_netmessages.pb.h>
#include <stdio.h>
#include "VacAllow.h"
#include "metamod_oslink.h"
#include "schemasystem/schemasystem.h"
#include <serversideclient.h>

VacAllow g_VacAllow;
PLUGIN_EXPOSE(VacAllow, g_VacAllow);
IVEngineServer2* engine = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;
CGlobalVars *gpGlobals = nullptr;

#define NETWORKSERVERSERVICE_INTERFACE_VERSION "NetworkServerService_001"

IUtilsApi* g_pUtils;

SH_DECL_HOOK2_void(CServerSideClientBase, Disconnect, SH_NOATTRIB, 0, ENetworkDisconnectionReason, const char*);
SH_DECL_HOOK8(CNetworkGameServerBase, ConnectClient, SH_NOATTRIB, 0, CServerSideClientBase*, 
	const char*, ns_address*, HSteamNetConnection, const C2S_CONNECT_Message&, const char*, const byte*, int, bool);

CUtlVector<CServerSideClientBase*> g_HookedClients;

void Hook_Disconnect(ENetworkDisconnectionReason reason, const char* pszInternalReason)
{
	CServerSideClientBase* pClient = META_IFACEPTR(CServerSideClientBase);
	
	if (reason == NETWORK_DISCONNECT_STEAM_VACBANSTATE || 
	    reason == NETWORK_DISCONNECT_STEAM_VAC_CHECK_TIMEDOUT ||
	    reason == NETWORK_DISCONNECT_STEAM_BANNED)
	{
		pClient->m_bFullyAuthenticated = true;

		RETURN_META(MRES_SUPERCEDE);
	}
	
	RETURN_META(MRES_IGNORED);
}

void HookClient(CServerSideClientBase* pClient)
{
	if (pClient && g_HookedClients.Find(pClient) == g_HookedClients.InvalidIndex())
	{
		SH_ADD_HOOK(CServerSideClientBase, Disconnect, pClient, SH_STATIC(Hook_Disconnect), false);
		g_HookedClients.AddToTail(pClient);
	}
}

CServerSideClientBase* Hook_ConnectClient(const char* pszName, ns_address* pAddr, HSteamNetConnection hPeer, 
	const C2S_CONNECT_Message& msg, const char* pszDecryptedPassword, const byte* pAuthTicket, int nAuthTicketLength, bool bIsLowViolence)
{
	CServerSideClientBase* pClient = META_RESULT_ORIG_RET(CServerSideClientBase*);
	
	if (pClient)
	{
		HookClient(pClient);
	}
	
	RETURN_META_VALUE(MRES_IGNORED, pClient);
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
	
	CNetworkGameServerBase* pServer = g_pNetworkServerService->GetIGameServer();
	if (pServer)
	{
		SH_ADD_HOOK(CNetworkGameServerBase, ConnectClient, pServer, SH_STATIC(Hook_ConnectClient), true); // post hook
		
		const auto& clients = pServer->GetClients();
		for (int i = 0; i < clients.Count(); i++)
		{
			if (clients[i])
			{
				HookClient(clients[i]);
			}
		}
	}
}

bool VacAllow::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);

	g_SMAPI->AddListener( this, this );

	return true;
}

bool VacAllow::Unload(char *error, size_t maxlen)
{
	ConVar_Unregister();
	
	return true;
}

void VacAllow::AllPluginsLoaded()
{
	char error[64];
	int ret;
	g_pUtils = (IUtilsApi *)g_SMAPI->MetaFactory(Utils_INTERFACE, &ret, NULL);
	if (ret == META_IFACE_FAILED)
	{
		g_SMAPI->Format(error, sizeof(error), "Missing Utils system plugin");
		ConColorMsg(Color(255, 0, 0, 255), "[%s] %s\n", GetLogTag(), error);
		std::string sBuffer = "meta unload "+std::to_string(g_PLID);
		engine->ServerCommand(sBuffer.c_str());
		return;
	}
	g_pUtils->StartupServer(g_PLID, StartupServer);
}

const char* VacAllow::GetLicense()
{
	return "GPL";
}

const char* VacAllow::GetVersion()
{
	return "1.0.1";
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
