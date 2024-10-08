/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "coroutine/taskdsl.h"
#include "entity.h"
#include "renderer/api.h"

DEFINE_ENTITY_TYPE(WriggleSlave, {
	struct {
		Sprite *circle, *particle;
	} sprites;

	cmplx pos;
	int spawn_time;
	Color color;
	cmplxf scale;

	COEVENTS_ARRAY(
		despawned
	) events;
});

Boss *stage3_spawn_wriggle(cmplx pos);
void stage3_draw_wriggle_spellbg(Boss *boss, int time);

void stage3_init_wriggle_slave(WriggleSlave *slave, cmplx pos);
WriggleSlave *stage3_host_wriggle_slave(cmplx pos);
void stage3_despawn_wriggle_slave(WriggleSlave *slave);

DECLARE_EXTERN_TASK(wriggle_slave_damage_trail, { BoxedWriggleSlave slave; });
DECLARE_EXTERN_TASK(wriggle_slave_follow, {
	BoxedWriggleSlave slave;
	BoxedBoss boss;
	real rot_speed;
	real rot_initial;
	cmplx *out_dir;
});
