/*
 * $Source$
 * $Date$
 * $Revision$
 *
 * DESCRIPTION:
 * Chadwick expanded event descriptor program
 * 
 * This file is part of Chadwick, tools for baseball play-by-play and stats
 * Copyright (C) 2002-2007, Ted Turocy (drarbiter@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or (at 
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cwlib/chadwick.h"

/*************************************************************************
 * Global variables for command-line options
 *************************************************************************/

extern int ascii;

/* Fields to display (-f) */
int fields[97] = {
  1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
  0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 
  0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 0, 0, 1, 1, 1, 0, 0, 0, 0,
  0, 1, 0, 0, 0, 0, 0, 0, 1, 1,
  1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

int max_field = 96;

/* Extended fields to display (-x) */
int ext_fields[46] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0
};

int max_ext_field = 45;

char program_name[20] = "cwevent";

int print_header = 0;

/*************************************************************************
 * Utility functions (in some cases, candidates for refactor to cwlib)
 *************************************************************************/

/* Compute the eventual "fate" of the runner on 'base' */
int cwevent_runner_fate(CWGameIterator *orig_gameiter, int base)
{
  CWGameIterator *gameiter;
  if (orig_gameiter->event_data->advance[base] == 0 ||
      orig_gameiter->event_data->advance[base] >= 4) {
    return orig_gameiter->event_data->advance[base];
  }
  
  base = orig_gameiter->event_data->advance[base];
  gameiter = cw_gameiter_copy(orig_gameiter);
  while (gameiter->event != NULL && 
	 gameiter->state->inning == orig_gameiter->state->inning &&
	 gameiter->state->batting_team == orig_gameiter->state->batting_team &&
	 base >= 1 && base <= 3) {
    cw_gameiter_next(gameiter);
    if (gameiter->event && strcmp(gameiter->event->event_text, "NP")) {
      base = gameiter->event_data->advance[base];
    }
  }

  cw_gameiter_cleanup(gameiter);
  free(gameiter);
  return base;
}			

/* Compute the number of runs scored after this play in half inning */
int cwevent_future_runs(CWGameIterator *orig_gameiter)
{
  int runs = 0;
  CWGameIterator *gameiter = cw_gameiter_copy(orig_gameiter);
  cw_gameiter_next(gameiter);
  while (gameiter->event != NULL &&
	 gameiter->state->inning == orig_gameiter->state->inning &&
	 gameiter->state->batting_team == orig_gameiter->state->batting_team) {
    if (strcmp(gameiter->event->event_text, "NP")) {
      runs += cw_event_runs_on_play(gameiter->event_data);
    }
    cw_gameiter_next(gameiter);
  }
  
  cw_gameiter_cleanup(gameiter);
  free(gameiter);
  return runs;
}


/*************************************************************************
 * Functions to output fields
 *************************************************************************/

/*
 * typedef to declare the pointer-to-function type
 */
typedef int (*field_func)(char *, CWGameIterator *,
			  CWRoster *, CWRoster *);


/*
 * preprocessor directive for conveniently declaring function signature
 */

#define DECLARE_FIELDFUNC(funcname) \
int funcname(char *buffer, CWGameIterator *gameiter, \
	     CWRoster *visitors, CWRoster *home)

/* Field 0 */
DECLARE_FIELDFUNC(cwevent_game_id)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-12s",
		 gameiter->game->game_id);
} 

/* Field 1 */
DECLARE_FIELDFUNC(cwevent_visiting_team)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-3s",
		 cw_game_info_lookup(gameiter->game, "visteam"));
}

/* Field 2 */
DECLARE_FIELDFUNC(cwevent_inning)
{
  return sprintf(buffer, (ascii) ? "%d" : "%4d", gameiter->event->inning);
}

/* Field 3 */
DECLARE_FIELDFUNC(cwevent_batting_team)
{
  return sprintf(buffer, "%d", gameiter->event->batting_team);
}

/* Field 4 */
DECLARE_FIELDFUNC(cwevent_outs)
{
  return sprintf(buffer, "%d", gameiter->state->outs); 
}

/* Field 5 */
DECLARE_FIELDFUNC(cwevent_balls)
{
  if (strlen(gameiter->event->count) >= 2 && 
      gameiter->event->count[0] != '?' &&
      gameiter->event->count[1] != '?') {
    return sprintf(buffer, "%c", gameiter->event->count[0]);
  }
  else {
    return sprintf(buffer, "0");
  }
}

/* Field 6 */
DECLARE_FIELDFUNC(cwevent_strikes)
{
  if (strlen(gameiter->event->count) >= 2 &&
      gameiter->event->count[0] != '?' &&
      gameiter->event->count[1] != '?') {
    return sprintf(buffer, "%c", gameiter->event->count[1]);
  }
  else {
    return sprintf(buffer, "0");
  }
}

/* Field 7 */
DECLARE_FIELDFUNC(cwevent_pitches)
{  
  /* This bit of code wipes out leading whitespace, which bevent
   * does not include in its output */
  char *foo = gameiter->event->pitches;
  while (foo != '\0' && isspace(*foo)) {
    foo++;
  }
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-20s", foo);
}

/* Field 8 */
DECLARE_FIELDFUNC(cwevent_visitor_score)
{ 
  return sprintf(buffer, (ascii) ? "%d" : "%2d", gameiter->state->score[0]);
}

/* Field 9 */
DECLARE_FIELDFUNC(cwevent_home_score)
{ 
  return sprintf(buffer, (ascii) ? "%d" : "%2d", gameiter->state->score[1]);
}

/* Field 10 */
DECLARE_FIELDFUNC(cwevent_batter)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s", 
		 gameiter->event->batter);
}

/* Field 11 */
DECLARE_FIELDFUNC(cwevent_batter_hand)
{
  char batterHand, pitcherHand;
  if (gameiter->event->batter_hand == ' ') {
    batterHand = cw_roster_batting_hand((gameiter->event->batting_team == 0) ? 
					visitors : home,
					gameiter->event->batter);
  }
  else {
    batterHand = gameiter->event->batter_hand;
  }

  if (batterHand == 'B') {
    pitcherHand =
      cw_roster_throwing_hand((gameiter->event->batting_team == 0) ?
			      home : visitors,
			      gameiter->state->fielders[1][1-gameiter->state->batting_team]);

    if (pitcherHand == 'L') {
      batterHand = 'R';
    }
    else if (pitcherHand == 'R') {
      batterHand = 'L';
    }
    else {
      batterHand = '?';
    }
  }

  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c", batterHand);
}

/* Field 12 */
DECLARE_FIELDFUNC(cwevent_res_batter)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 cw_gamestate_charged_batter(gameiter->state,
					     gameiter->event->batter,
					     gameiter->event_data));
}

/* Field 13 */
DECLARE_FIELDFUNC(cwevent_res_batter_hand)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 cw_gamestate_charged_batter_hand(gameiter->state,
						  gameiter->event->batter,
						  gameiter->event_data,
						  (gameiter->event->batting_team == 0) ? visitors : home,
						  (gameiter->event->batting_team == 0) ? home : visitors));
}

/* Field 14 */
DECLARE_FIELDFUNC(cwevent_pitcher)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
	  gameiter->state->fielders[1][1-gameiter->state->batting_team]);
}

/* Field 15 */
DECLARE_FIELDFUNC(cwevent_pitcher_hand)
{
  char pitcherHand;

  if (gameiter->event->pitcher_hand == ' ') {
    pitcherHand = 
      cw_roster_throwing_hand((gameiter->event->batting_team == 0) ?
			      home : visitors,
			      gameiter->state->fielders[1][1-gameiter->state->batting_team]);
  }
  else {
    pitcherHand = gameiter->event->pitcher_hand;
  }

  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c", pitcherHand);
}

/* Field 16 */
DECLARE_FIELDFUNC(cwevent_res_pitcher)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 cw_gamestate_charged_pitcher(gameiter->state,
					      gameiter->event_data));
}

/* Field 17 */
DECLARE_FIELDFUNC(cwevent_res_pitcher_hand)
{
  char resPitcherHand;

  if (gameiter->event->pitcher_hand == ' ') {
    resPitcherHand = 
      cw_roster_throwing_hand((gameiter->event->batting_team == 0) ?
			      home : visitors,
			      cw_gamestate_charged_pitcher(gameiter->state,
							   gameiter->event_data));
  }
  else {
    resPitcherHand = gameiter->event->pitcher_hand;
  }

  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c", resPitcherHand);
}

/* Field 18 */
DECLARE_FIELDFUNC(cwevent_catcher)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 gameiter->state->fielders[2][1-gameiter->state->batting_team]);
}

/* Field 19 */
DECLARE_FIELDFUNC(cwevent_first_baseman)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 gameiter->state->fielders[3][1-gameiter->state->batting_team]);
}

/* Field 20 */
DECLARE_FIELDFUNC(cwevent_second_baseman)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 gameiter->state->fielders[4][1-gameiter->state->batting_team]);
}

/* Field 21 */
DECLARE_FIELDFUNC(cwevent_third_baseman)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 gameiter->state->fielders[5][1-gameiter->state->batting_team]);
}

