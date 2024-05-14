#pragma once
#include <stdint.h>

#define MOBJ_COLLISION_TYPE_NONE			0
#define MOBJ_COLLISION_TYPE_SPHERE			1
#define MOBJ_COLLISION_TYPE_SPHEROID		2
#define MOBJ_COLLISION_TYPE_CYLINDER		3
#define MOBJ_COLLISION_TYPE_BOX				4
#define MOBJ_COLLISION_TYPE_CUSTOM			5

typedef struct grpconf_entry_t_ {
	uint16_t objectId;
	uint16_t has3DModel;
	uint16_t nearClip;
	uint16_t farClip;
	uint16_t collisionType;
	uint16_t width;
	uint16_t height;
	uint16_t depth;
} grpconf_entry_t;
