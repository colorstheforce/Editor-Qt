/* !!!! GENERATED FILE - DO NOT EDIT !!!! */

/*
 * This file is part of EasyRPG.
 *
 * EasyRPG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http: *www.gnu.org/licenses/>.
 */

#ifndef _RPG_TREEMAP_H_
#define _RPG_TREEMAP_H_

// Headers
#include <vector>
#include "rpg_mapinfo.h"
#include "rpg_start.h"

/**
 * RPG::TreeMap class.
 */
namespace RPG {
	class TreeMap {
	public:
		TreeMap();

		std::vector<MapInfo> maps;
		std::vector<int> tree_order;
		int active_node;
		Start start;
	};
}

#endif