/* Field 22 */
DECLARE_FIELDFUNC(cwevent_shortstop)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 gameiter->state->fielders[6][1-gameiter->state->batting_team]);
}

/* Field 23 */
DECLARE_FIELDFUNC(cwevent_left_fielder)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 gameiter->state->fielders[7][1-gameiter->state->batting_team]);
}

/* Field 24 */
DECLARE_FIELDFUNC(cwevent_center_fielder)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 gameiter->state->fielders[8][1-gameiter->state->batting_team]);
}

/* Field 25 */
DECLARE_FIELDFUNC(cwevent_right_fielder)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
	  gameiter->state->fielders[9][1-gameiter->state->batting_team]);
}

/* Field 26 */
DECLARE_FIELDFUNC(cwevent_runner_first)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 gameiter->state->runners[1]);
}

/* Field 27 */
DECLARE_FIELDFUNC(cwevent_runner_second)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 gameiter->state->runners[2]);
}

/* Field 28 */
DECLARE_FIELDFUNC(cwevent_runner_third)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 gameiter->state->runners[3]);
}

/* Field 29 */
DECLARE_FIELDFUNC(cwevent_event_text)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-20s",
		 gameiter->event->event_text);
}

/* Field 30 */
DECLARE_FIELDFUNC(cwevent_leadoff_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 (gameiter->state->is_leadoff) ? 'T' : 'F');
}

/* Field 31 */
DECLARE_FIELDFUNC(cwevent_ph_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 (gameiter->state->ph_flag) ? 'T' : 'F');
}

/* Field 32 */
DECLARE_FIELDFUNC(cwevent_defensive_position)
{
  return sprintf(buffer, (ascii) ? "%d" : "%2d", 
		 cw_gamestate_player_position(gameiter->state,
					      gameiter->state->batting_team,
					      gameiter->event->batter));
}

/* Field 33 */
DECLARE_FIELDFUNC(cwevent_lineup_position)
{
  /* A bit of a kludge to handle case where pitcher enters the lineup
   * after DH goes away */
  int lineupSlot = cw_gamestate_lineup_slot(gameiter->state,
					    gameiter->state->batting_team,
					    gameiter->event->batter);
  if (lineupSlot > 0) {
    return sprintf(buffer, "%d", lineupSlot);
  }
  else {
    return sprintf(buffer, "%d",
	    (gameiter->state->num_batters[gameiter->state->batting_team]) % 9 + 1);
  }
}

/* Field 34 */
DECLARE_FIELDFUNC(cwevent_event_type)
{
  return sprintf(buffer, (ascii) ? "%d" : "%2d", gameiter->event_data->event_type);
}

/* Field 35 */
DECLARE_FIELDFUNC(cwevent_batter_event_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 cw_event_is_batter(gameiter->event_data) ? 'T' : 'F');
}

/* Field 36 */
DECLARE_FIELDFUNC(cwevent_ab_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 cw_event_is_official_ab(gameiter->event_data) ? 'T' : 'F');
}

/* Field 37 */
DECLARE_FIELDFUNC(cwevent_hit_value)
{
  return sprintf(buffer, "%d", 
		 (gameiter->event_data->event_type >= CW_EVENT_SINGLE &&
		  gameiter->event_data->event_type <= CW_EVENT_HOMERUN) ?
		 gameiter->event_data->event_type - CW_EVENT_SINGLE + 1 : 0);
}

/* Field 38 */
DECLARE_FIELDFUNC(cwevent_sh_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->sh_flag ? 'T' : 'F');
}

/* Field 39 */
DECLARE_FIELDFUNC(cwevent_sf_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->sf_flag ? 'T' : 'F');
}

/* Field 40 */
DECLARE_FIELDFUNC(cwevent_outs_on_play)
{
  return sprintf(buffer, "%d", cw_event_outs_on_play(gameiter->event_data));
}

/* Field 41 */
DECLARE_FIELDFUNC(cwevent_dp_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->dp_flag ? 'T' : 'F');
}

/* Field 42 */
DECLARE_FIELDFUNC(cwevent_tp_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->tp_flag ? 'T' : 'F');
}

/* Field 43 */
DECLARE_FIELDFUNC(cwevent_rbi_on_play)
{
  return sprintf(buffer, "%d", cw_event_rbi_on_play(gameiter->event_data));
}

/* Field 44 */
DECLARE_FIELDFUNC(cwevent_wp_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->wp_flag ? 'T' : 'F');
}

/* Field 45 */
DECLARE_FIELDFUNC(cwevent_pb_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->pb_flag ? 'T' : 'F');
}

/* Field 46 */
DECLARE_FIELDFUNC(cwevent_fielded_by)
{
  return sprintf(buffer, "%d", gameiter->event_data->fielded_by);
}

/* Field 47 */
DECLARE_FIELDFUNC(cwevent_batted_ball_type)
{
  if (gameiter->event_data->batted_ball_type != ' ') {
    return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		   gameiter->event_data->batted_ball_type);
  }
  else {
    return sprintf(buffer, (ascii) ? "\"\"" : " ");
  }
}

/* Field 48 */
DECLARE_FIELDFUNC(cwevent_bunt_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->bunt_flag ? 'T' : 'F');
}

/* Field 49 */
DECLARE_FIELDFUNC(cwevent_foul_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->foul_flag ? 'T' : 'F');
}

/* Field 50 */
DECLARE_FIELDFUNC(cwevent_hit_location)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%4s",
		 gameiter->event_data->hit_location);
}

/* Field 51 */
DECLARE_FIELDFUNC(cwevent_num_errors)
{
  return sprintf(buffer, "%d", gameiter->event_data->num_errors);
}

/* Field 52 */
DECLARE_FIELDFUNC(cwevent_error1_player)
{
  return sprintf(buffer, "%d", gameiter->event_data->errors[0]);
}

/* Field 53 */
DECLARE_FIELDFUNC(cwevent_error1_type)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "   %c",
		 gameiter->event_data->error_types[0]);
}

/* Field 54 */
DECLARE_FIELDFUNC(cwevent_error2_player)
{
  return sprintf(buffer, "%d", gameiter->event_data->errors[1]);
}

/* Field 55 */
DECLARE_FIELDFUNC(cwevent_error2_type)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "   %c",
		 gameiter->event_data->error_types[1]);
}

/* Field 56 */
DECLARE_FIELDFUNC(cwevent_error3_player)
{
  return sprintf(buffer, "%d", gameiter->event_data->errors[2]);
}

/* Field 57 */
DECLARE_FIELDFUNC(cwevent_error3_type)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "   %c",
		 gameiter->event_data->error_types[2]);
}

/* Field 58 */
DECLARE_FIELDFUNC(cwevent_batter_advance)
{
  return sprintf(buffer, "%d", gameiter->event_data->advance[0]);
}

/* Field 59 */
DECLARE_FIELDFUNC(cwevent_runner1_advance)
{
  return sprintf(buffer, "%d", gameiter->event_data->advance[1]);
}

/* Field 60 */
DECLARE_FIELDFUNC(cwevent_runner2_advance)
{
  return sprintf(buffer, "%d", gameiter->event_data->advance[2]);
}

/* Field 61 */
DECLARE_FIELDFUNC(cwevent_runner3_advance)
{
  return sprintf(buffer, "%d", gameiter->event_data->advance[3]);
}

/* Field 62 */
DECLARE_FIELDFUNC(cwevent_play_on_batter)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%12s",
		 gameiter->event_data->play[0]);
}

/* Field 63 */
DECLARE_FIELDFUNC(cwevent_play_on_runner1)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%12s",
		 gameiter->event_data->play[1]);
}

/* Field 64 */
DECLARE_FIELDFUNC(cwevent_play_on_runner2)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%12s",
		 gameiter->event_data->play[2]);
}

/* Field 65 */
DECLARE_FIELDFUNC(cwevent_play_on_runner3)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%12s",
		 gameiter->event_data->play[3]);
}

/* Field 66 */
DECLARE_FIELDFUNC(cwevent_sb2_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->sb_flag[1] ? 'T' : 'F');
}

/* Field 67 */
DECLARE_FIELDFUNC(cwevent_sb3_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->sb_flag[2] ? 'T' : 'F');
}

/* Field 68 */
DECLARE_FIELDFUNC(cwevent_sbh_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->sb_flag[3] ? 'T' : 'F');
}

/* Field 69 */
DECLARE_FIELDFUNC(cwevent_cs2_flag)
{ 
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->cs_flag[1] ? 'T' : 'F');
}

/* Field 70 */
DECLARE_FIELDFUNC(cwevent_cs3_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->cs_flag[2] ? 'T' : 'F');
}

/* Field 71 */
DECLARE_FIELDFUNC(cwevent_csh_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->cs_flag[3] ? 'T' : 'F');
}

/* Field 72 */
DECLARE_FIELDFUNC(cwevent_po1_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->po_flag[1] ? 'T' : 'F');
}

/* Field 73 */
DECLARE_FIELDFUNC(cwevent_po2_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->po_flag[2] ? 'T' : 'F');
}

