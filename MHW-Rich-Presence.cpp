#include <iostream>
#include "discord/discord.h"
#include <Windows.h>
#include <memory.h>
#include <TlHelp32.h>

discord::Core* core{};

int waittime = 0;
int loopnumber = 0;
int baseAddress;

int hunterrank = 0;

LPCWSTR process_name = L"MonsterHunterWorld.exe";
HANDLE mhw_handle = NULL;
boolean checking = true;

DWORD BASE_ADDRESS = { 0x1029258C };

using namespace discord;


void InitializeDiscord()
{
	auto result = discord::Core::Create(666709854622187570, DiscordCreateFlags_Default, &core);
	discord::Activity activity{};
	activity.SetState("Test");
	activity.SetDetails("This is a prototype");
	core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
		// TODO;
		});
}

/* Locate a process from the name. */
DWORD FindProcessId(const std::wstring& processName)
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);

	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	Process32First(processesSnapshot, &processInfo);
	if (!processName.compare(processInfo.szExeFile))
	{
		CloseHandle(processesSnapshot);
		return processInfo.th32ProcessID;
	}

	while (Process32Next(processesSnapshot, &processInfo))
	{
		if (!processName.compare(processInfo.szExeFile))
		{
			CloseHandle(processesSnapshot);
			return processInfo.th32ProcessID;
		}
	}

	CloseHandle(processesSnapshot);
	return 0;
}

void Hook()
{
	checking = false;
	mhw_handle = OpenProcess(PROCESS_ALL_ACCESS, true, FindProcessId(process_name));

	if (mhw_handle == NULL) {
		std::cout << "Failed to hook onto " << process_name << "!";
		return;
	}
	else
		std::cout << "Successfully hooked onto " << process_name << "!";
}


bool IsMHWRunning()
{
	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, true, FindProcessId(process_name));
	return processHandle != NULL;
}

void AttemptHook()
{
	std::cout << "Attempting to hook onto " << process_name << "...";

	if (IsMHWRunning() == false) {
		std::cout << "Failed to hook onto " << process_name << "! Waiting for process...";

		while (checking) {
			waittime++;

			if (waittime >= 10000000) {
				waittime = 0;

				if (IsMHWRunning() == true)
					Hook();
			}
		}
	}
	else {
		Hook();
	}
}

void ReadMemory()
{
	// TODO;
}

void application_loop()
{
	loopnumber++;
	ReadMemory();
	std::cout << "Application Loop " << loopnumber << "\n";
	::core->RunCallbacks();
}

int main()
{
	InitializeDiscord();
	AttemptHook();
	
	while (true) {
		waittime++;

		if (waittime >= 2000000000) {
			waittime = 0;
			application_loop();
		}
	}
}