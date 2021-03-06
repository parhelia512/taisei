/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "menu.h"
#include "common.h"
#include "ingamemenu.h"
#include "global.h"
#include "stagedraw.h"
#include "video.h"

void return_to_game(MenuData *m, void *arg) {
}

void return_to_title(MenuData *m, void *arg) {
	global.game_over = GAMEOVER_ABORT;
}

void restart_game(MenuData *m, void *arg) {
	global.game_over = GAMEOVER_RESTART;
}

void create_ingame_menu(MenuData *m) {
	create_menu(m);
	m->draw = draw_ingame_menu;
	m->flags = MF_Abortable | MF_Transient | MF_AlwaysProcessInput;
	m->transition = TransEmpty;
	add_menu_entry(m, "Return to Game", return_to_game, NULL);
	add_menu_entry(m, "Restart the Game", restart_game, NULL)->transition = TransFadeBlack;
	add_menu_entry(m, "Return to Title", return_to_title, NULL)->transition = TransFadeBlack;
	set_transition(TransEmpty, 0, m->transition_out_time);
}

void draw_ingame_menu_bg(MenuData *menu, float f) {
	float rad = f*IMENU_BLUR;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	video_set_viewport();
	set_ortho();

	Shader *shader = get_shader("ingame_menu");
	glUseProgram(shader->prog);
	glUniform1f(uniloc(shader, "rad"), rad);
	glUniform1f(uniloc(shader, "phase"), menu->frames / 100.0);
	stage_draw_foreground();
	glUseProgram(0);
}

void draw_ingame_menu(MenuData *menu) {
	glPushMatrix();

	draw_ingame_menu_bg(menu, 1.0-menu_fade(menu));

	glPushMatrix();
	glTranslatef(VIEWPORT_X, VIEWPORT_Y, 0);
	glTranslatef(VIEWPORT_W/2, VIEWPORT_H/4, 0);

	draw_menu_selector(0, menu->drawdata[0], menu->drawdata[1]*2, 41, menu->frames);

	menu->drawdata[0] += (menu->cursor*35 - menu->drawdata[0])/7.0;
	menu->drawdata[1] += (stringwidth(menu->entries[menu->cursor].name, _fonts.standard) - menu->drawdata[1])/10.0;

	if(menu->context) {
		float s = 0.3 + 0.2 * sin(menu->frames/10.0);
		glColor4f(1-s/2, 1-s/2, 1-s, 1-menu_fade(menu));
		draw_text(AL_Center, 0, -2 * 35, menu->context, _fonts.standard);
	}

	int i;
	for(i = 0; i < menu->ecount; i++) {
		if(menu->entries[i].action) {
			float s = 0, t = 0.7;
			if(i == menu->cursor) {
				t = 1;
				s = 0.3 + 0.2*sin(menu->frames/7.0);
			}

			glColor4f(t-s,t-s,t-s/2, 1-menu_fade(menu));
		} else
			glColor4f(0.5, 0.5, 0.5, 0.5);

		draw_text(AL_Center, 0, i*35, menu->entries[i].name, _fonts.standard);
	}

	glColor4f(1,1,1,1);
	glPopMatrix();
	glPopMatrix();

	stage_draw_hud();
}