/* Field 74 */
DECLARE_FIELDFUNC(cwevent_po3_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 gameiter->event_data->po_flag[3] ? 'T' : 'F');
}

/*
 * In cwevent, the "responsible pitcher" is usually the pitcher responsible
 * at the beginning of the play.  However, on a play like 32(3)/FO.2-H(E2),
 * the runner scoring should be charged to the pitcher who was initially
 * responsible for the runner on third, and so that pitcher is listed
 * as the responsible pitcher so that stats can be calculated directly
 * from the cwevent output without having to reparse the play.
 */
char *
cwevent_responsible_pitcher(CWGameState *state, CWEventData *event_data,
			    int base)
{
  if (base == 3) {
    return state->pitchers[3];
  }
  else if (base == 2) {
    if (cw_event_runner_put_out(event_data, 3) &&
	event_data->fc_flag[3] && event_data->advance[2] >= 4) {
      return state->pitchers[3];
    }
    else {
      return state->pitchers[2];
    }
  }
  else {
    if (cw_event_runner_put_out(event_data, 3) &&
	event_data->fc_flag[3] && event_data->advance[2] >= 4) {
      return state->pitchers[2];
    }
    else if (cw_event_runner_put_out(event_data, 3) &&
	     !strcmp(state->runners[2], "") &&
	     event_data->advance[1] >= 4) {
      return state->pitchers[3];
    }
    else {
      return state->pitchers[1];
    }
  }
}

/* Field 75 */
DECLARE_FIELDFUNC(cwevent_responsible_pitcher1)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 cwevent_responsible_pitcher(gameiter->state, 
					     gameiter->event_data, 1));
}

/* Field 76 */
DECLARE_FIELDFUNC(cwevent_responsible_pitcher2)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 cwevent_responsible_pitcher(gameiter->state,
					     gameiter->event_data, 2));
}

/* Field 77 */
DECLARE_FIELDFUNC(cwevent_responsible_pitcher3)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 cwevent_responsible_pitcher(gameiter->state,
					     gameiter->event_data, 3));
}

/* Field 78 */
DECLARE_FIELDFUNC(cwevent_new_game_flag)
{
  CWEvent *event = gameiter->event->prev;
  while (event && !strcmp(event->event_text, "NP")) {
    event = event->prev;
  }
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 (event == NULL) ? 'T' : 'F');
}

/* Field 79 */
DECLARE_FIELDFUNC(cwevent_end_game_flag)
{
  CWEvent *event = gameiter->event->next;
  while (event && !strcmp(event->event_text, "NP")) {
    event = event->next;
  }
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 (event == NULL) ? 'T' : 'F');
}

/* Field 80 */
DECLARE_FIELDFUNC(cwevent_pr1_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "       %c", 
		 (gameiter->state->removed_for_pr[1]) ? 'T' : 'F');
}

/* Field 81 */
DECLARE_FIELDFUNC(cwevent_pr2_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "       %c",
		 (gameiter->state->removed_for_pr[2]) ? 'T' : 'F');
}

/* Field 82 */
DECLARE_FIELDFUNC(cwevent_pr3_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "       %c",
		 (gameiter->state->removed_for_pr[3]) ? 'T' : 'F');
}

/* Field 83 */
DECLARE_FIELDFUNC(cwevent_removed_runner1)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s", 
		 ((gameiter->state->removed_for_pr[1]) ?
		  gameiter->state->removed_for_pr[1] : ""));
}

/* Field 84 */
DECLARE_FIELDFUNC(cwevent_removed_runner2)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s", 
		 ((gameiter->state->removed_for_pr[2]) ?
		  gameiter->state->removed_for_pr[2] : ""));
}

/* Field 85 */
DECLARE_FIELDFUNC(cwevent_removed_runner3)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s", 
		 ((gameiter->state->removed_for_pr[3]) ?
		  gameiter->state->removed_for_pr[3] : ""));
}

/* Field 86 */
DECLARE_FIELDFUNC(cwevent_removed_batter)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		 ((gameiter->state->removed_for_ph) ? 
		  gameiter->state->removed_for_ph : ""));
}

/* Field 87 */
DECLARE_FIELDFUNC(cwevent_removed_batter_position)
{
  return sprintf(buffer, (ascii) ? "%d" : "%2d", 
		 ((gameiter->state->removed_for_ph) ? 
		  gameiter->state->removed_position : 0));
}

/* Field 88 */
DECLARE_FIELDFUNC(cwevent_putout1)
{
  return sprintf(buffer, "%d", gameiter->event_data->putouts[0]);
}

/* Field 89 */
DECLARE_FIELDFUNC(cwevent_putout2)
{
  return sprintf(buffer, "%d", gameiter->event_data->putouts[1]);
}

/* Field 90 */
DECLARE_FIELDFUNC(cwevent_putout3)
{
  return sprintf(buffer, "%d", gameiter->event_data->putouts[2]);
}

/* Field 91 */
DECLARE_FIELDFUNC(cwevent_assist1)
{
  return sprintf(buffer, "%d", gameiter->event_data->assists[0]);
}

/* Field 92 */
DECLARE_FIELDFUNC(cwevent_assist2)
{
  return sprintf(buffer, "%d", gameiter->event_data->assists[1]);
}

/* Field 93 */
DECLARE_FIELDFUNC(cwevent_assist3)
{
  return sprintf(buffer, "%d", gameiter->event_data->assists[2]);
}

/* Field 94 */
DECLARE_FIELDFUNC(cwevent_assist4)
{
  return sprintf(buffer, "%d", gameiter->event_data->assists[3]);
}

/* Field 95 */
DECLARE_FIELDFUNC(cwevent_assist5)
{
  return sprintf(buffer, "%d", gameiter->event_data->assists[4]);
}

/* Field 96 */
DECLARE_FIELDFUNC(cwevent_event_number)
{
  return sprintf(buffer, (ascii) ? "%d" : "%3d",		 
		 gameiter->state->event_count + 1);
}

static field_func function_ptrs[] = {
  cwevent_game_id,                /* 0 */
  cwevent_visiting_team,          /* 1 */
  cwevent_inning,                 /* 2 */
  cwevent_batting_team,           /* 3 */
  cwevent_outs,                   /* 4 */
  cwevent_balls,                  /* 5 */
  cwevent_strikes,                /* 6 */
  cwevent_pitches,                /* 7 */
  cwevent_visitor_score,          /* 8 */ 
  cwevent_home_score,             /* 9 */
  cwevent_batter,                 /* 10 */
  cwevent_batter_hand,            /* 11 */
  cwevent_res_batter,             /* 12 */
  cwevent_res_batter_hand,        /* 13 */
  cwevent_pitcher,                /* 14 */
  cwevent_pitcher_hand,           /* 15 */
  cwevent_res_pitcher,            /* 16 */
  cwevent_res_pitcher_hand,       /* 17 */
  cwevent_catcher,                /* 18 */
  cwevent_first_baseman,          /* 19 */
  cwevent_second_baseman,         /* 20 */
  cwevent_third_baseman,          /* 21 */
  cwevent_shortstop,              /* 22 */
  cwevent_left_fielder,           /* 23 */
  cwevent_center_fielder,         /* 24 */
  cwevent_right_fielder,          /* 25 */
  cwevent_runner_first,           /* 26 */
  cwevent_runner_second,          /* 27 */
  cwevent_runner_third,           /* 28 */
  cwevent_event_text,             /* 29 */
  cwevent_leadoff_flag,           /* 30 */
  cwevent_ph_flag,                /* 31 */
  cwevent_defensive_position,     /* 32 */
  cwevent_lineup_position,        /* 33 */
  cwevent_event_type,             /* 34 */
  cwevent_batter_event_flag,      /* 35 */
  cwevent_ab_flag,                /* 36 */
  cwevent_hit_value,              /* 37 */
  cwevent_sh_flag,                /* 38 */
  cwevent_sf_flag,                /* 39 */
  cwevent_outs_on_play,           /* 40 */
  cwevent_dp_flag,                /* 41 */
  cwevent_tp_flag,                /* 42 */
  cwevent_rbi_on_play,            /* 43 */
  cwevent_wp_flag,                /* 44 */
  cwevent_pb_flag,                /* 45 */
  cwevent_fielded_by,             /* 46 */
  cwevent_batted_ball_type,       /* 47 */
  cwevent_bunt_flag,              /* 48 */
  cwevent_foul_flag,              /* 49 */
  cwevent_hit_location,           /* 50 */
  cwevent_num_errors,             /* 51 */
  cwevent_error1_player,          /* 52 */
  cwevent_error1_type,            /* 53 */
  cwevent_error2_player,          /* 54 */
  cwevent_error2_type,            /* 55 */
  cwevent_error3_player,          /* 56 */
  cwevent_error3_type,            /* 57 */
  cwevent_batter_advance,         /* 58 */
  cwevent_runner1_advance,        /* 59 */
  cwevent_runner2_advance,        /* 60 */
  cwevent_runner3_advance,        /* 61 */
  cwevent_play_on_batter,         /* 62 */
  cwevent_play_on_runner1,        /* 63 */
  cwevent_play_on_runner2,        /* 64 */
  cwevent_play_on_runner3,        /* 65 */
  cwevent_sb2_flag,               /* 66 */
  cwevent_sb3_flag,               /* 67 */
  cwevent_sbh_flag,               /* 68 */
  cwevent_cs2_flag,               /* 69 */
  cwevent_cs3_flag,               /* 70 */
  cwevent_csh_flag,               /* 71 */
  cwevent_po1_flag,               /* 72 */
  cwevent_po2_flag,               /* 73 */
  cwevent_po3_flag,               /* 74 */
  cwevent_responsible_pitcher1,   /* 75 */
  cwevent_responsible_pitcher2,   /* 76 */
  cwevent_responsible_pitcher3,   /* 77 */
  cwevent_new_game_flag,          /* 78 */
  cwevent_end_game_flag,          /* 79 */
  cwevent_pr1_flag,               /* 80 */
  cwevent_pr2_flag,               /* 81 */
  cwevent_pr3_flag,               /* 82 */
  cwevent_removed_runner1,        /* 83 */
  cwevent_removed_runner2,        /* 84 */
  cwevent_removed_runner3,        /* 85 */
  cwevent_removed_batter,         /* 86 */
  cwevent_removed_batter_position, /* 87 */
  cwevent_putout1,                /* 88 */
  cwevent_putout2,                /* 89 */
  cwevent_putout3,                /* 90 */
  cwevent_assist1,                /* 91 */
  cwevent_assist2,                /* 92 */
  cwevent_assist3,                /* 93 */
  cwevent_assist4,                /* 94 */
  cwevent_assist5,                /* 95 */
  cwevent_event_number            /* 96 */
};

