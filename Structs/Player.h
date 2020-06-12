#pragma once
#include <string>
#include "./WeaponType.cpp"

//
// The basic player data is housed within this.
//
class Player
{
	std::string name;
	int rank;
	int master_rank;
	WeaponType weapon_type;
	int guided_lands_levels[6];

public:
	/*
		Verify the provided player and this one aren't different.
	*/
	bool is_different(Player player) {
		return !(player.get_name() == this->get_name()&&player.get_rank()==this->get_rank()&&player.get_master_rank()==this->get_master_rank()&&player.get_weapon_type()==this->get_weapon_type());
	}

	void set_name(const std::string &name) {
		this->name = name;
	}

	std::string get_name()
	{
		return this->name;
	}

	void set_rank(const int& rank, const int& master_rank) {
		this->rank = rank;
		this->master_rank = master_rank;
	}

	int get_rank() {
		return this->rank;
	}

	int get_master_rank() {
		return this->master_rank;
	}

	void set_weapon_type(const WeaponType &type) {
		this->weapon_type = type;
	}

	WeaponType get_weapon_type() {
		return this->weapon_type;
	}

	void set_guided_lands_level(const int &forest,  const int &desert, const int &coral, const int &elders, const int &tundra, const int &volcano) {
		this->guided_lands_levels[0] = forest;
		this->guided_lands_levels[1] = desert;
		this->guided_lands_levels[2] = coral;
		this->guided_lands_levels[3] = elders;
		this->guided_lands_levels[4] = tundra;
		this->guided_lands_levels[5] = volcano;
	}
};

