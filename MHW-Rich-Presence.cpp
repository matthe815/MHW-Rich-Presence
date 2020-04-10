#include <iostream>
#include "discord/discord.h"
#include <Windows.h>
#include <memory.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <string>
#include "QuestData.h"
#include "Player.h"
#include <clocale>
#include "rapidjson/document.h"
#include <fstream>
#include "rapidjson/filereadstream.h"

///
// Discord.
///
discord::Core* core{};

///
// Application management.
///
int waittime = 0
, updatetimer = 0
, loopnumber = 0
, process_id;

///
// Storage for retrieved memory values.
///
QuestData quest{};
Player player{};

///
// A list of information.
///
std::string questNames[512];

///
// Process information
///
LPCWSTR process_name = L"MonsterHunterWorld.exe";
HANDLE mhw_handle = NULL;
boolean checking = true;
boolean last_quest_status = false;
boolean quest_status = false;

boolean last_guided_lands_status = false;
boolean in_guided_lands = false;
int last_map = 0;
int current_map = 0;

///
// Application settings.
///
std::string version = "0.9.1";
std::string language = "English";
std::string language_quests = "English";
rapidjson::Document languageData;
int saveSlot = 1;
bool hideSelf = false;
bool autoClose = false;

///
// Pointer information
///
long long MHW_PTR = 0x140000000 + 0x04EA20A8
, START_INDEX = 0x10000080
, END_INDEX = 0xFFFF0080
, QUEST_START_INDEX = 0x10000F20
, QUEST_END_INDEX = 0xF0000000
, BASE_ADDRESS = 0
, QUEST_ADDRESS = 0;

LPCVOID NEW_EQUIPMENT_ADDRESS = 0;
LPCVOID LEVEL_ADDRESS = 0;

///
// Static addresses
///
long long MHW_BASE_ADDRESS = 0x140000000
, EQUIPMENT_OFFSET = 0x04ECB860
, LEVEL_OFFSET = 0x04F29FF0;

boolean displayName = false; // Whether or not to display the player's name.

///
// This generates a new core to display Rich Presence for use during the application's runtime.
///
void InitializeDiscord()
{
	discord::Core::Create(666709854622187570, DiscordCreateFlags_Default, &core);
}

///
// Get if the player is actually in the guided lands.
///
void IsInGuidedLands()
{
	int monster1;

	ReadProcessMemory(mhw_handle, (LPCVOID)(BASE_ADDRESS + 0x27B9F6), &monster1, sizeof(monster1), NULL);

	last_guided_lands_status = in_guided_lands;
	in_guided_lands = monster1 != 0;
}

void GetTrueMapID()
{
	last_map = current_map;
	ReadProcessMemory(mhw_handle, (LPCVOID)((long long)LEVEL_ADDRESS + 0xAED0), &current_map, 2, NULL);
}

///
// Obtain the guided lands levels.
///
void GetGuidedLevels()
{
	int forest = 0,
		wildspire = 0,
		coral = 0,
		rotten = 0,
		elder = 0,
		tundra = 0;

	ReadProcessMemory(mhw_handle, (LPCVOID)(BASE_ADDRESS + 0x27B928), &forest, 4, NULL);
	ReadProcessMemory(mhw_handle, (LPCVOID)(BASE_ADDRESS + 0x27B92C), &wildspire, 4, NULL);
	ReadProcessMemory(mhw_handle, (LPCVOID)(BASE_ADDRESS + 0x27B930), &coral, 4, NULL);
	ReadProcessMemory(mhw_handle, (LPCVOID)(BASE_ADDRESS + 0x27B934), &rotten, 4, NULL);
	ReadProcessMemory(mhw_handle, (LPCVOID)(BASE_ADDRESS + 0x27B938), &elder, 4, NULL);
	ReadProcessMemory(mhw_handle, (LPCVOID)(BASE_ADDRESS + 0x27B93C), &tundra, 4, NULL);

	player.set_guided_lands(forest > 10000 ? (forest / 10000) + 1 : 1, wildspire > 10000 ? (wildspire / 10000) + 1 : 1, coral > 10000 ? (coral / 10000) + 1 : 1, rotten > 10000 ? (rotten / 10000) + 1 : 1, elder > 10000 ? (elder / 10000) + 1 : 1, tundra > 10000 ? (tundra / 10000) + 1 : 1);
}

