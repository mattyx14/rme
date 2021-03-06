
//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////
// $URL: http://svn.rebarp.se/svn/RME/trunk/source/brush.hpp $
// $Id: brush.hpp 310 2010-02-26 18:03:48Z admin $

#include "main.h"

#include "brush.h"
#include "carpet_brush.h"
#include "creature_brush.h"
#include "doodad_brush.h"
#include "ground_brush.h"
#include "house_brush.h"
#include "house_exit_brush.h"
#include "raw_brush.h"
#include "spawn_brush.h"
#include "table_brush.h"
#include "wall_brush.h"
#include "waypoint_brush.h"

#include "settings.h"

#include "sprites.h"

#include "item.h"
#include "complexitem.h"
#include "creatures.h"
#include "creature.h"
#include "map.h"

#include "gui.h"

Brushes brushes;

Brushes::Brushes() {
}

Brushes::~Brushes() {
}

void Brushes::clear() {
	for(BrushMap::iterator it = brushes.begin(); it != brushes.end(); it++) {
		delete it->second;
	}
	for(BorderMap::iterator it = borders.begin(); it != borders.end(); it++) {
		delete it->second;
	}
	brushes.clear();
	borders.clear();
}

void Brushes::init() {
	addBrush(gui.optional_brush = newd OptionalBorderBrush());
	addBrush(gui.eraser = newd EraserBrush());
	addBrush(gui.spawn_brush = newd SpawnBrush());
	addBrush(gui.normal_door_brush = newd DoorBrush(WALL_DOOR_NORMAL));
	addBrush(gui.locked_door_brush = newd DoorBrush(WALL_DOOR_LOCKED));
	addBrush(gui.magic_door_brush = newd DoorBrush(WALL_DOOR_MAGIC));
	addBrush(gui.quest_door_brush = newd DoorBrush(WALL_DOOR_QUEST));
	addBrush(gui.hatch_door_brush = newd DoorBrush(WALL_HATCH_WINDOW));
	addBrush(gui.window_door_brush = newd DoorBrush(WALL_WINDOW));
	addBrush(gui.house_brush = newd HouseBrush());
	addBrush(gui.house_exit_brush = newd HouseExitBrush());
	addBrush(gui.waypoint_brush = newd WaypointBrush());

	addBrush(gui.pz_brush = newd FlagBrush(TILESTATE_PROTECTIONZONE));
	addBrush(gui.rook_brush = newd FlagBrush(TILESTATE_NOPVP));
	addBrush(gui.nolog_brush = newd FlagBrush(TILESTATE_NOLOGOUT));
	addBrush(gui.pvp_brush = newd FlagBrush(TILESTATE_PVPZONE));

	GroundBrush::init();
	WallBrush::init();
	TableBrush::init();
	CarpetBrush::init();
}

