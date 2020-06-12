#include <iostream>
#include <Windows.h>
#include <string>
#include <TlHelp32.h>
#include "discord/discord.h"
#include "rapidjson/document.h"
#include <clocale>
#include "rapidjson/filereadstream.h"

#define BUFFER_SIZE 100024
#define TICK_WAIT 5000
#define BASE_ADDRESS 0x140000000
#define APPLICATION_VERSION 1.0

/*
	Structures
*/
#include "Structs/Player.h"

HANDLE handle;

/*
	Memory addresses
*/
long long STATIC_POINTERS[] {0x0, 0x0, 0x0};
long long ADDRESS_LIST[] = { 0x04F53580 /* Level offset */, 0x05103AF8 /* Party offset */};
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

/*
	Quickly locate the process id of MHW by process name.
*/
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

/*
	Just hook.
*/
auto hook() -> void {
	handle = OpenProcess(PROCESS_ALL_ACCESS, true, find_process_id(process_name));
	std::cout << (handle == NULL ? "Failed to hook onto " : "Successfully hooked onto ") << process_name << "!" << std::endl;
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

///
// Read a static address.
///
auto read_static_address(LPCVOID address) -> LPCVOID {
	LPCVOID address2;
	ReadProcessMemory(handle, address, &address2, 16, NULL);
	return address2;
}

///
// Default debug information.
///
auto write_initial_debug_data() -> void {
	std::cout << "----------------------------------------" << std::endl;
	std::cout << languageData["APP_DATA"].GetString() << APPLICATION_VERSION << std::endl;
	
	if (hideSelf) // Show a hideSelf disclaimer if the config option is enabled.
		std::cout << languageData["CLOSE_DISCLAIMER"].GetString() << std::endl;

	std::cout << "----------------------------------------" << std::endl;
	Sleep(TICK_WAIT);
}

///
// Find crucial memory addresses.
///
auto find_indexes() -> void {
	STATIC_POINTERS[1] = (long long)read_static_address((LPCVOID)ADDRESS_LIST[0]); // Set the level offset static address.

	std::cout << "Selected Save Slot: #" << saveSlot << std::endl;

	for (long long address = 0x10000080; address < 0xFFFF0080; address += 0x1000) {
		int byteArray = 0;
		ReadProcessMemory(handle, (LPCVOID)address, &byteArray, sizeof(byteArray), NULL);

		// Base check
		if (byteArray == 1125815816) {
			int byteArray2 = 0;
			ReadProcessMemory(handle, (LPCVOID)(address + 0x1), &byteArray2, sizeof(byteArray2), NULL);

			if (byteArray2 == 1)
				continue;

			STATIC_POINTERS[0] = address;

			if (saveSlot == 2)
				STATIC_POINTERS[0] += 0x27E9F0;
			else if (saveSlot == 3)
				STATIC_POINTERS[0] += 0x4FD3E0;

			std::cout << languageData["PLAYER_ADDRESS_FOUND"].GetString() << std::endl;
			std::cout << languageData["PLAYER_ADDRESS_LOCATION"].GetString() << (LPCVOID)STATIC_POINTERS[0] << std::endl;
			std::cout << languageData["PLAYER_ADDRESS_VALUE"].GetString() << byteArray << std::endl;
			break;
		}
	}

	if (STATIC_POINTERS[0] == 0x0)
		std::cout << languageData["PLAYER_ADDRESS_NOT_FOUND"].GetString() << std::endl;
}

auto main() -> int {
	/*
		Load settings.
	*/
	load_config();
	load_language();

	write_initial_debug_data();
	initialize_discord();
	hook();

	while (true) {
		if (handle == NULL) {
			hook();
			continue;
		}

		if (STATIC_POINTERS[0] == 0x0||STATIC_POINTERS[1] == 0x0)
			find_indexes();

		/*
			Perform an application tick.
		*/
		process_tick();
		Sleep(TICK_WAIT);
	}

	return 0;
}