///
// This sends an update tick to the Discord RPC (Must be run after InitializeDiscord()).
///
void UpdateDiscord()
{
	if (player.get_last_hunter_rank() == player.get_hunter_rank() && player.get_last_master_rank() == player.get_master_rank()
		&& quest.get_id() == quest.get_last_id() && last_quest_status == quest_status && last_guided_lands_status == in_guided_lands && last_map == current_map) // Stop if there's identical data.
		return;

	std::cout << player.get_name() << " -- HR/MR " << player.get_hunter_rank() << "/" << player.get_master_rank() << std::endl;

	discord::Activity activity{}; // A blank object to send to the Discord RPC.

	///
	// Generate and format strings for use in the Rich Presence.
	///
	std::string details = (current_map != 301 && current_map != 302 && current_map != 303 && current_map != 304 && current_map != 305 & current_map != 306 ? languageData["IN_QUEST"].GetString() : languageData["IN_HUB"].GetString());
	std::string state = (displayName ? player.get_name() + " -- " : "") + "HR/MR: " + std::to_string((int)player.get_hunter_rank()) + "/" + std::to_string((int)player.get_master_rank());
	std::string map = "map_" + std::to_string(current_map);

	///
	// Apply image assets.
	///
	activity.GetAssets().SetLargeImage(map.c_str());
	activity.GetAssets().SetLargeText(languageData[map.c_str()].GetString());
	activity.GetAssets().SetSmallImage(current_map != 301 && current_map != 302 && current_map != 303 && current_map != 304 && current_map != 305 & current_map != 306 ? "quest" : "");

	///
	// Display special info in the guided lands.
	///
	if (current_map == 109) {
		details = languageData["EXPLORING"].GetString() + std::to_string(player.get_forest_level()) + "/" + std::to_string(player.get_wildspire_level()) + "/" + std::to_string(player.get_coral_level()) +
			"/" + std::to_string(player.get_rotten_level()) + "/" + std::to_string(player.get_elder_level()) + "/" + std::to_string(player.get_tundra_level());

		activity.GetAssets().SetSmallImage("quest");
	}
	else if (current_map == 0) {
		details = languageData["MAIN_MENU"].GetString();
		state = "";
		activity.GetAssets().SetSmallImage("");
	}

	///
	// Apply the state and details to the activity object.
	///
	activity.SetDetails(details.c_str());
	activity.SetState(state.c_str());

	core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {}); // Update the Discord status.
}

///
// Locate a process from a provided process name. Will return 0 in the case that there is no process by that name or if it fails.
///
DWORD FindProcessId(const std::wstring& processName)
{
	///
	// Create a ProcessEntry, figure out its' size, and make a new snapshot.
	///
	PROCESSENTRY32 processInfo;
	HANDLE         processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	processInfo.dwSize = sizeof(processInfo);

	if (processesSnapshot == INVALID_HANDLE_VALUE) // If the handle represented by the snapshot tool is invalid, then fail.
		return 0;

	///
	// Obtain the first process visible to the snapshot, and check if the requested name is the same as the file, return its' ID if so.
	///
	Process32First(processesSnapshot, &processInfo);
	if (!processName.compare(processInfo.szExeFile)) {
		CloseHandle(processesSnapshot);
		return processInfo.th32ProcessID;
	}

	///
	// Check every other process for the one with the requested name until it finds it or runs out of processes.
	///
	while (Process32Next(processesSnapshot, &processInfo)) {
		if (!processName.compare(processInfo.szExeFile)) {
			CloseHandle(processesSnapshot);
			process_id = processInfo.th32ProcessID;
			return processInfo.th32ProcessID;
		}
	}

	CloseHandle(processesSnapshot); // IMPORTANT, keeps from memory leakage.
	return 0;
}

///
// Obtain the base memory address for Monster Hunter World.
///
DWORD_PTR GetProcessBaseAddress()
{
	MODULEENTRY32 ModuleEntry = { 0 };
	HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process_id);

	if (!SnapShot)
		return NULL;

	// You have to initialize the size, otherwise it will not work
	ModuleEntry.dwSize = sizeof(ModuleEntry);

	if (!Module32First(SnapShot, &ModuleEntry))
		return NULL;

	do {
		// Check if the module name matches the one we're looking for
		if (!wcscmp(ModuleEntry.szModule, process_name)) {
			CloseHandle(SnapShot);
			return (DWORD_PTR)ModuleEntry.modBaseAddr;
		}
	} while (Module32Next(SnapShot, &ModuleEntry));

	CloseHandle(SnapShot);
	return NULL;
}

