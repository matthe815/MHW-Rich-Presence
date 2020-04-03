#pragma once
#include <string>

//
// The basic player data is housed within this.
//
class Player
{
	std::string hunter_name;

	int hunter_rank      = 0,
		last_hunter_rank = 0,
		master_rank      = 0,
		last_master_rank = 0;

	float session_time    = 0,
		last_session_time = 0;

	bool in_quest     = 0,
		last_in_quest = 0;

	bool last_in_session = 0;

	int weapon_type = 0;

	///
	// Guided Lands level
	///
	int forest_level    = 0,
		wildspire_level = 0,
		coral_level     = 0,
		rotten_level    = 0,
		elder_level     = 0,
		tundra_level    = 0;

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

	void set_guided_lands(int forest, int wildspire, int coral, int rotten, int elder, int tundra)
	{
		forest_level = forest;
		wildspire_level = wildspire;
		coral_level = coral;
		rotten_level = rotten;
		elder_level = elder;
		tundra_level = tundra;
	}

	void set_weapon(int type)
	{
		weapon_type = type;
	}

	int get_weapon_type()
	{
		return weapon_type;
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

	// ---------------------

	int get_forest_level()
	{
		return forest_level;
	}

	int get_wildspire_level()
	{
		return wildspire_level;
	}

	int get_coral_level()
	{
		return coral_level;
	}

	int get_rotten_level()
	{
		return rotten_level;
	}

	int get_elder_level()
	{
		return elder_level;
	}

	int get_tundra_level()
	{
		return tundra_level;
	}
};

