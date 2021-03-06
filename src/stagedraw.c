/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "global.h"
#include "stagedraw.h"
#include "stagetext.h"
#include "video.h"

static int apply_shaderrules(ShaderRule *shaderrules, int fbonum) {
	for(ShaderRule *rule = shaderrules; *rule; ++rule) {
		glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo.bg[!fbonum].fbo);
		(*rule)(fbonum);
		fbonum = !fbonum;
	}

	return fbonum;
}

static void draw_wall_of_text(float f, const char *txt) {
	fontrenderer_draw(&resources.fontren, txt,_fonts.standard);
	Texture *tex = &resources.fontren.tex;
	int strw = tex->w;
	int strh = tex->h;

	float w = SCREEN_W;
	float h = SCREEN_H;

	glPushMatrix();
	glTranslatef(w/2, h/2, 0);
	glScalef(w, h, 1.0);

	Shader *shader = get_shader("spellcard_walloftext");
	glUseProgram(shader->prog);
	glUniform1f(uniloc(shader, "w"), strw/(float)tex->truew);
	glUniform1f(uniloc(shader, "h"), strh/(float)tex->trueh);
	glUniform1f(uniloc(shader, "ratio"), h/w);
	glUniform2f(uniloc(shader, "origin"), creal(global.boss->pos)/h, cimag(global.boss->pos)/w);
	glUniform1f(uniloc(shader, "t"), f);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex->gltex);
	draw_quad();
	glUseProgram(0);

	glPopMatrix();
}

static void draw_spellbg(int t) {
	glPushMatrix();
	glTranslatef(VIEWPORT_X, VIEWPORT_Y, 0);

	Boss *b = global.boss;
	b->current->draw_rule(b, t);

	if(b->current->type == AT_ExtraSpell)
		draw_extraspell_bg(b, t);

	glPushMatrix();
	glTranslatef(creal(b->pos), cimag(b->pos), 0);
	glRotatef(global.frames*7.0, 0, 0, -1);

	if(t < 0) {
		float f = 1.0 - t/(float)ATTACK_START_DELAY;
		glScalef(f,f,f);
	}

	draw_texture(0,0,"boss_spellcircle0");
	glPopMatrix();

	float delay = 0;
	if(b->current->type == AT_ExtraSpell)
		delay = ATTACK_START_DELAY_EXTRA-ATTACK_START_DELAY;
	float f = -(t+delay)/ATTACK_START_DELAY;
	if(f > 0)
		draw_wall_of_text(f, b->current->name);

	if(t < ATTACK_START_DELAY && b->dialog) {
		glPushMatrix();
		float f = -0.5*t/(float)ATTACK_START_DELAY+0.5;
		glColor4f(1,1,1,-f*f+2*f);
		draw_texture_p(VIEWPORT_W*3/4-10*f*f,VIEWPORT_H*2/3-10*f*f,b->dialog);
		glColor4f(1,1,1,1);
		glPopMatrix();
	}

	glPopMatrix();
}

