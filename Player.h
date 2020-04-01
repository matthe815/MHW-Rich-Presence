#pragma once
#include <string>

//
// The basic player data is housed within this.
//
class Player
{
	std::string hunter_name;

	int hunter_rank = 0,
		last_hunter_rank = 0,
		master_rank = 0,
		last_master_rank = 0;

	float session_time = 0,
		last_session_time = 0;

	bool in_quest = 0,
		last_in_quest = 0;

	bool last_in_session = 0;

public:
	void set_data(std::string name, int hr, int mr, float current_session, bool is_quest)
	{
		hunter_name = name;

		last_hunter_rank = get_hunter_rank();
		hunter_rank = hr;

		last_master_rank = get_master_rank();
		master_rank = mr;

		last_in_session = is_in_session();

		last_session_time = get_session_time();
		session_time = current_session;

		last_in_quest = is_quest;
		in_quest = is_quest;
	}

	std::string get_name()
	{
		return hunter_name;
	}

	int get_last_hunter_rank()
	{
		return last_hunter_rank;
	}

	int get_last_master_rank()
	{
		return last_master_rank;
	}

	float get_last_session_time()
	{
		return last_session_time;
	}

	int get_hunter_rank()
	{
		return hunter_rank;
	}

	int get_master_rank()
	{
		return master_rank;
	}

	float get_session_time()
	{
		return session_time;
	}

	bool get_last_in_quest()
	{
		return last_in_quest;
	}

	bool get_last_in_session()
	{
		return last_in_session;
	}

	bool is_in_quest()
	{
		return in_quest;
	}

	bool is_in_session()
	{
		return last_session_time != session_time;
	}
};

