#pragma once

#include <iostream>
#include <mutex>
#include <vector>
#include <random>
#include <algorithm>
#include <execution>
#include <chrono>
#include <array>
#include <memory>

constexpr auto c_collection_size = 5U;
constexpr auto c_player_size = 4U;

class gem_collection
{
	std::array<int, 4> gem_sockets{ 0, 0, 0, 0 };
	int gem_shield{ 0 };
	int collection_color{ 0 };
	int collection_gems_count{ 0 };
public:
	gem_collection() = default;

	int get_collection_color()
	{
		return collection_color;
	}
	int get_collection_count()
	{
		return collection_gems_count;
	}
	int add_gem_to_collection(int color)
	{
		if (collection_color == 0)
		{
			collection_color = color;
		}

		gem_sockets[collection_gems_count] = color;
		++collection_gems_count;

		if (collection_gems_count == 3) return 1;

		return 0;
	}
	int destroy_gem_in_collection()
	{
		gem_sockets[collection_gems_count] = 0;
		--collection_gems_count;

		bool empty_collection = true;
		for (size_t i = 0; i < gem_sockets.size(); ++i)
		{
			if (gem_sockets[i] != 0) empty_collection = false;
		}

		//bool empty_collection = std::count_if(gem_sockets.begin(), gem_sockets.end(), [](int n) { return n != 0; });

		if (empty_collection)
			collection_color = 0;

		if (collection_gems_count == 3) return 1;
		return 0;
	}
	int get_shield_level()
	{
		return gem_shield;
	}
	void set_shield_level(int level)
	{
		gem_shield = level;
	}

	void print_collection()
	{
		std::cout << ", Color: " << collection_color << ", Shield: " << gem_shield << std::endl;

		for (size_t i = 0; i < gem_sockets.size(); ++i)
		{
			std::cout << ">> Socket " << i << " color: " << gem_sockets[i] << std::endl;
		}
	}
	void print_collection_shields()
	{
		std::cout << ", Shield: " << gem_shield << std::endl;
	}
};

class player
{
	std::array<std::unique_ptr<gem_collection>, c_collection_size> gem_collections{};
public:
	player()
	{
		for (size_t i = 0; i < c_collection_size; ++i)
		{
			gem_collections[i] = std::make_unique<gem_collection>();
		}
	}

	int add_gem_to_collection(int collection, int color)
	{
		if (gem_collections[collection]->get_collection_count() >= 3) throw;

		if (gem_collections[collection]->get_collection_color() != 0 &&
			color != gem_collections[collection]->get_collection_color()) throw;

		return gem_collections[collection]->add_gem_to_collection(color);
	}
	void add_shield_to_collection(int collection)
	{
		auto level = gem_collections[collection]->get_shield_level();

		if (level == 0)
			gem_collections[collection]->set_shield_level(1);
		if (level == 1)
			gem_collections[collection]->set_shield_level(2);
	}
	int destroy_gem_in_collection(int color)
	{
		auto arrow{ 0 };
		for (auto& collection : gem_collections)
		{
			if (collection->get_collection_color() == color)
			{
				auto level = collection->get_shield_level();
				if (level == 2) break;
				if (level == 1)
				{
					std::cout << "Detroying shield!" << std::endl;
					collection->set_shield_level(0);
				}
				if (level == 0)
				{
					std::cout << "Destroying " << color << " gem!" << std::endl;
					arrow = collection->destroy_gem_in_collection();
				}
			}
		}
		return arrow;
	}
	std::vector<int> get_collection_colors()
	{
		std::vector<int> colors;

		for (auto& collection : gem_collections)
		{
			if (auto color = collection->get_collection_color(); color != 0)
			{
				colors.push_back(color);
			}
		}
		return std::move(colors);
	}