static int apply_bg_shaders(ShaderRule *shaderrules, int fbonum) {
	Boss *b = global.boss;
	if(b && b->current && b->current->draw_rule) {
		int t = global.frames - b->current->starttime;

		int o = fbonum;
		if(t < 4*ATTACK_START_DELAY || b->current->endtime) {
			fbonum = apply_shaderrules(shaderrules, fbonum);
		}

		if(fbonum == o) {
			glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo.bg[!fbonum].fbo);
			draw_fbo_viewport(resources.fbo.bg + fbonum);
			fbonum = !fbonum;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo.bg[!fbonum].fbo);
		draw_spellbg(t);

		int nh = resources.fbo.bg[fbonum].nh;
		float scale = resources.fbo.bg[fbonum].scale;
		complex pos = (VIEWPORT_H*I + conj(b->pos) + (VIEWPORT_X + VIEWPORT_Y*I))*scale;
		float ratio = (float)resources.fbo.bg[fbonum].nh/resources.fbo.bg[fbonum].nw;

		glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo.bg[fbonum].fbo);
		if(t<4*ATTACK_START_DELAY) {
			Shader *shader = get_shader("spellcard_intro");
			glUseProgram(shader->prog);

			glUniform1f(uniloc(shader, "ratio"), ratio);
			glUniform2f(uniloc(shader, "origin"), creal(pos)/nh, cimag(pos)/nh);

			float delay = ATTACK_START_DELAY;
			if(b->current->type == AT_ExtraSpell)
				delay = ATTACK_START_DELAY_EXTRA;
			float duration = ATTACK_START_DELAY_EXTRA;

			glUniform1f(uniloc(shader, "t"), (t+delay)/duration);
		} else if(b->current->endtime) {
			int tn = global.frames - b->current->endtime;
			Shader *shader = get_shader("spellcard_outro");
			glUseProgram(shader->prog);


			float delay = ATTACK_END_DELAY;

			if(boss_is_dying(b)) {
				delay = BOSS_DEATH_DELAY;
			} else if(b->current->type == AT_ExtraSpell) {
				delay = ATTACK_END_DELAY_EXTRA;
			}

			glUniform1f(uniloc(shader, "ratio"), ratio);
			glUniform2f(uniloc(shader, "origin"), creal(pos)/nh, cimag(pos)/nh);

			glUniform1f(uniloc(shader, "t"), max(0,tn/delay+1));

		} else {
			glUseProgram(0);
		}

		draw_fbo_viewport(&resources.fbo.bg[!fbonum]);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(0);
	} else
		fbonum = apply_shaderrules(shaderrules, fbonum);

	return fbonum;
}

static void apply_zoom_shader(void) {
	Shader *shader = get_shader("boss_zoom");
	glUseProgram(shader->prog);

	int nh = resources.fbo.bg[0].nh;
	float scale = resources.fbo.bg[0].scale;
	complex fpos = (VIEWPORT_H*I + conj(global.boss->pos) + (VIEWPORT_X + VIEWPORT_Y*I))*scale;
	complex pos = fpos + 15*cexp(I*global.frames/4.5)*scale;

	glUniform2f(uniloc(shader, "blur_orig"),
			creal(pos)/nh, cimag(pos)/nh);
	glUniform2f(uniloc(shader, "fix_orig"),
			creal(fpos)/nh, cimag(fpos)/nh);

	float spellcard_sup = 1;
	// This factor is used to surpress the effect near the start of spell cards.
	// This is necessary so it doesn’t distort the awesome spinning background effect.

	if(global.boss->current && global.boss->current->draw_rule) {
		float t = (global.frames - global.boss->current->starttime + ATTACK_START_DELAY)/(float)ATTACK_START_DELAY;
		spellcard_sup = 1-1/(0.1*t*t+1);
	}

	if(boss_is_dying(global.boss)) {
		float t = (global.frames - global.boss->current->endtime)/(float)BOSS_DEATH_DELAY + 1;
		spellcard_sup = 1-t*t;
	}

	glUniform1f(uniloc(shader, "blur_rad"), spellcard_sup*(0.2+0.025*sin(global.frames/15.0)));
	glUniform1f(uniloc(shader, "rad"), 0.24);
	glUniform1f(uniloc(shader, "ratio"), (float)resources.fbo.bg[0].nh/resources.fbo.bg[0].nw);

	if(global.boss->zoomcolor) {
		static float clr[4];
		parse_color_array(global.boss->zoomcolor, clr);
		glUniform4fv(uniloc(shader, "color"), 1, clr);
	} else {
		glUniform4f(uniloc(shader, "color"), 0.1, 0.2, 0.3, 1);
	}
}