bool Brushes::unserializeBrush(xmlNodePtr node, wxArrayString& warnings) {
	std::string strVal;
	std::string brushname;
	Brush* brush = NULL;

	if(!readXMLValue(node, "name", brushname)) {
		warnings.push_back(wxT("Brush node without name."));
		return false;
	}

	if(brushname == "all" || brushname == "none") {
		warnings.push_back(wxString(wxT("Using reserved brushname \"")) << wxstr(brushname) << wxT("\"."));
		return false;
	}

	if((brush = getBrush(brushname))) {
		// Brush already forward-declared, later follows real declaration?
	} else {
		if(readXMLValue(node, "type", strVal)) {
			if(strVal == "border" || strVal == "ground") {
				brush = newd GroundBrush();
			} else if(strVal == "wall") {
				brush = newd WallBrush();
			} else if(strVal == "wall decoration") {
				brush = newd WallDecorationBrush();
			} else if(strVal == "carpet") {
				brush = newd CarpetBrush();
			} else if(strVal == "table") {
				brush = newd TableBrush();
			} else if(strVal == "doodad") {
				brush = newd DoodadBrush();
			} else {
				warnings.push_back(wxString(wxT("Unknown brush type ")) << wxstr(strVal));
				return false;
			}
		} else {
			warnings.push_back(wxT("Couldn't read brush type"));
			return false;
		}
		ASSERT(brush);

		brush->setName(brushname);
	}

	if(node->children == NULL) {
		// Forward declaration
		brushes.insert(std::make_pair(brush->getName(), brush));
	} else {
		wxArrayString subwarnings;
		brush->load(node, subwarnings);
		if(subwarnings.size()) {
			warnings.push_back(wxString(wxT("Errors while loading brush \"")) << wxstr(brush->getName()) << wxT("\""));
			warnings.insert(warnings.end(), subwarnings.begin(), subwarnings.end());
		}

		if(brush->getName() == "all" || brush->getName() == "none") {
			warnings.push_back(wxString(wxT("Using reserved brushname '")) << wxstr(brush->getName()) << wxT("'."));
			delete brush;
			return false;
		}

		if(getBrush(brush->getName())) {
			if(getBrush(brush->getName()) != brush) {
				warnings.push_back(wxString(wxT("Duplicate brush name ")) << wxstr(brush->getName()) << wxT(". Undefined behaviour may ensue."));
			} else {
				// Don't insert
				return true;
			}
		}
		brushes.insert(std::make_pair(brush->getName(), brush));
	}
	return true;
}

bool Brushes::unserializeBorder(xmlNodePtr node, wxArrayString& warnings) {
	int id;
	
	if(readXMLValue(node, "id", id)) {
		if(borders[id] != NULL) {
			wxString warning = wxT("Border ID ");
			warning << id << wxT(" already exists");
			warnings.push_back(warning);
			return false;
		}
		AutoBorder* border = newd AutoBorder(id);
		border->load(node, warnings);
		borders[id] = border;
		return true;
	}
	wxString warning = wxT("Couldn't read border id node");
	warnings.push_back(warning);
	return false;

}

void Brushes::addBrush(Brush *brush) {
	brushes.insert(std::make_pair(brush->getName(), brush));
}

Brush* Brushes::getBrush(std::string name) const {
	BrushMap::const_iterator it = brushes.find(name);
	if(it != brushes.end())
		return it->second;
	return NULL;
}

uint Brush::id_counter = 0;

Brush::Brush() :
	id(++id_counter),
	visible(false)
{
}

Brush::~Brush() {

}

TerrainBrush::TerrainBrush() :
	look_id(0),
	hate_friends(false)
{

}

TerrainBrush::~TerrainBrush() {
}

bool TerrainBrush::friendOf(TerrainBrush* other) {
	uint32_t borderID = other->getID();
	for(std::vector<uint32_t>::iterator fit = friends.begin(); fit != friends.end(); ++fit) {
		if(*fit == borderID) {
			//printf("%s is friend of %s\n", getName().c_str(), other->getName().c_str());
			return !hate_friends;
		} else if(*fit == 0xFFFFFFFF) {
			//printf("%s is generic friend of %s\n", getName().c_str(), other->getName().c_str());
			return !hate_friends;
		}
	}
	//printf("%s is enemy of %s\n", getName().c_str(), other->getName().c_str());
	return hate_friends;
}

//=============================================================================
// Flag brush
// draws pz etc.

FlagBrush::FlagBrush(uint _flag) : flag(_flag) {
}

FlagBrush::~FlagBrush() {
}

std::string FlagBrush::getName() const {
	switch(flag) {
		case TILESTATE_PROTECTIONZONE: return "PZ brush (0x01)";
		case TILESTATE_NOPVP: return "No combat zone brush (0x04)";
		case TILESTATE_NOLOGOUT: return "No logout zone brush (0x08)";
		case TILESTATE_PVPZONE: return "PVP Zone brush (0x10)";
	}
	return "Unknown flag brush";
}

