#pragma once
#include <string>

class Player
{
	std::string hunter_name;

	float hunter_rank;
	float last_hunter_rank;

	float session_time;
	float last_session_time;

	bool in_quest;
	bool last_in_quest;

	bool last_in_session;

public:
	void set_data(std::string name, float hr, float current_session, bool is_quest)
	{
		hunter_name = name;

		last_hunter_rank = get_hunter_rank();
		hunter_rank = hr;

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

	float get_last_hunter_rank()
	{
		return last_hunter_rank;
	}

	float get_last_session_time()
	{
		return last_session_time;
	}

	float get_hunter_rank()
	{
		return hunter_rank;
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