static char *field_names[] = {
  /*  0 */ "GAME_ID",
  /*  1 */ "AWAY_TEAM_ID",
  /*  2 */ "INN_CT",
  /*  3 */ "HOME_ID",
  /*  4 */ "OUTS_CT",
  /*  5 */ "BALLS_CT",
  /*  6 */ "STRIKES_CT",
  /*  7 */ "PITCH_SEQ_TX",
  /*  8 */ "AWAY_SCORE_CT",
  /*  9 */ "HOME_SCORE_CT",
  /* 10 */ "BAT_ID",
  /* 11 */ "BAT_HAND_CD",
  /* 12 */ "RESP_BAT_ID",
  /* 13 */ "RESP_BAT_HAND_CD",
  /* 14 */ "PIT_ID",
  /* 15 */ "PIT_HAND_CD",
  /* 16 */ "RESP_PIT_ID",
  /* 17 */ "RESP_PIT_HAND_CD",
  /* 18 */ "POS2_FLD_ID",
  /* 19 */ "POS3_FLD_ID",
  /* 20 */ "POS4_FLD_ID",
  /* 21 */ "POS5_FLD_ID",
  /* 22 */ "POS6_FLD_ID",
  /* 23 */ "POS7_FLD_ID",
  /* 24 */ "POS8_FLD_ID",
  /* 25 */ "POS9_FLD_ID",
  /* 26 */ "BASE1_RUN_ID",
  /* 27 */ "BASE2_RUN_ID",
  /* 28 */ "BASE3_RUN_ID",
  /* 29 */ "EVENT_TX",
  /* 30 */ "LEADOFF_FL",
  /* 31 */ "PH_FL",
  /* 32 */ "BAT_FLD_CD",
  /* 33 */ "BAT_LINEUP_ID",
  /* 34 */ "EVENT_CD",
  /* 35 */ "BAT_EVENT_FL",
  /* 36 */ "AB_FL",
  /* 37 */ "H_CD",
  /* 38 */ "SH_FL",
  /* 39 */ "SF_FL",
  /* 40 */ "EVENT_OUTS_CT",
  /* 41 */ "DP_FL",
  /* 42 */ "TP_FL",
  /* 43 */ "RBI_CT",
  /* 44 */ "WP_FL",
  /* 45 */ "PB_FL",
  /* 46 */ "FLD_ID",
  /* 47 */ "BATTEDBALL_CD",
  /* 48 */ "BUNT_FL",
  /* 49 */ "FOUL_FL",
  /* 50 */ "BATTEDBALL_LOC_TX",
  /* 51 */ "ERR_CT",
  /* 52 */ "ERR1_FLD_ID",
  /* 53 */ "ERR1_CD",
  /* 54 */ "ERR2_FLD_ID",
  /* 55 */ "ERR2_CD",
  /* 56 */ "ERR3_FLD_ID",
  /* 57 */ "ERR3_CD",
  /* 58 */ "BAT_DEST_ID",
  /* 59 */ "RUN1_DEST_ID",
  /* 60 */ "RUN2_DEST_ID",
  /* 61 */ "RUN3_DEST_ID",
  /* 62 */ "BAT_PLAY_TX",
  /* 63 */ "RUN1_PLAY_TX",
  /* 64 */ "RUN2_PLAY_TX",
  /* 65 */ "RUN3_PLAY_TX",
  /* 66 */ "RUN1_SB_FL",
  /* 67 */ "RUN2_SB_FL",
  /* 68 */ "RUN3_SB_FL",
  /* 69 */ "RUN1_CS_FL",
  /* 70 */ "RUN2_CS_FL",
  /* 71 */ "RUN3_CS_FL",
  /* 72 */ "RUN1_PK_FL",
  /* 73 */ "RUN2_PK_FL",
  /* 74 */ "RUN3_PK_FL",
  /* 75 */ "RUN1_RESP_PIT_ID",
  /* 76 */ "RUN2_RESP_PIT_ID",
  /* 77 */ "RUN3_RESP_PIT_ID",
  /* 78 */ "GAME_NEW_FL",
  /* 79 */ "GAME_END_FL",
  /* 80 */ "PR_RUN1_FL",
  /* 81 */ "PR_RUN2_FL",
  /* 82 */ "PR_RUN3_FL",
  /* 83 */ "REMOVED_FOR_PR_RUN1_ID",
  /* 84 */ "REMOVED_FOR_PR_RUN2_ID",
  /* 85 */ "REMOVED_FOR_PR_RUN3_ID",
  /* 86 */ "REMOVED_FOR_PH_BAT_ID",
  /* 87 */ "REMOVED_FOR_PH_BAT_FLD_ID",
  /* 88 */ "PO1_FLD_ID",
  /* 89 */ "PO2_FLD_ID",
  /* 90 */ "PO3_FLD_ID",
  /* 91 */ "ASS1_FLD_ID",
  /* 92 */ "ASS2_FLD_ID",
  /* 93 */ "ASS3_FLD_ID",
  /* 94 */ "ASS4_FLD_ID",
  /* 95 */ "ASS5_FLD_ID",
  /* 96 */ "EVENT_CT"
};

/*************************************************************************
 * Implementation of "extended" fields
 *************************************************************************/

/* Extended Field 0 */
DECLARE_FIELDFUNC(cwevent_home_team_id)
{
  return sprintf(buffer, (ascii) ? "\"%s\"" : "%-3s",
		 cw_game_info_lookup(gameiter->game, "hometeam"));
}

/* Extended Field 1 */
DECLARE_FIELDFUNC(cwevent_batting_team_id)
{
  if (gameiter->state->batting_team == 0) {
    return sprintf(buffer, (ascii) ? "\"%s\"" : "%-3s",
		   cw_game_info_lookup(gameiter->game, "visteam"));
  }
  else {
    return sprintf(buffer, (ascii) ? "\"%s\"" : "%-3s",
		   cw_game_info_lookup(gameiter->game, "hometeam"));
  }
}

/* Extended Field 2 */
DECLARE_FIELDFUNC(cwevent_fielding_team_id)
{
  if (gameiter->state->batting_team == 1) {
    return sprintf(buffer, (ascii) ? "\"%s\"" : "%-3s",
		   cw_game_info_lookup(gameiter->game, "visteam"));
  }
  else {
    return sprintf(buffer, (ascii) ? "\"%s\"" : "%-3s",
		   cw_game_info_lookup(gameiter->game, "hometeam"));
  }
}

/* Extended Field 3 */
DECLARE_FIELDFUNC(cwevent_half_inning)
{
  if (cw_game_info_lookup(gameiter->game, "htbf") &&
      !strcmp(cw_game_info_lookup(gameiter->game, "htbf"), "true")) {
    return sprintf(buffer, "%d", 1-gameiter->state->batting_team);
  }
  else {
    return sprintf(buffer, "%d", gameiter->state->batting_team);
  }
}

/* Extended Field 4 */
DECLARE_FIELDFUNC(cwevent_start_half_inning)
{
  CWEvent *event = gameiter->event->prev;

  while (event) {
    if (event->inning != gameiter->event->inning ||
	event->batting_team != gameiter->event->batting_team) {
      return sprintf(buffer, (ascii) ? "\"%c\"" : "%c", 'T');
    }
    else if (strcmp(event->event_text, "NP")) {
      return sprintf(buffer, (ascii) ? "\"%c\"" : "%c", 'F');
    }
    else {
      event = event->prev;
    }
  }

  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c", 'T');
}

