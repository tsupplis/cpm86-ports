/*
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */
#include <string.h>
#include "rogue.h"

char login_name[MAX_OPT_LEN];
char *nick_name = (char *) 0;
char *rest_file = 0;
char cant_int = 0;
char did_int = 0;
char score_only;
char init_curses = 0;
char save_is_interactive = 1;
char ask_quit = 1;
char no_skull = 0;
char passgo = 0;
char *error_file = "rogue.esv";
char *byebye_string = "Okay, bye bye!";

int
init(argc, argv)
        int argc;
        char *argv[];
{
	return ovloader("init", argc, argv);
}

void
clean_up(estr)
        char *estr;
{
	if (save_is_interactive) {
		if (init_curses) {
			move(DROWS-1, 0);
			refresh();
			stop_window();
		}
		printf("\n%s\n", estr);
	}
	md_exit(0);
}

void
start_window()
{
	crmode();
	noecho();
#ifndef BAD_NONL
	nonl();
#endif
	md_control_keybord(0);
}

void
stop_window()
{
	endwin();
	md_control_keybord(1);
}

void
byebye(int sig)
{
	md_ignore_signals();
	if (ask_quit) {
		quit(1);
	} else {
		clean_up(byebye_string);
	}
	md_heed_signals();
}

void
onintr(int sig)
{
	md_ignore_signals();
	if (cant_int) {
		did_int = 1;
	} else {
		check_message();
		message("interrupt", 1);
	}
	md_heed_signals();
}

void
error_save(int sig)
{
	save_is_interactive = 0;
	save_into_file(error_file);
	clean_up("");
}