int FlagBrush::getLookID() const {
	switch(flag) {
		case TILESTATE_PROTECTIONZONE: return EDITOR_SPRITE_PZ_TOOL;
		case TILESTATE_NOPVP: return EDITOR_SPRITE_NOPVP_TOOL;
		case TILESTATE_NOLOGOUT: return EDITOR_SPRITE_NOLOG_TOOL;
		case TILESTATE_PVPZONE: return EDITOR_SPRITE_PVPZ_TOOL;
	}
	return 0;
}

bool FlagBrush::canDraw(BaseMap* map, Position pos) const {
	Tile* tile = map->getTile(pos);
	return tile && tile->hasGround();
}

void FlagBrush::undraw(BaseMap* map, Tile* tile) {
	tile->unsetMapFlags(flag);
}

void FlagBrush::draw(BaseMap* map, Tile* tile, void* parameter) {
	if(tile->hasGround()) {
		tile->setMapFlags(flag);
	}
}


//=============================================================================
// Door brush

DoorBrush::DoorBrush(DoorType _doortype) : doortype(_doortype) {
}

DoorBrush::~DoorBrush() {
}

std::string DoorBrush::getName() const {
	switch(doortype) {
		case WALL_DOOR_NORMAL: return "Normal door brush";
		case WALL_DOOR_LOCKED: return "Locked door brush";
		case WALL_DOOR_MAGIC: return "Magic door brush";
		case WALL_DOOR_QUEST: return "Quest door brush";
		case WALL_WINDOW: return "Window brush";
		case WALL_HATCH_WINDOW: return "Hatch window brush";
		default: return "Unknown door brush";
	}
}

int DoorBrush::getLookID() const {
	switch(doortype) {
		case WALL_DOOR_NORMAL: return EDITOR_SPRITE_DOOR_NORMAL;
		case WALL_DOOR_LOCKED: return EDITOR_SPRITE_DOOR_LOCKED;
		case WALL_DOOR_MAGIC: return EDITOR_SPRITE_DOOR_MAGIC;
		case WALL_DOOR_QUEST: return EDITOR_SPRITE_DOOR_QUEST;
		case WALL_WINDOW: return EDITOR_SPRITE_WINDOW_NORMAL;
		case WALL_HATCH_WINDOW: return EDITOR_SPRITE_WINDOW_HATCH;
		default: return EDITOR_SPRITE_DOOR_NORMAL;
	}
}

void DoorBrush::switchDoor(Item* item) {
	ASSERT(item);
	ASSERT(item->isBrushDoor());

	WallBrush* wb = item->getWallBrush();
	if(!wb) return;

	bool new_open = !item->isOpen();
	BorderType wall_alignment = item->getWallAlignment();
	DoorType doortype = WALL_UNDEFINED;

	for(std::vector<WallBrush::DoorType>::iterator iter = wb->door_items[wall_alignment].begin();
			iter != wb->door_items[wall_alignment].end();
			++iter)
	{
		WallBrush::DoorType& dt = *iter;
		if(dt.id == item->getID()) {
			doortype = dt.type;
			break;
		}
	}
	if(doortype == WALL_UNDEFINED) return;

	for(std::vector<WallBrush::DoorType>::iterator iter = wb->door_items[wall_alignment].begin();
			iter != wb->door_items[wall_alignment].end();
			++iter)
	{
		WallBrush::DoorType& dt = *iter;
		if(dt.type == doortype) {
			ASSERT(dt.id);
			ItemType& it = item_db[dt.id];
			ASSERT(it.id != 0);

			if(it.isOpen == new_open) {
				item->setID(dt.id);
				return;
			}
		}
	}
}

