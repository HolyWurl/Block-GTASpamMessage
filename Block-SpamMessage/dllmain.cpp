#include "pch.h"
#include "minhook/minhook.h"
#include "sigscan.h"

bool g_running = true;
bool IsSpam(std::string message) {
	const char* words[] = {"外挂", "群", "刷", "VX", "微", "解锁", "辅助", "淘宝", "卡网", "科技", "成人", "少妇", "萝莉", "淫", "少女", "片", "官网", "www", "WWW", "xyz", "top", "cn", "QQ", "qq", "激情"};
	for (int i = 0; i < 25; i++)
	{
		if (strstr(message.c_str(), words[i]) != NULL) 
			return true;
	}
	return false;
}

class CEventNetWorkTextMessageReceived
{
public:
    char pad_0000[24]; //0x0000
    char m_info[8]; //0x0018
    char pad_0020[36]; //0x0020
}; //Size: 0x0044
using event_network_text_message_received_t = char __fastcall (CEventNetWorkTextMessageReceived* a1, DWORD64* a2, int a3);
typedef __int64(__cdecl* get_chat_data_t)(__int64 a1, __int64 a2, __int64 a3, const char* receivetext, BOOL a5);
event_network_text_message_received_t* m_event_network_text_message_received{};
get_chat_data_t m_get_chat_data{};

get_chat_data_t og_get_chat_data = nullptr;
__int64 __cdecl hk_get_chat_data(__int64 a1, __int64 a2, __int64 a3, const char* receivetext, BOOL a5)
{
	bool isspam = IsSpam(receivetext);
    if (isspam)
        return 0;

	return og_get_chat_data(a1, a2, a3, receivetext, a5);
}

event_network_text_message_received_t* og_event_network_text_message_received = nullptr;
bool hk_event_network_text_message_received(CEventNetWorkTextMessageReceived* a1, DWORD64* a2, int a3)
{
    bool isspam = IsSpam(a1->m_info);
    if (isspam)
        return false;
    return og_event_network_text_message_received(a1, a2, a3);
}

DWORD Mainthread(LPVOID lp)
{
    MH_Initialize();

    pattern_batch main_batch;
    main_batch.add("EventNetWorkTextMessageReceived", "48 83 EC 28 4C 8B CA 48 85 D2 0F 84 ? ? ? ? 41 BA ? ? ? ? 45 3B C2 0F 85 ? ? ? ? 48 8D 51 18 48 8B C2 49 0B C1 83 E0 0F 0F 85 ? ? ? ? B8 ? ? ? ? 8D 48 7F 0F 28 02 41 0F 29 01 0F 28 4A 10 41 0F 29 49 ? 0F 28 42 20 41 0F 29 41 ? 0F 28 4A 30 41 0F 29 49 ? 0F 28 42 40 41 0F 29 41 ? 0F 28 4A 50 41 0F 29 49 ? 0F 28 42 60 41 0F 29 41 ? 0F 28 4A 70 4C 03 C9 48 03 D1 41 0F 29 49 ? 48 FF C8 75 AF 0F 28 02 41 0F 29 01 0F 28 4A 10 41 0F 29 49 ? 0F 28 42 20 41 0F 29 41 ? 0F 28 4A 30 41 0F 29 49 ? 0F 28 42 40 41 0F 29 41 ? 0F 28 4A 50 41 0F 29 49 ? 48 8B 42 60", [=](ptr_manage ptr)
    {
        m_event_network_text_message_received = ptr.as<event_network_text_message_received_t*>();
    });
    main_batch.add("get_chat_data", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30 49 8B F8 44 8B 81 ? ? ? ?", [=](ptr_manage ptr)
    {
        m_get_chat_data = ptr.as<get_chat_data_t>();
    });
    main_batch.run();
    MH_CreateHook(m_get_chat_data, hk_get_chat_data, (LPVOID*)&og_get_chat_data);
    MH_CreateHook(m_event_network_text_message_received, hk_event_network_text_message_received, (LPVOID*)&og_event_network_text_message_received);
    MH_EnableHook(MH_ALL_HOOKS);
    while (g_running)
    {

    }
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, NULL, static_cast<LPTHREAD_START_ROUTINE>(Mainthread), hModule, NULL, NULL);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