	void print_gem_collections()
	{
		for (size_t i = 0; i < gem_collections.size(); ++i)
		{
			std::cout << "Collection: " << i;
			gem_collections[i]->print_collection();
		}
	}
	void print_gem_shields()
	{
		for (size_t i = 0; i < gem_collections.size(); ++i)
		{
			std::cout << "Collection: " << i;
			gem_collections[i]->print_collection_shields();
		}
	}
};

static std::mt19937 engine{ 1974 };

inline int get_rand_int()
{
	std::uniform_int_distribution<int> dist{ 1, 7 };
	return dist(engine);
}

int get_dice(size_t count, int sides)
{
	std::vector<int> r;
	std::uniform_int_distribution<int> d{ 1, sides };

	for (size_t i = 0; i < count; ++i)
	{
		r.push_back(d(engine));
	}

	return std::accumulate(r.begin(), r.end(), 0);
}

int run()
{
	std::vector<int> targets;
	auto next_gem = get_rand_int();
	while (targets.size() < 7)
	{
		bool found = false;
		for (auto gem : targets)
		{
			if (next_gem == gem) found = true;
		}
		if (!found)
			targets.push_back(next_gem);

		next_gem = get_rand_int();
	}

	int gem_left{ 0 };
	int gem_right{ 0 };
	int active_player{ 0 };
	std::string x{ "\0" };

	std::array<std::unique_ptr<player>, 4> players{ std::make_unique<player>(), std::make_unique<player>(), std::make_unique<player>(), std::make_unique<player>() };

	std::cout << "Welcome to Glitter Greed!" << std::endl << std::endl;
	std::cout << "Start? (y/n): ";
	std::cin >> x;

	while (x != "n")
	{
		gem_left = get_rand_int();
		gem_right = get_rand_int();

		std::cout << std::endl << "Player " << active_player << " rolls a ";

		if (gem_left == gem_right)
		{
			std::cout << "shield gem!" << std::endl << std::endl;

			players[active_player]->print_gem_shields();

			std::string z{ "\0" };
			std::cout << "Which collection? (number): ";
			std::cin >> z;

			int collection = std::atoi(z.c_str());

			players[active_player]->add_shield_to_collection(collection);
			players[active_player]->print_gem_shields();
		}
		else
		{
			std::cout << gem_left << " and " << gem_right << std::endl << std::endl;

			players[active_player]->print_gem_collections();

			std::string y{ "\0" };
			std::cout << std::endl << "Take gem? (l/r): ";
			std::cin >> y;

			std::string z{ "\0" };
			std::cout << "Which collection? (number): ";
			std::cin >> z;

			int collection = std::atoi(z.c_str());
			int arrow{ 0 };
			int selected_gem{ 0 };

			if (y == "l")
			{
				arrow = players[active_player]->add_gem_to_collection(collection, gem_left);
				selected_gem = gem_left;
			}

			if (y == "r")
			{
				arrow = players[active_player]->add_gem_to_collection(collection, gem_right);
				selected_gem = gem_right;
			}

			if (arrow)
			{
				std::cout << "Arrow fired! Color: " << selected_gem;
				int target_color{ 0 };
				for (size_t i = 0; i < targets.size() - 1; ++i)
				{
					if (targets[i] == selected_gem) target_color = targets[i + 1];
				}
				if (target_color == 0) target_color = targets[0];

				std::cout << " Target Color: " << target_color << std::endl;

				for (auto& player : players)
				{
					// TODO: Do not destroy self gems...
					for (auto& color : player->get_collection_colors())
					{
						if (color == target_color)
						{
							// if shield for socket
							auto arrow = player->destroy_gem_in_collection(color);
						}

					}
					std::cout << "Player Collection:" << std::endl;
					player->print_gem_collections();
				}
			}
			std::cout << std::endl;
			players[active_player]->print_gem_collections();
		}

		std::cout << std::endl << "Continue? (y/n): ";

		++active_player;
		if (active_player > 3) active_player = 0;

		std::cin >> x;
	}
}