static FBO* stage_render_bg(StageInfo *stage) {
	glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo.bg[0].fbo);
	float scale = resources.fbo.bg[0].scale;
	glViewport(0, 0, scale*SCREEN_W, scale*SCREEN_H);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
		glTranslatef(-(VIEWPORT_X+VIEWPORT_W/2), -(VIEWPORT_Y+VIEWPORT_H/2),0);
		glEnable(GL_DEPTH_TEST);
		stage->procs->draw();
	glPopMatrix();

	int num = apply_bg_shaders(stage->procs->shader_rules, 0);
	return &resources.fbo.bg[num];
}

static void stage_draw_objects(void) {
	if(global.boss) {
		glPushMatrix();
		glTranslatef(creal(global.boss->pos), cimag(global.boss->pos), 0);

		if(!(global.frames % 5)) {
			complex offset = (frand()-0.5)*50 + (frand()-0.5)*20.0*I;
			create_particle3c("boss_shadow", -20.0*I, rgba(0.2,0.35,0.5,0.5), EnemyFlareShrink, enemy_flare, 50, (-100.0*I-offset)/(50.0+frand()*10), add_ref(global.boss));
		}

		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glRotatef(global.frames*4.0, 0, 0, -1);

		float f = 0.8+0.1*sin(global.frames/8.0);

		if(boss_is_dying(global.boss)) {
			float t = (global.frames - global.boss->current->endtime)/(float)BOSS_DEATH_DELAY + 1;
			f -= t*(t-0.7)/(1-t);
		}

		glScalef(f,f,f);
		draw_texture(0, 0, "boss_circle");
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPopMatrix();
	}

	player_draw(&global.plr);

	draw_items();
	draw_projectiles(global.projs);
	draw_projectiles(global.particles);
	draw_lasers(true);
	draw_enemies(global.enemies);
	draw_lasers(false);

	if(global.boss)
		draw_boss(global.boss);

	if(global.dialog)
		draw_dialog(global.dialog);

	stagetext_draw();
}

static void postprocess_prepare(FBO *fbo, Shader *s) {
	float w = fbo->scale;
	float h = fbo->scale;

	glUniform1i(uniloc(s, "frames"), global.frames);
	glUniform2f(uniloc(s, "view_ofs"), VIEWPORT_X * w, VIEWPORT_Y * h);
	glUniform2f(uniloc(s, "view_scale"), VIEWPORT_W * w, VIEWPORT_H * h);
}

void stage_draw_foreground(void) {
	// draw the foreground FBO
	glPushMatrix();
		// apply the screenshake effect
		if(global.shake_view) {
			glTranslatef(global.shake_view*sin(global.frames),global.shake_view*sin(global.frames*1.1+3),0);
			glScalef(1+2*global.shake_view/VIEWPORT_W,1+2*global.shake_view/VIEWPORT_H,1);
			glTranslatef(-global.shake_view,-global.shake_view,0);

			if(global.shake_view_fade) {
				global.shake_view -= global.shake_view_fade;
				if(global.shake_view <= 0)
					global.shake_view = global.shake_view_fade = 0;
			}
		}

		draw_fbo(&resources.fbo.fg[0]);
	glPopMatrix();
}