/* Extended Field 5 */
DECLARE_FIELDFUNC(cwevent_end_half_inning)
{
  CWEvent *event = gameiter->event->next;

  while (event) {
    if (event->inning != gameiter->event->inning ||
	event->batting_team != gameiter->event->batting_team) {
      return sprintf(buffer, (ascii) ? "\"%c\"" : "%c", 'T');
    }
    else if (strcmp(event->event_text, "NP")) {
      return sprintf(buffer, (ascii) ? "\"%c\"" : "%c", 'F');
    }
    else {
      event = event->next;
    }
  }

  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c", 'T');
}

/* Extended Field 6 */
DECLARE_FIELDFUNC(cwevent_offense_score)
{ 
  return sprintf(buffer, (ascii) ? "%d" : "%2d", 
		 gameiter->state->score[gameiter->state->batting_team]);
}

/* Extended Field 7 */
DECLARE_FIELDFUNC(cwevent_defense_score)
{ 
  return sprintf(buffer, (ascii) ? "%d" : "%2d", 
		 gameiter->state->score[1-gameiter->state->batting_team]);
}

/* Extended Field 8 */
DECLARE_FIELDFUNC(cwevent_offense_score_inning)
{
  return sprintf(buffer, (ascii) ? "%d" : "%2d", 
		 gameiter->state->inning_score);
}

/* Extended Field 9 */
DECLARE_FIELDFUNC(cwevent_offense_batters_game)
{ 
  return sprintf(buffer, (ascii) ? "%d" : "%3d", 
		 gameiter->state->num_batters[gameiter->state->batting_team]);
}

/* Extended Field 10 */
DECLARE_FIELDFUNC(cwevent_offense_batters_inning)
{ 
  return sprintf(buffer, (ascii) ? "%d" : "%2d", 
		 gameiter->state->inning_batters);
}

/* Extended Field 11 */
DECLARE_FIELDFUNC(cwevent_start_pa_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c", 
		 ((gameiter->state->is_new_pa) ? 'T' : 'F'));
}

/* Extended Field 12 */
DECLARE_FIELDFUNC(cwevent_truncated_pa_flag)
{
  CWGameIterator *gi;

  if (cw_event_is_batter(gameiter->event_data)) {
    return sprintf(buffer, (ascii) ? "\"%c\"" : "%c", 'F');
  }

  gi = cw_gameiter_copy(gameiter);
  cw_gameiter_next(gi);
  while (gi->event != NULL &&
	 gi->state->inning == gameiter->state->inning &&
	 gi->state->batting_team == gameiter->state->batting_team) {
    if (strcmp(gi->event->event_text, "NP")) {
      if (cw_event_is_batter(gi->event_data)) {
	cw_gameiter_cleanup(gi);
	free(gi);
	return sprintf(buffer, (ascii) ? "\"%c\"" : "%c", 'F');
      }
    }
    cw_gameiter_next(gi);
  }

  cw_gameiter_cleanup(gi);
  free(gi);
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c", 'F');
}

/* Extended Field 13 */
DECLARE_FIELDFUNC(cwevent_base_state_start)
{
  return sprintf(buffer, "%d",
		 ((strcmp(gameiter->state->runners[3], "") ? 4 : 0) +
		  (strcmp(gameiter->state->runners[2], "") ? 2 : 0) +
		  (strcmp(gameiter->state->runners[1], "") ? 1 : 0)));
}

/* Extended Field 14 */
DECLARE_FIELDFUNC(cwevent_base_state_end)
{
  int r3 = 0, r2 = 0, r1 = 0;
  int base;

  for (base = 0; base <= 3; base++) {
    if (gameiter->event_data->advance[base] == 3)  r3 = 1;
    if (gameiter->event_data->advance[base] == 2)  r2 = 1;
    if (gameiter->event_data->advance[base] == 1)  r1 = 1;
  }

  return sprintf(buffer, "%d", 4*r3 + 2*r2 + r1);
}

/* Extended Field 15 */
DECLARE_FIELDFUNC(cwevent_runner1_defensive_position)
{
  if (!strcmp(gameiter->state->runners[1], "")) {
    return sprintf(buffer, "0");
  }
  else {
    return sprintf(buffer, (ascii) ? "%d" : "%2d", 
		   cw_gamestate_player_position(gameiter->state,
						gameiter->state->batting_team,
						gameiter->state->runners[1]));
  }
}

/* Extended Field 16 */
DECLARE_FIELDFUNC(cwevent_runner1_lineup_position)
{
  int lineupSlot;

  if (!strcmp(gameiter->state->runners[1], "")) {
    return sprintf(buffer, "0");
  }

  /* A bit of a kludge to handle case where pitcher enters the lineup
   * after DH goes away */
  lineupSlot = cw_gamestate_lineup_slot(gameiter->state,
					gameiter->state->batting_team,
					gameiter->state->runners[1]);
  if (lineupSlot > 0) {
    return sprintf(buffer, "%d", lineupSlot);
  }
  else {
    return sprintf(buffer, "%d",
	    (gameiter->state->num_batters[gameiter->state->batting_team]) % 9 + 1);
  }
}

/* Extended Field 17 */
DECLARE_FIELDFUNC(cwevent_runner2_defensive_position)
{
  if (!strcmp(gameiter->state->runners[2], "")) {
    return sprintf(buffer, "0");
  }
  else {
    return sprintf(buffer, (ascii) ? "%d" : "%2d", 
		   cw_gamestate_player_position(gameiter->state,
						gameiter->state->batting_team,
						gameiter->state->runners[2]));
  }
}

/* Extended Field 18 */
DECLARE_FIELDFUNC(cwevent_runner2_lineup_position)
{
  int lineupSlot;

  if (!strcmp(gameiter->state->runners[2], "")) {
    return sprintf(buffer, "0");
  }

  /* A bit of a kludge to handle case where pitcher enters the lineup
   * after DH goes away */
  lineupSlot = cw_gamestate_lineup_slot(gameiter->state,
					gameiter->state->batting_team,
					gameiter->state->runners[2]);
  if (lineupSlot > 0) {
    return sprintf(buffer, "%d", lineupSlot);
  }
  else {
    return sprintf(buffer, "%d",
	    (gameiter->state->num_batters[gameiter->state->batting_team]) % 9 + 1);
  }
}

/* Extended Field 19 */
DECLARE_FIELDFUNC(cwevent_runner3_defensive_position)
{
  if (!strcmp(gameiter->state->runners[3], "")) {
    return sprintf(buffer, "0");
  }
  else {
    return sprintf(buffer, (ascii) ? "%d" : "%2d", 
		   cw_gamestate_player_position(gameiter->state,
						gameiter->state->batting_team,
						gameiter->state->runners[3]));
  }
}

/* Extended Field 20 */
DECLARE_FIELDFUNC(cwevent_runner3_lineup_position)
{
  int lineupSlot;

  if (!strcmp(gameiter->state->runners[3], "")) {
    return sprintf(buffer, "0");
  }

  /* A bit of a kludge to handle case where pitcher enters the lineup
   * after DH goes away */
  lineupSlot = cw_gamestate_lineup_slot(gameiter->state,
					gameiter->state->batting_team,
					gameiter->state->runners[3]);
  if (lineupSlot > 0) {
    return sprintf(buffer, "%d", lineupSlot);
  }
  else {
    return sprintf(buffer, "%d",
	    (gameiter->state->num_batters[gameiter->state->batting_team]) % 9 + 1);
  }
}

/* Extended Field 21 */
DECLARE_FIELDFUNC(cwevent_pitches_balls)
{
  int balls = 0;
  char *pitch = gameiter->event->pitches;

  while (*pitch != '\0') {
    if (*pitch == 'B' || *pitch == 'H' || *pitch == 'I' || *pitch == 'P' || 
	*pitch == 'V') {
      balls++;
    }
    pitch++;
  }

  return sprintf(buffer, (ascii) ? "%d" : "%02d", balls);
}

/* Extended Field 22 */
DECLARE_FIELDFUNC(cwevent_pitches_balls_intentional)
{
  int balls = 0;
  char *pitch = gameiter->event->pitches;

  while (*pitch != '\0') {
    if (*pitch == 'I') {
      balls++;
    }
    pitch++;
  }

  return sprintf(buffer, (ascii) ? "%d" : "%02d", balls);
}

/* Extended Field 23 */
DECLARE_FIELDFUNC(cwevent_pitches_balls_pitchout)
{
  int balls = 0;
  char *pitch = gameiter->event->pitches;

  while (*pitch != '\0') {
    if (*pitch == 'P') {
      balls++;
    }
    pitch++;
  }

  return sprintf(buffer, (ascii) ? "%d" : "%02d", balls);
}

/* Extended Field 24 */
DECLARE_FIELDFUNC(cwevent_pitches_balls_other)
{
  int balls = 0;
  char *pitch = gameiter->event->pitches;

  while (*pitch != '\0') {
    if (*pitch == 'H' || *pitch == 'V') {
      balls++;
    }
    pitch++;
  }

  return sprintf(buffer, (ascii) ? "%d" : "%02d", balls);
}