///
// Hook onto the Monster Hunter: World process if possible.
///
void Hook()
{
	mhw_handle = OpenProcess(PROCESS_ALL_ACCESS, true, FindProcessId(process_name));
	checking = false; // Tell the system that it's not searching anymore.

	if (hideSelf == true)
		ShowWindow(GetConsoleWindow(), SW_SHOW);

	std::cout << (mhw_handle == NULL ? languageData["FAILED_HOOK"].GetString() : languageData["SUCCESSFUL_HOOK"].GetString()) << process_name << "!" << std::endl;
}

///
// Return whether or not Monster Hunter: World is ACTUALLY RUNNING.
///
bool IsMHWRunning()
{
	return OpenProcess(PROCESS_ALL_ACCESS, true, FindProcessId(process_name)) != NULL;
}

///
// Make a valiant effort to hook onto Monster Hunter: World, and retry every 10 seconds until it hooks.
///
void AttemptHook()
{
	if (hideSelf == true)
		ShowWindow(GetConsoleWindow(), SW_HIDE);

	std::cout << languageData["HOOK_ATTEMPT"].GetString() << process_name << std::endl;

	if (IsMHWRunning() == false) {
		std::cout << languageData["WAITING_FOR_HOOK"].GetString() << process_name << std::endl;

		while (checking) {
			Sleep(1000);

			if (IsMHWRunning() == true)
				Hook();
		}
	}
	else
		Hook();
}

///
// Read the memory of the current player, and store the information within the player object.
///
void ReadMemory()
{
	int hunter_rank,
		master_rank,
		session_duration = 0,
		quest_id = 0,
		map_id = 0;

	long long current_quest = 0;

	char hunter_name[20];

	ReadProcessMemory(mhw_handle, (LPCVOID)(BASE_ADDRESS + 0x90), &hunter_rank, sizeof(hunter_rank), NULL); // Obtain memory value for HR.
	ReadProcessMemory(mhw_handle, (LPCVOID)(BASE_ADDRESS + 0x50), &hunter_name, sizeof(hunter_name), NULL); // Obtain memory value for name.
	ReadProcessMemory(mhw_handle, (LPCVOID)(BASE_ADDRESS + 0xD4), &master_rank, sizeof(master_rank), NULL); // Obtain memory value for MR.

	GetTrueMapID();

	if (current_map == 109) {
		GetGuidedLevels();
	}
	else if (QUEST_ADDRESS != 0) {
		ReadProcessMemory(mhw_handle, (LPCVOID)(QUEST_ADDRESS - 0xE8), &quest_id, 2, NULL); // Obtain the quest ID.
		ReadProcessMemory(mhw_handle, (LPCVOID)(QUEST_ADDRESS - 0x168), &map_id, 2, NULL); // Obtain the map ID.

		quest.set_data(quest_id, map_id);
	}

	player.set_data(hunter_name != NULL ? hunter_name : "Cross", hunter_rank, master_rank, session_duration, current_quest != 0);
}

///
// Find the player memory address.
///
void FindPlayerIndex()
{
	std::cout << "Save Slot: " << saveSlot << std::endl;

	for (long long address = START_INDEX; address < END_INDEX; address += 0x1000) {
		int byteArray = 0;
		ReadProcessMemory(mhw_handle, (LPCVOID)address, &byteArray, sizeof(byteArray), NULL);

		// Base check
		if (byteArray == 1125683608) {
			int byteArray2 = 0;
			ReadProcessMemory(mhw_handle, (LPCVOID)(address + 0x1), &byteArray2, sizeof(byteArray2), NULL);

			if (byteArray2 == 1)
				continue;

			BASE_ADDRESS = address;

			if (saveSlot == 2)
				BASE_ADDRESS += 0x27E9F0;
			else if (saveSlot == 3)
				BASE_ADDRESS += 0x4FD3E0;

			std::cout << languageData["PLAYER_ADDRESS_FOUND"].GetString() << std::endl;
			std::cout << languageData["PLAYER_ADDRESS_LOCATION"].GetString() << (LPCVOID)BASE_ADDRESS << std::endl;
			std::cout << languageData["PLAYER_ADDRESS_VALUE"].GetString() << byteArray << std::endl;
			break;
		}
	}

	if (BASE_ADDRESS == 0)
		std::cout << languageData["PLAYER_ADDRESS_NOT_FOUND"].GetString() << std::endl;
}