bool DoorBrush::canDraw(BaseMap* map, Position pos) const {
	Tile* tile = map->getTile(pos);
	if(!tile) return false;
	Item* item = tile->getWall();
	if(!item) return false;
	WallBrush* wb = item->getWallBrush();
	if(!wb) return false;

	BorderType wall_alignment = item->getWallAlignment();

	uint16_t discarded_id = 0; // The id of a discarded match
	bool close_match = false;

	bool open = false;
	if(item->isBrushDoor()) {
		open = item->isOpen();
	}

	WallBrush* test_brush = wb;
	do {
		for(std::vector<WallBrush::DoorType>::iterator iter = test_brush->door_items[wall_alignment].begin();
				iter != test_brush->door_items[wall_alignment].end();
				++iter)
		{
			WallBrush::DoorType& dt = *iter;
			if(dt.type == doortype) {
				ASSERT(dt.id);
				ItemType& it = item_db[dt.id];
				ASSERT(it.id != 0);

				if(it.isOpen == open) {
					return true;
				} else if(close_match == false) {
					discarded_id = dt.id;
					close_match = true;
				}
				if(!close_match && discarded_id == 0) {
					discarded_id = dt.id;
				}
			}
		}
		test_brush = test_brush->redirect_to;
	} while(test_brush != wb && test_brush != NULL);
	// If we've found no perfect match, use a close-to perfect
	if(discarded_id) {
		return true;
	}
	return false;
}

void DoorBrush::undraw(BaseMap* map, Tile* tile) {
	for(ItemVector::iterator it = tile->items.begin();
			it != tile->items.end();
			++it)
	{
		Item* item = *it;
		if(item->isBrushDoor()) {
			item->getWallBrush()->draw(map, tile, NULL);
			if(settings.getInteger(Config::USE_AUTOMAGIC)) {
				tile->wallize(map);
			}
			return;
		}
	}

}

void DoorBrush::draw(BaseMap* map, Tile* tile, void* parameter) {
	for(ItemVector::iterator item_iter = tile->items.begin();
			item_iter != tile->items.end();)
	{
		Item* item = *item_iter;
		if(item->isWall() == false) {
			++item_iter;
			continue;
		}
		WallBrush* wb = item->getWallBrush();
		if(!wb) {
			++item_iter;
			continue;
		}

		BorderType wall_alignment = item->getWallAlignment();

		uint16_t discarded_id = 0; // The id of a discarded match
		bool close_match = false;
		bool perfect_match = false;

		bool open = false;
		if(parameter) {
			open = *reinterpret_cast<bool*>(parameter);
		}

		if(item->isBrushDoor()) {
			open = item->isOpen();
		}

		WallBrush* test_brush = wb;
		do {
			for(std::vector<WallBrush::DoorType>::iterator iter = test_brush->door_items[wall_alignment].begin();
					iter != test_brush->door_items[wall_alignment].end();
					++iter)
			{
				WallBrush::DoorType& dt = *iter;
				if(dt.type == doortype) {
					ASSERT(dt.id);
					ItemType& it = item_db[dt.id];
					ASSERT(it.id != 0);

					if(it.isOpen == open) {
						item = transformItem(item, dt.id, tile);
						perfect_match = true;
						break;
					} else if(close_match == false) {
						discarded_id = dt.id;
						close_match = true;
					}
					if(!close_match && discarded_id == 0) {
						discarded_id = dt.id;
					}
				}
			}
			test_brush = test_brush->redirect_to;
			if(perfect_match) {
				break;
			}
		} while(test_brush != wb && test_brush != NULL);

		// If we've found no perfect match, use a close-to perfect
		if(perfect_match == false && discarded_id) {
			item = transformItem(item, discarded_id, tile);
		}

		if(settings.getInteger(Config::AUTO_ASSIGN_DOORID) && tile->isHouseTile()) {
			Map* mmap = dynamic_cast<Map*>(map);
			Door* door = dynamic_cast<Door*>(item);
			if(mmap && door) {
				House* house = mmap->houses.getHouse(tile->getHouseID());
				ASSERT(house);
				Map* real_map = dynamic_cast<Map*>(map);
				if(real_map) {
					door->setDoorID(house->getEmptyDoorID());
				}
			}
		}

		// We need to consider decorations!
		while(true) {
			// Vector has been modified, before we can use the iterator again we need to find the wall item again
			item_iter = tile->items.begin();
			while(true) {
				if(item_iter == tile->items.end()) {
					return;
				}
				if(*item_iter == item) {
					++item_iter;
					if(item_iter == tile->items.end()) {
						return;
					}
					break;
				}
				++item_iter;
			}
			// Now it points to the correct item!

			item = *item_iter;
			if(item->isWall()) {
				if(WallDecorationBrush* wdb = dynamic_cast<WallDecorationBrush*>(item->getWallBrush())) {
					// We got a decoration!
					for(std::vector<WallBrush::DoorType>::iterator iter = wdb->door_items[wall_alignment].begin();
							iter != wdb->door_items[wall_alignment].end();
							++iter)
					{
						WallBrush::DoorType& dt = *iter;
						if(dt.type == doortype) {
							ASSERT(dt.id);
							ItemType& it = item_db[dt.id];
							ASSERT(it.id != 0);

							if(it.isOpen == open) {
								item = transformItem(item, dt.id, tile);
								perfect_match = true;
								break;
							} else if(close_match == false) {
								discarded_id = dt.id;
								close_match = true;
							}
							if(!close_match && discarded_id == 0) {
								discarded_id = dt.id;
							}
						}
					}
					// If we've found no perfect match, use a close-to perfect
					if(perfect_match == false && discarded_id) {
						item = transformItem(item, discarded_id, tile);
					}
					continue;
				}
			}
			break;
		}
		// If we get this far in the loop we should return
		return;
	}
}