/* Extended Field 25 */
DECLARE_FIELDFUNC(cwevent_pitches_strikes)
{
  int strikes = 0;
  char *pitch = gameiter->event->pitches;

  while (*pitch != '\0') {
    if (*pitch == 'C' || *pitch == 'F' || *pitch == 'K' || *pitch == 'L' ||
	*pitch == 'M' || *pitch == 'O' || *pitch == 'Q' || *pitch == 'R' ||
	*pitch == 'S' || *pitch == 'T' || *pitch == 'X' || *pitch == 'Y') {
      strikes++;
    }
    pitch++;
  }

  return sprintf(buffer, (ascii) ? "%d" : "%02d", strikes);
}

/* Extended Field 26 */
DECLARE_FIELDFUNC(cwevent_pitches_strikes_called)
{
  int strikes = 0;
  char *pitch = gameiter->event->pitches;

  while (*pitch != '\0') {
    if (*pitch == 'C') {
      strikes++;
    }
    pitch++;
  }

  return sprintf(buffer, (ascii) ? "%d" : "%02d", strikes);
}

/* Extended Field 27 */
DECLARE_FIELDFUNC(cwevent_pitches_strikes_swinging)
{
  int strikes = 0;
  char *pitch = gameiter->event->pitches;

  while (*pitch != '\0') {
    if (*pitch == 'M' || *pitch == 'S') {
      strikes++;
    }
    pitch++;
  }

  return sprintf(buffer, (ascii) ? "%d" : "%02d", strikes);
}

/* Extended Field 28 */
DECLARE_FIELDFUNC(cwevent_pitches_strikes_foul)
{
  int strikes = 0;
  char *pitch = gameiter->event->pitches;

  while (*pitch != '\0') {
    if (*pitch == 'F' || *pitch == 'L' || *pitch == 'O' || *pitch == 'T') {
      strikes++;
    }
    pitch++;
  }

  return sprintf(buffer, (ascii) ? "%d" : "%02d", strikes);
}

/* Extended Field 29 */
DECLARE_FIELDFUNC(cwevent_pitches_strikes_other)
{
  int strikes = 0;
  char *pitch = gameiter->event->pitches;

  while (*pitch != '\0') {
    if (*pitch == 'K') {
      strikes++;
    }
    pitch++;
  }

  return sprintf(buffer, (ascii) ? "%d" : "%02d", strikes);
}

/* Extended Field 30 */
DECLARE_FIELDFUNC(cwevent_runs_on_play)
{
  return sprintf(buffer, "%d", cw_event_runs_on_play(gameiter->event_data));
}

/* Extended Field 31 */
DECLARE_FIELDFUNC(cwevent_fielded_by_id)
{
  if (gameiter->event_data->fielded_by == 0) {
    return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s", "");
  }
  else {
    return sprintf(buffer, (ascii) ? "\"%s\"" : "%-8s",
		   gameiter->state->fielders[gameiter->event_data->fielded_by][1-gameiter->state->batting_team]);
  }
}

/* Extended Field 32 */
DECLARE_FIELDFUNC(cwevent_force_second_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 (gameiter->event_data->fc_flag[1] &&
		  (gameiter->event_data->gdp_flag || 
		   gameiter->event_data->force_flag)) ? 'T' : 'F');
}

/* Extended Field 33 */
DECLARE_FIELDFUNC(cwevent_force_third_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 (gameiter->event_data->fc_flag[2] &&
		  (gameiter->event_data->gdp_flag || 
		   gameiter->event_data->force_flag)) ? 'T' : 'F');
}

/* Extended Field 34 */
DECLARE_FIELDFUNC(cwevent_force_home_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 (gameiter->event_data->fc_flag[3] &&
		  (gameiter->event_data->gdp_flag || 
		   gameiter->event_data->force_flag)) ? 'T' : 'F');
}

/* Extended Field 35 */
DECLARE_FIELDFUNC(cwevent_safe_on_error_flag)
{
  return sprintf(buffer, (ascii) ? "\"%c\"" : "%c",
		 (gameiter->event_data->event_type == CW_EVENT_ERROR ||
		  (gameiter->event_data->event_type == CW_EVENT_GENERICOUT &&
		   gameiter->event_data->muff_flag[0])) ? 'T' : 'F');
}

/* Extended Field 36 */
DECLARE_FIELDFUNC(cwevent_batter_fate)
{
  return sprintf(buffer, "%d", cwevent_runner_fate(gameiter, 0));
}

/* Extended Field 37 */
DECLARE_FIELDFUNC(cwevent_runner1_fate)
{
  return sprintf(buffer, "%d", cwevent_runner_fate(gameiter, 1));
}

/* Extended Field 38 */
DECLARE_FIELDFUNC(cwevent_runner2_fate)
{
  return sprintf(buffer, "%d", cwevent_runner_fate(gameiter, 2));
}

/* Extended Field 39 */
DECLARE_FIELDFUNC(cwevent_runner3_fate)
{
  return sprintf(buffer, "%d", cwevent_runner_fate(gameiter, 3));
}

/* Extended Field 40 */
DECLARE_FIELDFUNC(cwevent_inning_future_runs)
{
  return sprintf(buffer, (ascii) ? "%d" : "%02d",
		 cwevent_future_runs(gameiter));
}

/* Extended Field 41 */
DECLARE_FIELDFUNC(cwevent_assist6)
{
  return sprintf(buffer, "%d", gameiter->event_data->assists[5]);
}

/* Extended Field 42 */
DECLARE_FIELDFUNC(cwevent_assist7)
{
  return sprintf(buffer, "%d", gameiter->event_data->assists[6]);
}

/* Extended Field 43 */
DECLARE_FIELDFUNC(cwevent_assist8)
{
  return sprintf(buffer, "%d", gameiter->event_data->assists[7]);
}

/* Extended Field 44 */
DECLARE_FIELDFUNC(cwevent_assist9)
{
  return sprintf(buffer, "%d", gameiter->event_data->assists[8]);
}

/* Extended Field 45 */
DECLARE_FIELDFUNC(cwevent_assist10)
{
  return sprintf(buffer, "%d", gameiter->event_data->assists[9]);
}

static field_func ext_function_ptrs[] = {
  /*  0 */ cwevent_home_team_id,             
  /*  1 */ cwevent_batting_team_id,          
  /*  2 */ cwevent_fielding_team_id,         
  /*  3 */ cwevent_half_inning,
  /*  4 */ cwevent_start_half_inning,
  /*  5 */ cwevent_end_half_inning,
  /*  6 */ cwevent_offense_score,
  /*  7 */ cwevent_defense_score,
  /*  8 */ cwevent_offense_score_inning,
  /*  9 */ cwevent_offense_batters_game,
  /* 10 */ cwevent_offense_batters_inning,
  /* 11 */ cwevent_start_pa_flag,
  /* 12 */ cwevent_truncated_pa_flag,
  /* 13 */ cwevent_base_state_start,
  /* 14 */ cwevent_base_state_end,
  /* 15 */ cwevent_runner1_defensive_position,
  /* 16 */ cwevent_runner1_lineup_position,
  /* 17 */ cwevent_runner2_defensive_position,
  /* 18 */ cwevent_runner2_lineup_position,
  /* 19 */ cwevent_runner3_defensive_position,
  /* 20 */ cwevent_runner3_lineup_position,
  /* 21 */ cwevent_pitches_balls,
  /* 22 */ cwevent_pitches_balls_intentional,
  /* 23 */ cwevent_pitches_balls_pitchout,
  /* 24 */ cwevent_pitches_balls_other,
  /* 25 */ cwevent_pitches_strikes,
  /* 26 */ cwevent_pitches_strikes_called,
  /* 27 */ cwevent_pitches_strikes_swinging,
  /* 28 */ cwevent_pitches_strikes_foul,
  /* 29 */ cwevent_pitches_strikes_other,
  /* 30 */ cwevent_runs_on_play,
  /* 31 */ cwevent_fielded_by_id,
  /* 32 */ cwevent_force_second_flag,
  /* 33 */ cwevent_force_third_flag,
  /* 34 */ cwevent_force_home_flag,
  /* 35 */ cwevent_safe_on_error_flag,
  /* 36 */ cwevent_batter_fate,
  /* 37 */ cwevent_runner1_fate,
  /* 38 */ cwevent_runner2_fate,
  /* 39 */ cwevent_runner3_fate,
  /* 40 */ cwevent_inning_future_runs,
  /* 41 */ cwevent_assist6,
  /* 42 */ cwevent_assist7,
  /* 43 */ cwevent_assist8,
  /* 44 */ cwevent_assist9,
  /* 45 */ cwevent_assist10
};

