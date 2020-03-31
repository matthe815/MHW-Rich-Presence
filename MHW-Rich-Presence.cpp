#include <iostream>
#include "discord/discord.h"
#include <Windows.h>
#include <memory.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <string>
#include "QuestData.h"
#include "Player.h"

discord::Core* core{};
std::unordered_map<int, std::string> questNames;
std::unordered_map<int, std::string> areaNames;

///
// Application tick management.
///
int waittime = 0;
int updatetimer = 0;
int loopnumber = 0;

///
// Storage for retrieved memory values.
///
QuestData quest{};
Player player{};

///
// Process information
///
LPCWSTR process_name = L"MonsterHunterWorld.exe";
int process_id;
HANDLE mhw_handle = NULL;
boolean checking = true;

///
// Pointer information
///
long long MHW_PTR = 0x140000000 + 0x04EA20A8;

long long SESSION_TIME_OFFSET = 0x7FC2CA24;

long long START_INDEX = 0xA4100080;
long long END_INDEX   = 0xA7100000;

long long BASE_ADDRESS;


///
// This generates a new core to display Rich Presence for use during the application's runtime.
///
void InitializeDiscord()
{
	discord::Core::Create(666709854622187570, DiscordCreateFlags_Default, &core);
}

///
// This sends an update tick to the Discord RPC (Must be run after InitializeDiscord()).
///
void UpdateDiscord()
{
	std::cout << "Updating Discord" << std::endl;
	std::cout << "Is in quest: " << player.is_in_quest();

	discord::Activity activity{}; // A blank object to send to the Discord RPC.

	///
	// Generate and format strings for use in the Rich Presence.
	///
	std::string details = (player.is_in_session() ? "ONLINE" : "OFFLINE") + (std::string)" -- " + (player.is_in_quest() == true ? "In Quest" : (std::string)"Chillin' in the Gathering Hub");
	std::string state = player.get_name() + " -- HR: " + std::to_string((int)player.get_hunter_rank());
	std::string map = "";

	///
	// Apply the necessary assets based on the player's quest status.
	///
	if (player.is_in_quest() == TRUE) {
		map = "map_" + std::to_string(quest.get_map_id());

		activity.GetAssets().SetSmallImage("quest");
		activity.GetAssets().SetSmallText(questNames[quest.get_id()].c_str());
	}

	activity.GetAssets().SetLargeImage(map != "" ? map.c_str() : "astera");
	activity.GetAssets().SetLargeText(map != "" ? areaNames[quest.get_map_id()].c_str() : "In Astera");

	///
	// Apply the state and details to the activity object.
	///
	activity.SetDetails(details.c_str());
	activity.SetState(state.c_str());

	
	core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {});
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
	// This structure contains lots of goodies about a module
	MODULEENTRY32 ModuleEntry = { 0 };
	// Grab a snapshot of all the modules in the specified process
	HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process_id);

	if (!SnapShot)
		return NULL;

	// You have to initialize the size, otherwise it will not work
	ModuleEntry.dwSize = sizeof(ModuleEntry);

	// Get the first module in the process
	if (!Module32First(SnapShot, &ModuleEntry))
		return NULL;

	do {
		// Check if the module name matches the one we're looking for
		if (!wcscmp(ModuleEntry.szModule, process_name)) {
			// If it does, close the snapshot handle and return the base address
			CloseHandle(SnapShot);
			return (DWORD_PTR)ModuleEntry.modBaseAddr;
		}
		// Grab the next module in the snapshot
	} while (Module32Next(SnapShot, &ModuleEntry));

	// We couldn't find the specified module, so return NULL
	CloseHandle(SnapShot);
	return NULL;
}

///
// Hook onto the Monster Hunter: World process if possible.
///
void Hook()
{
	checking = false;
	mhw_handle = OpenProcess(PROCESS_ALL_ACCESS, true, FindProcessId(process_name));

	if (mhw_handle == NULL)
		std::cout << "Failed to hook onto " << process_name << "!" << std::endl;
	else
		std::cout << "Successfully hooked onto " << process_name << "!" << std::endl;

	return;
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
	std::cout << "Attempting to hook onto " << process_name << "..." << std::endl;

	if (IsMHWRunning() == false) {
		std::cout << "Failed to hook onto " << process_name << "! Waiting for process..." << std::endl;

		while (checking) {
			waittime++;

			if (waittime >= 10000000) {
				waittime = 0;

				if (IsMHWRunning() == true)
					Hook();
			}
		}
	}
	else
		Hook();
}

///
// Read the memory of the player's selected quest.
///
void ReadQuestMemory()
{
	int id,
		map_id;

	long long quest_memory_address;

}

///
// Read the memory of the current player, and store the information within the player object.
///
void ReadMemory()
{
	int hunter_rank,
		  session_duration = 0;

	long long current_quest = 0;

	char hunter_name[20];


	long long value;

	ReadProcessMemory(mhw_handle, (LPCVOID)(0x70B69FE0+90), &value, sizeof(value), NULL);

	std::cout << value << std::endl;

	ReadProcessMemory(mhw_handle, (LPCVOID)(BASE_ADDRESS+0x90), &hunter_rank, sizeof(hunter_rank), NULL);
	ReadProcessMemory(mhw_handle, (LPCVOID)(BASE_ADDRESS+0x50), &hunter_name, sizeof(hunter_name), NULL);

	player.set_data(hunter_name != NULL ? hunter_name : "Cross", hunter_rank, session_duration, current_quest != 0);
	std::cout << player.get_name() << " -- HR " << player.get_hunter_rank() << " >> " << "Quest: " << current_quest << " Last Session Ping/Current: " << player.get_last_session_time() << "/" << player.get_session_time() << std::endl;

	if (current_quest != 0 && current_quest != 1592474848 && current_quest != 3052339328)
		ReadQuestMemory();
}

///
// Find the player memory address.
///
void FindPlayerIndex()
{
	for (long long address = START_INDEX; address < END_INDEX; address+=0x1000) {
		int byteArray = 0;
		ReadProcessMemory(mhw_handle, (LPCVOID)address, &byteArray, sizeof(byteArray), NULL);

		if (byteArray == 1125346736) {
			BASE_ADDRESS = address;
			std::cout << "Found HR memory address" << std::endl;
			break;
		}
	}
}

///
// This is a loop that fires every couple seconds.
///
void application_loop()
{
	loopnumber++;
	std::cout << "Application Loop " << loopnumber << "\n" << std::endl;

	ReadMemory();
	::core->RunCallbacks();
}

///
// Initialize Discord, attempt to hook the game, and begin the application loop.
///
int main()
{
	InitializeDiscord();
	AttemptHook();
	FindPlayerIndex();
	
	questNames[252] = "Camp Crasher";
	questNames[261] = "Snatch The Snatcher";

	areaNames[101] = "Ancient Forest";
	areaNames[102] = "Wildspire Waste";

	while (true) {
		waittime++;

		if (waittime >= 2000000000) {
			waittime = 0;
			application_loop();
			UpdateDiscord();
		}
	}
}