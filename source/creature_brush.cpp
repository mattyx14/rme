//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "creature_brush.h"
#include "settings.h"
#include "tile.h"
#include "creature.h"
#include "basemap.h"

//=============================================================================
// Creature brush

CreatureBrush::CreatureBrush(CreatureType* type) :
	Brush(),
	creature_type(type)
{
	ASSERT(type->brush == NULL);
	type->brush = this;
}

CreatureBrush::~CreatureBrush() {

}

int CreatureBrush::getLookID() const {
	return 0;
}

std::string CreatureBrush::getName() const {
	if(creature_type)
		return creature_type->name;
	return "Creature Brush";
}

bool CreatureBrush::canDraw(BaseMap* map, Position pos) const {
	Tile* tile = map->getTile(pos);
	if(creature_type && tile && !tile->isBlocking()) {
		if(tile->getLocation()->getSpawnCount() == 0) {
			if(settings.getInteger(Config::ALLOW_CREATURES_WITHOUT_SPAWN)) {
				if(tile->isPZ()) {
					if(creature_type->isNpc) {
						return true;
					}
				} else {
					return true;
				}
			}
		} else {
			if(tile->isPZ()) {
				if(creature_type->isNpc) {
					return true;
				}
			} else {
				return true;
			}
		}
	}
	return false;
}

void CreatureBrush::undraw(BaseMap* map, Tile* tile) {
	delete tile->creature;
	tile->creature = NULL;
}

void CreatureBrush::draw(BaseMap* map, Tile* tile, void* parameter) {
	ASSERT(tile);
	ASSERT(parameter);
	if(canDraw(map, tile->getPosition())) {
		undraw(map, tile);
		if(creature_type) {
			tile->creature = newd Creature(creature_type);
			tile->creature->setSpawnTime(*(int*)parameter);
		}
	}
}
