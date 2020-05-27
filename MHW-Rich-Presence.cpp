#include <iostream>
#include <Windows.h>
#include <string>
#include <TlHelp32.h>
#include "discord/discord.h"
#include "rapidjson/document.h"
#include <clocale>
#include "rapidjson/filereadstream.h"
#define BUFFER_SIZE 100024

/*
	Structures
*/
#include "Structs/Player.h"

int tickWait = 5000;
HANDLE handle;

/*
	Memory addresses
*/
long long BASE_ADDRESS = 0x140000000;
long long ADDRESS_LIST[] = { 0x04F4FA60, 0x05063D40 };
long long PLAYER_OFFSETS[] = { 0x90, 0x50, 0xD4 };

/*
	Application settings.
*/
bool displayName = true, hideSelf = false, autoClose = false;
std::string version = "0.9.5", language = "English", language_quests = "English";
rapidjson::Document languageData;
int saveSlot = 1;

discord::Core* core{};
Player main_player;

LPCWSTR process_name = L"MonsterHunterWorld.exe";

auto find_process_id(const std::wstring& processName) -> DWORD {
	PROCESSENTRY32 processInfo;
	HANDLE         processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	processInfo.dwSize = sizeof(processInfo);

	if (processesSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	Process32First(processesSnapshot, &processInfo);
	if (!processName.compare(processInfo.szExeFile)) {
		CloseHandle(processesSnapshot);
		return processInfo.th32ProcessID;
	}

	while (Process32Next(processesSnapshot, &processInfo)) {
		if (!processName.compare(processInfo.szExeFile)) {
			CloseHandle(processesSnapshot);
			return processInfo.th32ProcessID;
		}
	}

	CloseHandle(processesSnapshot);
	return 0;
}

auto hook() -> HANDLE {
	HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, true, find_process_id(process_name));
	return h;
}

/*
	Create a new rich presence instance for Discord.
	This needs to be run before update_discord.
*/
void initialize_discord()
{
	discord::Core::Create(666709854622187570, DiscordCreateFlags_Default, &core);
}

/*
	Send an update tick to Discord.
*/
void update_discord(Player player)
{
	if (main_player.is_different(player))
		main_player = player;

	discord::Activity activity{};

	core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {});
}

/*
	Obtain and return all information about a player through memory scanning.
*/
auto get_player(const HANDLE& handle) -> Player {
	char hunter_name[20];
	int hunter_rank, master_rank;

	ReadProcessMemory(handle, (LPCVOID)(BASE_ADDRESS + PLAYER_OFFSETS[1]), &hunter_name, sizeof(hunter_name), NULL); // Obtain memory value for name.
	ReadProcessMemory(handle, (LPCVOID)(BASE_ADDRESS + PLAYER_OFFSETS[0]), &hunter_rank, sizeof(hunter_rank), NULL); // Obtain memory value for HR.
	ReadProcessMemory(handle, (LPCVOID)(BASE_ADDRESS + PLAYER_OFFSETS[2]), &master_rank, sizeof(master_rank), NULL); // Obtain memory value for MR.

	Player player;

	player.set_name((std::string)hunter_name);
	player.set_rank(hunter_rank, master_rank);

	return player;
}

void process_tick()
{
	Player player = get_player(handle);
	update_discord(player);
}

void load_config()
{
	std::cout << "Loading config..." << std::endl;
	FILE* fp = fopen("config.json", "rb");

	char* readBuffer = (char*)malloc(BUFFER_SIZE);
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

	rapidjson::Document d;
	d.ParseStream(is);
	free(readBuffer);
	fclose(fp);

	displayName = d["displayName"].GetBool();
	language = d["language"].GetString();
	saveSlot = d["saveSlot"].GetInt();
	language_quests = d["quests"].GetString();
	autoClose = d["autoClose"].GetBool();
	hideSelf = d["hideSelfOnClose"].GetBool();

	std::cout << "Config loaded!" << std::endl;
}

///
// Load the selected language.
///
void load_language()
{
	std::cout << "Loading language..." << std::endl;

	FILE* fp = fopen(language.c_str(), "rb");

	char* readBuffer = (char*) malloc(BUFFER_SIZE);
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	languageData.ParseStream(is);
	free(readBuffer);
	fclose(fp);
	std::setlocale(LC_ALL, languageData["locale"].GetString());
	std::cout << language + " loaded!" << std::endl;
}

auto main() -> int {
	/*
		Load settings.
	*/
	load_config();
	load_language();

	initialize_discord();
	handle = hook();

	while (true) {
		if (handle == NULL) {
			handle = hook();
			continue;
		}

		/*
			Perform an application tick.
		*/
		process_tick();
		Sleep(tickWait);
	}

	return 0;
}