void stage_draw_scene(StageInfo *stage) {
#ifdef DEBUG
	bool key_nobg = gamekeypressed(KEY_NOBACKGROUND);
#else
	bool key_nobg = false;
#endif

	bool draw_bg = !config_get_int(CONFIG_NO_STAGEBG) && !key_nobg;
	FBO *fbg = NULL;

	if(draw_bg) {
		// render the 3D background
		fbg = stage_render_bg(stage);
	}

	// switch to foreground FBO
	glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo.fg[0].fbo);
	float scale = resources.fbo.fg[0].scale;
	glViewport(0, 0, scale*SCREEN_W, scale*SCREEN_H);
	set_ortho();

	if(draw_bg) {
		// enable boss background distortion
		if(global.boss) {
			apply_zoom_shader();
		}

		// draw the 3D background
		draw_fbo(fbg);

		// disable boss background distortion
		glUseProgram(0);

		// fade the background during bomb
		if(global.frames - global.plr.recovery < 0) {
			float t = BOMB_RECOVERY - global.plr.recovery + global.frames;
			float fade = 1;

			if(t < BOMB_RECOVERY/6)
				fade = t/BOMB_RECOVERY*6;

			if(t > BOMB_RECOVERY/4*3)
				fade = 1-t/BOMB_RECOVERY*4 + 3;

			glPushMatrix();
			fade_out(fade*0.6);
			glPopMatrix();
		}
	} else if(!key_nobg) {
		glClear(GL_COLOR_BUFFER_BIT);
	}

	// draw the 2D objects
	set_ortho();
	glPushMatrix();
		glTranslatef(VIEWPORT_X, VIEWPORT_Y, 0);
		stage_draw_objects();
	glPopMatrix();

	// apply custom postprocessing shaders
	FBO *ppfbo = postprocess(
		resources.stage_postprocess,
		resources.fbo.fg,
		resources.fbo.fg+1,
		postprocess_prepare,
		draw_fbo_viewport
	);

	// update the primary foreground FBO if needed
	if(ppfbo != resources.fbo.fg) {
		glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo.fg[0].fbo);
		draw_fbo_viewport(ppfbo);
	}

	// switch to main framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	video_set_viewport();
	set_ortho();

	// finally, draw stuff to the actual screen
	stage_draw_foreground();
	stage_draw_hud();
}

static void draw_star(int x, int y, float fill, float alpha) {
	Texture *star = get_tex("star");
	Shader *shader = get_shader("circleclipped_indicator");

	float clr[4];

	Color amul = rgba(alpha, alpha, alpha, alpha);
	Color fill_clr = multiply_colors(rgba(1.0f, 1.0f, 1.0f, 1.0f), amul);
	Color back_clr = multiply_colors(rgba(0.2f, 0.6f, 1.0f, 0.2f), amul);

	if(fill < 1) {
		fill_clr = mix_colors(derive_color(back_clr, CLRMASK_A, alpha), fill_clr, 0.35f);
	}

	if(fill >= 1 || fill <= 0) {
		parse_color_call(fill > 0 ? fill_clr : back_clr, glColor4f);
		draw_texture_p(x, y, star);
		glColor4f(1, 1, 1, 1);
		return;
	}

	glUseProgram(shader->prog);
	glUniform1f(uniloc(shader, "fill"), fill);
	glUniform1f(uniloc(shader, "tcfactor"), star->truew / (float)star->w);
	parse_color_array(fill_clr, clr);
	glUniform4fv(uniloc(shader, "fill_color"), 1, clr);
	parse_color_array(back_clr, clr);
	glUniform4fv(uniloc(shader, "back_color"), 1, clr);
	draw_texture_p(x, y, star);
	glUseProgram(0);
}

static void draw_stars(int x, int y, int numstars, int numfrags, int maxstars, int maxfrags, float alpha) {
	static const int star_width = 20;
	int i = 0;

	while(i < numstars) {
		draw_star(x + star_width * i++, y, 1, alpha);
	}

	if(numfrags) {
		draw_star(x + star_width * i++, y, numfrags / (float)maxfrags, alpha);
	}

	while(i < maxstars) {
		draw_star(x + star_width * i++, y, 0, alpha);
	}
}

