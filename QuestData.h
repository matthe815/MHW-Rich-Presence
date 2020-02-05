#pragma once
class QuestData
{
	int id;
	int map_id;

public: 
	void set_data(int quest_id, int map)
	{
		id = quest_id;
		map_id = map;
	}

	int get_id()
	{
		return id;
	}

	int get_map_id()
	{
		return map_id;
	}
};