//=============================================================================
// Gravel brush

OptionalBorderBrush::OptionalBorderBrush() {
}

OptionalBorderBrush::~OptionalBorderBrush() {
}

std::string OptionalBorderBrush::getName() const {
	return "Optional Border Tool";
}

int OptionalBorderBrush::getLookID() const {
	return EDITOR_SPRITE_OPTIONAL_BORDER_TOOL;
}

bool OptionalBorderBrush::canDraw(BaseMap* map, Position pos) const {
	Tile* tile = map->getTile(pos);

	// You can't do gravel on a mountain tile
	if(tile) {
		if(GroundBrush* bb = tile->getGroundBrush()) {
			if(bb->hasOptionalBorder()) {
				return false;
			}
		}
	}

	uint x = pos.x;
	uint y = pos.y;
	uint z = pos.z;

	tile = map->getTile(x - 1, y - 1, z);
	if(tile) if(GroundBrush* bb = tile->getGroundBrush()) if(bb->hasOptionalBorder()) return true;
	tile = map->getTile(x    , y - 1, z);
	if(tile) if(GroundBrush* bb = tile->getGroundBrush()) if(bb->hasOptionalBorder()) return true;
	tile = map->getTile(x + 1, y - 1, z);
	if(tile) if(GroundBrush* bb = tile->getGroundBrush()) if(bb->hasOptionalBorder()) return true;
	tile = map->getTile(x - 1, y   , z);
	if(tile) if(GroundBrush* bb = tile->getGroundBrush()) if(bb->hasOptionalBorder()) return true;
	tile = map->getTile(x + 1, y   , z);
	if(tile) if(GroundBrush* bb = tile->getGroundBrush()) if(bb->hasOptionalBorder()) return true;
	tile = map->getTile(x - 1, y + 1, z);
	if(tile) if(GroundBrush* bb = tile->getGroundBrush()) if(bb->hasOptionalBorder()) return true;
	tile = map->getTile(x    , y + 1, z);
	if(tile) if(GroundBrush* bb = tile->getGroundBrush()) if(bb->hasOptionalBorder()) return true;
	tile = map->getTile(x + 1, y + 1, z);
	if(tile) if(GroundBrush* bb = tile->getGroundBrush()) if(bb->hasOptionalBorder()) return true;

	return false;
}

void OptionalBorderBrush::undraw(BaseMap* map, Tile* tile) {
	tile->setOptionalBorder(false); // The bordering algorithm will handle this automagicaly
}

void OptionalBorderBrush::draw(BaseMap* map, Tile* tile, void* parameter) {
	tile->setOptionalBorder(true); // The bordering algorithm will handle this automagicaly
}