static char *ext_field_names[] = {
  /*  0 */ "HOME_TEAM_ID",
  /*  1 */ "BAT_TEAM_ID",
  /*  2 */ "FLD_TEAM_ID",
  /*  3 */ "INN_ID",
  /*  4 */ "INN_NEW_FL",
  /*  5 */ "INN_END_FL",
  /*  6 */ "START_BAT_SCORE_CT",
  /*  7 */ "START_OPP_SCORE_CT",
  /*  8 */ "INN_RUNS_CT",
  /*  9 */ "GAME_PA_CT",
  /* 10 */ "INN_PA_CT",
  /* 11 */ "PA_NEW_FL",
  /* 12 */ "PA_TRUNC_FL",
  /* 13 */ "START_BASES_CD",
  /* 14 */ "END_BASES_CD",
  /* 15 */ "RUN1_FLD_CD",
  /* 16 */ "RUN1_LINEUP_CD",
  /* 17 */ "RUN2_FLD_CD",
  /* 18 */ "RUN2_LINEUP_CD",
  /* 19 */ "RUN3_FLD_CD",
  /* 20 */ "RUN3_LINEUP_CD",
  /* 21 */ "PA_BALL_CT",
  /* 22 */ "PA_INTENT_BALL_CT",
  /* 23 */ "PA_PITCHOUT_BALL_CT",
  /* 24 */ "PA_OTHER_BALL_CT",
  /* 25 */ "PA_STRIKE_CT",
  /* 26 */ "PA_CALLED_STRIKE_CT",
  /* 27 */ "PA_SWINGMISS_STRIKE_CT",
  /* 28 */ "PA_FOUL_STRIKE_CT",
  /* 29 */ "PA_OTHER_STRIKE_CT",
  /* 30 */ "EVENT_RUNS_CT",
  /* 31 */ "FLD_ID",
  /* 32 */ "BASE2_FORCE_FL",
  /* 33 */ "BASE3_FORCE_FL",
  /* 34 */ "BASE4_FORCE_FL",
  /* 35 */ "BAT_SAFE_ERR_FL",
  /* 36 */ "BAT_FATE_ID",
  /* 37 */ "RUN1_FATE_ID",
  /* 38 */ "RUN2_FATE_ID",
  /* 39 */ "RUN3_FATE_ID",
  /* 40 */ "FATE_RUNS_CT",
  /* 41 */ "ASS6_FLD_ID",
  /* 42 */ "ASS7_FLD_ID",
  /* 43 */ "ASS8_FLD_ID",
  /* 44 */ "ASS9_FLD_ID",
  /* 45 */ "ASS10_FLD_ID"
};

void
cwevent_process_game(CWGame *game, CWRoster *visitors, CWRoster *home) 
{
  char *buf;
  char output_line[4096];
  int i, comma;
  CWGameIterator *gameiter = cw_gameiter_create(game);

  while (gameiter->event != NULL) {
    CWEvent *event = gameiter->event;

    if (!strcmp(event->event_text, "NP")) {
      cw_gameiter_next(gameiter);
      continue;
    }

    comma = 0;
    strcpy(output_line, "");
    buf = output_line;
    for (i = 0; i <= max_field; i++) {
      if (fields[i]) {
	if (ascii && comma) {
	  *(buf++) = ',';
	}
	else {
	  comma = 1;
	}
	buf += (*function_ptrs[i])(buf, gameiter, visitors, home);
      }
    }

    for (i = 0; i <= max_ext_field; i++) {
      if (ext_fields[i]) {
	if (ascii && comma) {
	  *(buf++) = ',';
	}
	else {
	  comma = 1;
	}
	buf += (*ext_function_ptrs[i])(buf, gameiter, visitors, home);
      }
    };

    printf(output_line);
    printf("\n");

    cw_gameiter_next(gameiter);
  }

  cw_gameiter_cleanup(gameiter);
  free(gameiter);
}

void (*cwtools_process_game)(CWGame *, CWRoster *, CWRoster *) = cwevent_process_game;

void
cwevent_print_help(void)
{
  fprintf(stderr, "\n\ncwevent generates files suitable for use by dBase or Lotus-like programs\n");
  fprintf(stderr, "Each record describes one event.\n");
  fprintf(stderr, "Usage: cwevent [options] eventfile...\n");
  fprintf(stderr, "options:\n");
  fprintf(stderr, "  -h        print this help\n");
  fprintf(stderr, "  -i id     only process game given by id\n");
  fprintf(stderr, "  -y year   Year to process (for teamyyyy and aaayyyy.ros).\n");
  fprintf(stderr, "  -s start  Earliest date to process (mmdd).\n");
  fprintf(stderr, "  -e end    Last date to process (mmdd).\n");
  fprintf(stderr, "  -a        generate Ascii-delimited format files (default)\n");
  fprintf(stderr, "  -ft       generate Fortran format files\n");
  fprintf(stderr, "  -f flist  give list of fields to output\n");
  fprintf(stderr, "              Default is 0-6,8-9,12-13,16-17,26-40,43-45,51,58-61\n");
  fprintf(stderr, "  -x flist  give list of extended fields to output\n");
  fprintf(stderr, "              Default is none\n");
  fprintf(stderr, "  -d        print list of field numbers and descriptions\n");
  fprintf(stderr, "  -q        operate quietly; do not output progress messages\n");
  fprintf(stderr, "  -n        print field names in first row of output\n\n");

  exit(0);
}

void (*cwtools_print_help)(void) = cwevent_print_help;