///
// Find the header for quests.
///
void FindQuestIndex()
{
	for (long long address = QUEST_START_INDEX; address < QUEST_END_INDEX; address += 0x1000) {
		int byteArray = 0;
		ReadProcessMemory(mhw_handle, (LPCVOID)address, &byteArray, sizeof(byteArray), NULL);

		// Base check
		if (byteArray == 1125167688) {
			QUEST_ADDRESS = address;

			std::cout << languageData["QUEST_ADDRESS_FOUND"].GetString() << std::endl;
			std::cout << languageData["QUEST_ADDRESS_LOCATION"].GetString() << (LPCVOID)QUEST_ADDRESS << std::endl;
			std::cout << languageData["QUEST_ADDRESS_VALUE"].GetString() << byteArray << std::endl;
			break;
		}
	}

	if (QUEST_ADDRESS == 0)
		std::cout << languageData["QUEST_ADDRESS_NOT_FOUND"].GetString() << std::endl;
}

boolean IsStillRunning()
{
	DWORD status = WaitForSingleObject(mhw_handle, 1);

	if (status == WAIT_OBJECT_0) // It's closed
		return false;
	else
		return true;
}

///
// This is a loop that fires every couple seconds.
///
void ApplicationLoop()
{
	loopnumber++;

	if (!IsStillRunning()) {
		BASE_ADDRESS = 0;
		QUEST_ADDRESS = 0;
		NEW_EQUIPMENT_ADDRESS = 0;
		LEVEL_ADDRESS = 0;
		checking = true;

		if (autoClose == true) { // Close self if requested.
			CloseWindow(GetConsoleWindow());
			exit(0);
		}

		AttemptHook();
		return;
	}

	ReadMemory();
	::core->RunCallbacks();
}

///
// Load the user's config.
///
void ReadConfig()
{
	std::cout << "Loading config..." << std::endl;
	FILE* fp = fopen("config.json", "rb");

	char readBuffer[65536];
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

	rapidjson::Document d;
	d.ParseStream(is);

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
void LoadLanguage()
{
	std::cout << "Loading language..." << std::endl;

	FILE* fp = fopen(language.c_str(), "rb");

	char readBuffer[65536];
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	languageData.ParseStream(is);
	fclose(fp);
	std::setlocale(LC_ALL, languageData["locale"].GetString());

	std::cout << language + " loaded!" << std::endl;
}

///
// Load the quest list.
///
void ReadQuests()
{
	std::cout << "Loading quests..." << std::endl;

	FILE* fp = fopen(language_quests.c_str(), "rb");

	char readBuffer[65536];
	rapidjson::Document d;
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	d.ParseStream(is);
	fclose(fp);

	std::cout << "Quests loaded!" << std::endl;
}

///
// Read a static address.
///
LPCVOID ReadStaticAddress(LPCVOID address)
{
	LPCVOID address2;
	ReadProcessMemory(mhw_handle, address, &address2, 16, NULL);
	return address2;
}

///
// Initialize Discord, attempt to hook the game, and begin the application loop.
///
int main()
{
	ReadConfig();
	LoadLanguage();
	ReadQuests();

	std::cout << "----------------------------------------" << std::endl;

	std::cout << "" << std::endl;
	std::cout << languageData["APP_DATA"].GetString() << version << std::endl;

	if (hideSelf == true) // Only display the message if the config option is enabled.
		std::cout << languageData["CLOSE_DISCLAIMER"].GetString() << std::endl;

	Sleep(5000);
	std::cout << "" << std::endl;

	std::cout << "-----------------------------------------" << std::endl;
	std::cout << "" << std::endl;

	InitializeDiscord();
	AttemptHook();

	LEVEL_ADDRESS = ReadStaticAddress((LPCVOID)(MHW_BASE_ADDRESS + LEVEL_OFFSET));

	FindPlayerIndex();

	while (true) {
		Sleep(2000);

		if (checking == false) {
			if (BASE_ADDRESS == 0)
				FindPlayerIndex();
			if (QUEST_ADDRESS == 0)
			//	FindQuestIndex();
			if (LEVEL_ADDRESS == 0)
				LEVEL_ADDRESS = ReadStaticAddress((LPCVOID)(MHW_BASE_ADDRESS + LEVEL_OFFSET));

			ApplicationLoop();
			UpdateDiscord();
		}
	}
}