void stage_draw_hud(void) {
	draw_texture(SCREEN_W/2.0, SCREEN_H/2.0, "hud");

	char buf[64];

	glPushMatrix();
	glTranslatef(615,0,0);

	glPushMatrix();
	glTranslatef((SCREEN_W - 615) * 0.25, SCREEN_H-170, 0);
	glScalef(0.6, 0.6, 0);

	draw_texture(0,0,difficulty_tex(global.diff));
	glPopMatrix();

	if(global.stage->type == STAGE_SPELL) {
		glColor4f(1, 1, 1, 0.7);
		draw_text(AL_Left, -6, 167, "N/A", _fonts.standard);
		draw_text(AL_Left, -6, 200, "N/A", _fonts.standard);
		glColor4f(1, 1, 1, 1.0);
	} else {
		float a = 1, s = 0, fadein = 1, fadeout = 1, fade = 1;

		if(global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell) {
			fadein  = min(1, -min(0, global.frames - global.boss->current->starttime) / (float)ATTACK_START_DELAY);
			fadeout = global.boss->current->finished * (1 - (global.boss->current->endtime - global.frames) / (float)ATTACK_END_DELAY_EXTRA) / 0.74;
			fade = max(fadein, fadeout);

			s = 1 - fade;
			a = 0.5 + 0.5 * fade;
		}

		draw_stars(0, 167, global.plr.lives, global.plr.life_fragments, PLR_MAX_LIVES, PLR_MAX_LIFE_FRAGMENTS, a);
		draw_stars(0, 200, global.plr.bombs, global.plr.bomb_fragments, PLR_MAX_BOMBS, PLR_MAX_BOMB_FRAGMENTS, a);

		if(s) {
			float s2 = max(0, swing(s, 3));
			glPushMatrix();
			glTranslatef((SCREEN_W - 615) * 0.25 - 615 * (1 - pow(2*fadein-1, 2)), 400, 0);
			glColor4f(0.3, 0.6, 0.7, 0.7 * s);
			glRotatef(-25 + 360 * (1-s2), 0, 0, 1);
			glScalef(s2, s2, 0);
			draw_text(AL_Center,  1,  1, "Extra Spell!", _fonts.mainmenu);
			draw_text(AL_Center, -1, -1, "Extra Spell!", _fonts.mainmenu);
			glColor4f(1, 1, 1, s);
			draw_text(AL_Center, 0, 0, "Extra Spell!", _fonts.mainmenu);
			glColor4f(1, 1, 1, 1);
			glPopMatrix();
		}
	}

	// snprintf(buf, sizeof(buf), "%.2f", global.plr.power / 100.0);
	// draw_text(AL_Left, -6, 236, buf, _fonts.standard);

	draw_stars(0, 236, global.plr.power / 100, global.plr.power % 100, PLR_MAX_POWER / 100, 100, 1);

	snprintf(buf, sizeof(buf), "%i", global.plr.graze);
	draw_text(AL_Left, -6, 270, buf, _fonts.standard);

	snprintf(buf, sizeof(buf), "%i", global.plr.points);
	draw_text(AL_Left, 8, 49, buf, _fonts.standard);

	snprintf(buf, sizeof(buf), "%i", progress.hiscore);
	draw_text(AL_Left, 8, 83, buf, _fonts.standard);

	if(global.plr.iddqd) {
		draw_text(AL_Left, -70, 475, "GOD MODE", _fonts.mainmenu);
	}

	glPopMatrix();

	snprintf(buf, sizeof(buf), "%i fps", global.fps.show_fps);
	draw_text(AL_Right, SCREEN_W, SCREEN_H - 0.5 * stringheight(buf, _fonts.standard), buf, _fonts.standard);

	if(global.boss)
		draw_texture(VIEWPORT_X+creal(global.boss->pos), 590, "boss_indicator");

	if(global.replaymode == REPLAY_PLAY) {
		snprintf(buf, sizeof(buf), "Replay: %s (%i fps)", global.replay.playername, global.replay_stage->fps);
		glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		draw_text(AL_Left, 0, SCREEN_H - 0.5 * stringheight(buf, _fonts.standard), buf, _fonts.standard);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
#ifdef PLR_DPS_STATS
	else {
		snprintf(buf, sizeof(buf), "Avg DPS: %f", global.plr.total_dmg / (global.frames / (double)FPS));
		draw_text(AL_Left, 0, SCREEN_H - 0.5 * stringheight(buf, _fonts.standard), buf, _fonts.standard);
	}
#endif
}