void
cwevent_print_field_list(void)
{
  fprintf(stderr, "\nThese are the available fields and the numbers to use with the -f option\n");
  fprintf(stderr, "to name them.  The default fields are marked with an asterisk (*).\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "number  field\n");
  fprintf(stderr, "------  -----\n");
  fprintf(stderr, "0       game id*\n");
  fprintf(stderr, "1       visiting team*\n");
  fprintf(stderr, "2       inning*\n");
  fprintf(stderr, "3       batting team*\n");
  fprintf(stderr, "4       outs*\n");
  fprintf(stderr, "5       balls*\n");
  fprintf(stderr, "6       strikes*\n");
  fprintf(stderr, "7       pitch sequence\n");
  fprintf(stderr, "8       vis score*\n");
  fprintf(stderr, "9       home score*\n");
  fprintf(stderr, "10      batter\n");
  fprintf(stderr, "11      batter hand\n");
  fprintf(stderr, "12      res batter*\n");
  fprintf(stderr, "13      res batter hand*\n");
  fprintf(stderr, "14      pitcher\n");
  fprintf(stderr, "15      pitcher hand\n");
  fprintf(stderr, "16      res pitcher*\n");
  fprintf(stderr, "17      res pitcher hand*\n");
  fprintf(stderr, "18      catcher\n");
  fprintf(stderr, "19      first base\n");
  fprintf(stderr, "20      second base\n");
  fprintf(stderr, "21      third base\n");
  fprintf(stderr, "22      shortstop\n");
  fprintf(stderr, "23      left field\n");
  fprintf(stderr, "24      center field\n");
  fprintf(stderr, "25      right field\n");
  fprintf(stderr, "26      first runner*\n");
  fprintf(stderr, "27      second runner*\n");
  fprintf(stderr, "28      third runner*\n");
  fprintf(stderr, "29      event text*\n");
  fprintf(stderr, "30      leadoff flag*\n");
  fprintf(stderr, "31      pinchhit flag*\n");
  fprintf(stderr, "32      defensive position*\n");
  fprintf(stderr, "33      lineup position*\n");
  fprintf(stderr, "34      event type*\n");
  fprintf(stderr, "35      batter event flag*\n");
  fprintf(stderr, "36      ab flag*\n");
  fprintf(stderr, "37      hit value*\n");
  fprintf(stderr, "38      SH flag*\n");
  fprintf(stderr, "39      SF flag*\n");
  fprintf(stderr, "40      outs on play*\n");
  fprintf(stderr, "41      double play flag\n");
  fprintf(stderr, "42      triple play flag\n");
  fprintf(stderr, "43      RBI on play*\n");
  fprintf(stderr, "44      wild pitch flag*\n");
  fprintf(stderr, "45      passed ball flag*\n");
  fprintf(stderr, "46      fielded by\n");
  fprintf(stderr, "47      batted ball type\n");
  fprintf(stderr, "48      bunt flag\n");
  fprintf(stderr, "49      foul flag\n");
  fprintf(stderr, "50      hit location\n");
  fprintf(stderr, "51      num errors*\n");
  fprintf(stderr, "52      1st error player\n");
  fprintf(stderr, "53      1st error type\n");
  fprintf(stderr, "54      2nd error player\n");
  fprintf(stderr, "55      2nd error type\n");
  fprintf(stderr, "56      3rd error player\n");
  fprintf(stderr, "57      3rd error type\n");
  fprintf(stderr, "58      batter dest* (5 if scores and unearned, 6 if team unearned)\n");
  fprintf(stderr, "59      runner on 1st dest* (5 if scores and unearned, 6 if team unearned)\n");
  fprintf(stderr, "60      runner on 2nd dest* (5 if scores and unearned, 6 if team unearned)\n");
  fprintf(stderr, "61      runner on 3rd dest* (5 if scores and unearned, 6 if team unearned)\n");
  fprintf(stderr, "62      play on batter\n");
  fprintf(stderr, "63      play on runner on 1st\n");
  fprintf(stderr, "64      play on runner on 2nd\n");
  fprintf(stderr, "65      play on runner on 3rd\n");
  fprintf(stderr, "66      SB for runner on 1st flag\n");
  fprintf(stderr, "67      SB for runner on 2nd flag\n");
  fprintf(stderr, "68      SB for runner on 3rd flag\n");
  fprintf(stderr, "69      CS for runner on 1st flag\n");
  fprintf(stderr, "70      CS for runner on 2nd flag\n");
  fprintf(stderr, "71      CS for runner on 3rd flag\n");
  fprintf(stderr, "72      PO for runner on 1st flag\n");
  fprintf(stderr, "73      PO for runner on 2nd flag\n");
  fprintf(stderr, "74      PO for runner on 3rd flag\n");
  fprintf(stderr, "75      Responsible pitcher for runner on 1st\n");
  fprintf(stderr, "76      Responsible pitcher for runner on 2nd\n");
  fprintf(stderr, "77      Responsible pitcher for runner on 3rd\n");
  fprintf(stderr, "78      New Game Flag\n");
  fprintf(stderr, "79      End Game Flag\n");
  fprintf(stderr, "80      Pinch-runner on 1st\n");
  fprintf(stderr, "81      Pinch-runner on 2nd\n");
  fprintf(stderr, "82      Pinch-runner on 3rd\n");
  fprintf(stderr, "83      Runner removed for pinch-runner on 1st\n");
  fprintf(stderr, "84      Runner removed for pinch-runner on 2nd\n");
  fprintf(stderr, "85      Runner removed for pinch-runner on 3rd\n");
  fprintf(stderr, "86      Batter removed for pinch-hitter\n");
  fprintf(stderr, "87      Position of batter removed for pinch-hitter\n");
  fprintf(stderr, "88      Fielder with First Putout (0 if none)\n");
  fprintf(stderr, "89      Fielder with Second Putout (0 if none)\n");
  fprintf(stderr, "90      Fielder with Third Putout (0 if none)\n");
  fprintf(stderr, "91      Fielder with First Assist (0 if none)\n");
  fprintf(stderr, "92      Fielder with Second Assist (0 if none)\n");
  fprintf(stderr, "93      Fielder with Third Assist (0 if none)\n");
  fprintf(stderr, "94      Fielder with Fourth Assist (0 if none)\n");
  fprintf(stderr, "95      Fielder with Fifth Assist (0 if none)\n");
  fprintf(stderr, "96      event num\n");
  
  fprintf(stderr, "\n");

  fprintf(stderr, "These additional fields are available in this version of cwevent.\n");
  fprintf(stderr, "These are specified using the -x option, and appear in the output\n");
  fprintf(stderr, "after all fields specified with -f. By default, none of these\n");
  fprintf(stderr, "fields are output.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "number  field\n");
  fprintf(stderr, "------  -----\n");
  fprintf(stderr, "0       home team id\n");
  fprintf(stderr, "1       batting team id\n");
  fprintf(stderr, "2       fielding team id\n");
  fprintf(stderr, "3       half inning (differs from batting team if home team bats first)\n");
  fprintf(stderr, "4       start of half inning flag\n");
  fprintf(stderr, "5       end of half inning flag\n");
  fprintf(stderr, "6       score for team on offense\n");
  fprintf(stderr, "7       score for team on defense\n");
  fprintf(stderr, "8       runs scored in this half inning\n");
  fprintf(stderr, "9       number of plate appearances in game for team on offense\n");
  fprintf(stderr, "10      number of plate appearances in inning for team on offense\n");
  fprintf(stderr, "11      start of plate appearance flag\n");
  fprintf(stderr, "12      truncated plate appearance flag\n");
  fprintf(stderr, "13      base state at start of play\n");
  fprintf(stderr, "14      base state at end of play\n");
  fprintf(stderr, "15      defensive position of runner on first\n");
  fprintf(stderr, "16      lineup position of runner on first\n");
  fprintf(stderr, "17      defensive position of runner on second\n");
  fprintf(stderr, "18      lineup position of runner on second\n");
  fprintf(stderr, "19      defensive position of runner on third\n");
  fprintf(stderr, "20      lineup position of runner on third\n");
  fprintf(stderr, "21      number of balls in plate appearance\n");
  fprintf(stderr, "22      number of intentional balls in plate appearance\n");
  fprintf(stderr, "23      number of pitchouts in plate appearance\n");
  fprintf(stderr, "24      number of other balls in plate appearance\n");
  fprintf(stderr, "25      number of strikes in plate appearance\n");
  fprintf(stderr, "26      number of called strikes in plate appearance\n");
  fprintf(stderr, "27      number of swinging strikes in plate appearance\n");
  fprintf(stderr, "28      number of foul balls in plate appearance\n");
  fprintf(stderr, "29      number of other strikes in plate appearance\n");
  fprintf(stderr, "30      number of runs on play\n");
  fprintf(stderr, "31      id of player fielding batted ball\n");
  fprintf(stderr, "32      force play at second flag\n");
  fprintf(stderr, "33      force play at third flag\n");
  fprintf(stderr, "34      force play at home flag\n");
  fprintf(stderr, "35      batter safe on error flag\n");
  fprintf(stderr, "36      fate of batter (base ultimately advanced to)\n");
  fprintf(stderr, "37      fate of runner on first\n");
  fprintf(stderr, "38      fate of runner on second\n");
  fprintf(stderr, "39      fate of runner on third\n");
  fprintf(stderr, "40      runs scored in half inning after this event\n");
  fprintf(stderr, "41      fielder with sixth assist\n");
  fprintf(stderr, "42      fielder with seventh assist\n");
  fprintf(stderr, "43      fielder with eighth assist\n");
  fprintf(stderr, "44      fielder with ninth assist\n");
  fprintf(stderr, "45      fielder with tenth assist\n");
  
  exit(0);
}

void (*cwtools_print_field_list)(void) = cwevent_print_field_list;

void
cwevent_print_welcome_message(char *argv0)
{
  fprintf(stderr, 
	  "\nChadwick expanded event descriptor, version " VERSION); 
  fprintf(stderr, "\n  Type '%s -h' for help.\n", argv0);
  fprintf(stderr, "This is free software, " 
	  "subject to the terms of the GNU GPL license.\n\n");
}

void (*cwtools_print_welcome_message)(char *) = cwevent_print_welcome_message;

void
cwevent_initialize(void)
{
  int i, comma = 0;
  char output_line[4096];
  char *buf;

  if (!ascii || !print_header) {
    return;
  }

  strcpy(output_line, "");
  buf = output_line;

  for (i = 0; i <= max_field; i++) {
    if (fields[i]) {
      if (ascii && comma) {
	*(buf++) = ',';
      }
      else {
	comma = 1;
      }
      buf += sprintf(buf, "\"%s\"", field_names[i]);
    }
  }

  for (i = 0; i <= max_ext_field; i++) {
    if (ext_fields[i]) {
      if (ascii && comma) {
	*(buf++) = ',';
      }
      else {
	comma = 1;
      }
      buf += sprintf(buf, "\"%s\"", ext_field_names[i]);
    }
  }

  printf(output_line);
  printf("\n");
}

void (*cwtools_initialize)(void) = cwevent_initialize;

void
cwevent_cleanup(void)
{
}

void (*cwtools_cleanup)(void) = cwevent_cleanup;

extern char year[5];
extern char first_date[5];
extern char last_date[5];
extern char game_id[20];
extern int ascii;
extern int quiet;

extern void
cwtools_parse_field_list(char *text, int max_field, int *fields);

int
cwevent_parse_command_line(int argc, char *argv[])
{
  int i;
  strcpy(year, "");

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-a")) {
      ascii = 1;
    }
    else if (!strcmp(argv[i], "-d")) {
      (*cwtools_print_welcome_message)(argv[0]);
      (*cwtools_print_field_list)();
    }
    else if (!strcmp(argv[i], "-e")) {
      if (++i < argc) {
	strncpy(last_date, argv[i], 4);
      }
    }
    else if (!strcmp(argv[i], "-h")) {
      (*cwtools_print_welcome_message)(argv[0]);
      (*cwtools_print_help)();
    }
    else if (!strcmp(argv[i], "-q")) {
      quiet = 1;
    }
    else if (!strcmp(argv[i], "-i")) {
      if (++i < argc) {
	strncpy(game_id, argv[i], 19);
      }
    }
    else if (!strcmp(argv[i], "-f")) {
      if (++i < argc) {
	cwtools_parse_field_list(argv[i], max_field, fields);
      }
    }
    else if (!strcmp(argv[i], "-n")) {
      print_header = 1;
    }
    else if (!strcmp(argv[i], "-ft")) {
      ascii = 0;
    }
    else if (!strcmp(argv[i], "-s")) {
      if (++i < argc) {
	strncpy(first_date, argv[i], 4);
      }
    }
    else if (!strcmp(argv[i], "-x")) {
      if (++i < argc) {
	cwtools_parse_field_list(argv[i], max_ext_field, ext_fields);
      }
    }
    else if (!strcmp(argv[i], "-y")) {
      if (++i < argc) {
	strncpy(year, argv[i], 5);
      }
    }
    else if (argv[i][0] == '-') {
      fprintf(stderr, "*** Invalid option '%s'.\n", argv[i]);
      exit(1);
    }
    else {
      break;
    }
  }

  return i;
}

int (*cwtools_parse_command_line)(int, char *argv[]) = cwevent_parse_command_line;
