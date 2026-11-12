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

/* No data lives in this overlay (the overlay data area is not initialized
 * at load time): write_failed is owned by the root, see save.c.
 */

static void
r_write(fd, buf, n)
        int fd;
        char *buf;
        int n;
{
	if (!write_failed) {
		if (write(fd, buf, n) != n) {
			message("write() failed, don't know why", 0);
			sound_bell();
			write_failed = 1;
		}
	}
}

static void
write_string(s, fd)
        char *s;
        int fd;
{
	short n;

	n = strlen(s) + 1;
	xxxx(s, n);
	r_write(fd, (char *) &n, sizeof(short));
	r_write(fd, s, n);
}

static void
write_pack(pack, fd)
        object *pack;
        int fd;
{
	object t;

	while ((pack = pack->next_object)) {
		r_write(fd, (char *) pack, sizeof(object));
	}
	t.ichar = t.what_is = 0;
	r_write(fd, (char *) &t, sizeof(object));
}

static void
r_read(fd, buf, n)
        int fd;
        char *buf;
        int n;
{
	if (read(fd, buf, n) != n) {
		clean_up("read() failed, don't know why");
	}
}

static void
rw_dungeon(fd, rw)
        int fd;
        boolean rw;
{
	int i, j;
	char buf[DCOLS];

	for (i = 0; i < DROWS; i++) {
		if (rw) {
			r_write(fd, (char *) dungeon[i], (DCOLS * sizeof(dungeon[0][0])));
			for (j = 0; j < DCOLS; j++) {
				buf[j] = mvinch(i, j);
			}
			r_write(fd, buf, DCOLS);
		} else {
			r_read(fd, (char *) dungeon[i], (DCOLS * sizeof(dungeon[0][0])));
			r_read(fd, buf, DCOLS);
			for (j = 0; j < DCOLS; j++) {
				mvaddch(i, j, buf[j]);
			}
		}
	}
}

static void
read_string(s, fd)
        char *s;
        int fd;
{
	short n;

	r_read(fd, (char *) &n, sizeof(short));
	r_read(fd, s, n);
	xxxx(s, n);
}

static void
rw_id(id_table, fd, n, wr)
        struct id id_table[];
        int fd;
        int n;
        boolean wr;
{
	int i;

	for (i = 0; i < n; i++) {
		if (wr) {
			r_write(fd, (char *) &(id_table[i].value), sizeof(short));
			r_write(fd, (char *) &(id_table[i].id_status),
				sizeof(unsigned short));
			write_string(id_table[i].title, fd);
		} else {
			r_read(fd, (char *) &(id_table[i].value), sizeof(short));
			r_read(fd, (char *) &(id_table[i].id_status),
				sizeof(unsigned short));
			read_string(id_table[i].title, fd);
		}
	}
}

static void
rw_rooms(fd, rw)
        int fd;
        boolean rw;
{
	int i;

	for (i = 0; i < MAXROOMS; i++) {
		rw ? r_write(fd, (char *) (rooms + i), sizeof(room)) :
			r_read(fd, (char *) (rooms + i), sizeof(room));
	}
}

static void
real_save_into_file(sfile)
        char *sfile;
{
	int fd;
	int file_id;
	char name_buffer[80];
	char *hptr;
	struct rogue_time rt_buf;

	if (sfile[0] == '~') {
	        hptr = md_getenv("HOME");
		if (hptr) {
			(void) strcpy(name_buffer, hptr);
			(void) strcat(name_buffer, sfile+1);
			sfile = name_buffer;
		}
	}
	if (	((fd = creat(sfile, 0666)) < 0) ||
			((file_id = md_get_file_id(sfile)) == -1)) {
		message("problem accessing the save file", 0);
		return;
	}
	md_ignore_signals();
	write_failed = 0;
	(void) xxx(1);
	r_write(fd, (char *) &detect_monster, sizeof(detect_monster));
	r_write(fd, (char *) &cur_level, sizeof(cur_level));
	r_write(fd, (char *) &max_level, sizeof(max_level));
	write_string(hunger_str, fd);
	write_string(login_name, fd);
	r_write(fd, (char *) &party_room, sizeof(party_room));
	write_pack(&level_monsters, fd);
	write_pack(&level_objects, fd);
	r_write(fd, (char *) &file_id, sizeof(file_id));
	rw_dungeon(fd, 1);
	r_write(fd, (char *) &foods, sizeof(foods));
	r_write(fd, (char *) &rogue, sizeof(fighter));
	write_pack(&rogue.pack, fd);
	rw_id(id_potions, fd, POTIONS, 1);
	rw_id(id_scrolls, fd, SCROLS, 1);
	rw_id(id_wands, fd, WANDS, 1);
	rw_id(id_rings, fd, RINGS, 1);
	r_write(fd, (char *) traps, (MAX_TRAPS * sizeof(trap)));
	r_write(fd, (char *) is_wood, (WANDS * sizeof(boolean)));
	r_write(fd, (char *) &cur_room, sizeof(cur_room));
	rw_rooms(fd, 1);
	r_write(fd, (char *) &being_held, sizeof(being_held));
	r_write(fd, (char *) &bear_trap, sizeof(bear_trap));
	r_write(fd, (char *) &halluc, sizeof(halluc));
	r_write(fd, (char *) &blind, sizeof(blind));
	r_write(fd, (char *) &confused, sizeof(confused));
	r_write(fd, (char *) &levitate, sizeof(levitate));
	r_write(fd, (char *) &haste_self, sizeof(haste_self));
	r_write(fd, (char *) &see_invisible, sizeof(see_invisible));
	r_write(fd, (char *) &detect_monster, sizeof(detect_monster));
	r_write(fd, (char *) &score_only, sizeof(score_only));
	r_write(fd, (char *) &m_moves, sizeof(m_moves));
	md_gct(&rt_buf);
	rt_buf.second += 10;		/* allow for some processing time */
	r_write(fd, (char *) &rt_buf, sizeof(rt_buf));
	close(fd);

	if (write_failed) {
		(void) md_df(sfile);	/* delete file */
	} else {
		clean_up("");
	}
}

static void
real_save_game()
{
	char fname[64];

	if (!get_input_line("file name?", save_file, fname, "game not saved",
			0, 1)) {
		return;
	}
	check_message();
	message(fname, 0);
	real_save_into_file(fname);
}

static void
read_pack(pack, fd, is_rogue)
        object *pack;
        int fd;
        boolean is_rogue;
{
	object read_obj, *new_obj;

	for (;;) {
		r_read(fd, (char *) &read_obj, sizeof(object));
		if (read_obj.ichar == 0) {
			pack->next_object = (object *) 0;
			break;
		}
		new_obj = alloc_object();
		*new_obj = read_obj;
		if (is_rogue) {
			if (new_obj->in_use_flags & BEING_WORN) {
					do_wear(new_obj);
			} else if (new_obj->in_use_flags & BEING_WIELDED) {
					do_wield(new_obj);
			} else if (new_obj->in_use_flags & (ON_EITHER_HAND)) {
				do_put_on(new_obj,
					((new_obj->in_use_flags & ON_LEFT_HAND) ? 1 : 0));
			}
		}
		pack->next_object = new_obj;
		pack = new_obj;
	}
}

static boolean
has_been_touched(saved_time, mod_time)
        struct rogue_time *saved_time, *mod_time;
{
	if (saved_time->year < mod_time->year) {
		return(1);
	} else if (saved_time->year > mod_time->year) {
		return(0);
	}
	if (saved_time->month < mod_time->month) {
		return(1);
	} else if (saved_time->month > mod_time->month) {
		return(0);
	}
	if (saved_time->day < mod_time->day) {
		return(1);
	} else if (saved_time->day > mod_time->day) {
		return(0);
	}
	if (saved_time->hour < mod_time->hour) {
		return(1);
	} else if (saved_time->hour > mod_time->hour) {
		return(0);
	}
	if (saved_time->minute < mod_time->minute) {
		return(1);
	} else if (saved_time->minute > mod_time->minute) {
		return(0);
	}
	if (saved_time->second < mod_time->second) {
		return(1);
	}
	return(0);
}

static void
real_restore(fname)
        char *fname;
{
	int fd;
	struct rogue_time saved_time, mod_time;
	char buf[4];
	char tbuf[40];
	int new_file_id, saved_file_id;

	new_file_id = md_get_file_id(fname);
	if (new_file_id == -1) {
		clean_up("no save file");
        }
        fd = open(fname, 0);
        if (fd < 0) {
		clean_up("cannot open file");
        }
	if (md_link_count(fname) > 1) {
		clean_up("file has link");
	}
	(void) xxx(1);
	r_read(fd, (char *) &detect_monster, sizeof(detect_monster));
	r_read(fd, (char *) &cur_level, sizeof(cur_level));
	r_read(fd, (char *) &max_level, sizeof(max_level));
	read_string(hunger_str, fd);

	(void) strcpy(tbuf, login_name);
	read_string(login_name, fd);
	if (strcmp(tbuf, login_name)) {
		clean_up("you're not the original player");
	}

	r_read(fd, (char *) &party_room, sizeof(party_room));
	read_pack(&level_monsters, fd, 0);
	read_pack(&level_objects, fd, 0);
	r_read(fd, (char *) &saved_file_id, sizeof(saved_file_id));
	if (new_file_id != saved_file_id) {
		clean_up("sorry, saved game is not in the same file");
	}
	rw_dungeon(fd, 0);
	r_read(fd, (char *) &foods, sizeof(foods));
	r_read(fd, (char *) &rogue, sizeof(fighter));
	read_pack(&rogue.pack, fd, 1);
	rw_id(id_potions, fd, POTIONS, 0);
	rw_id(id_scrolls, fd, SCROLS, 0);
	rw_id(id_wands, fd, WANDS, 0);
	rw_id(id_rings, fd, RINGS, 0);
	r_read(fd, (char *) traps, (MAX_TRAPS * sizeof(trap)));
	r_read(fd, (char *) is_wood, (WANDS * sizeof(boolean)));
	r_read(fd, (char *) &cur_room, sizeof(cur_room));
	rw_rooms(fd, 0);
	r_read(fd, (char *) &being_held, sizeof(being_held));
	r_read(fd, (char *) &bear_trap, sizeof(bear_trap));
	r_read(fd, (char *) &halluc, sizeof(halluc));
	r_read(fd, (char *) &blind, sizeof(blind));
	r_read(fd, (char *) &confused, sizeof(confused));
	r_read(fd, (char *) &levitate, sizeof(levitate));
	r_read(fd, (char *) &haste_self, sizeof(haste_self));
	r_read(fd, (char *) &see_invisible, sizeof(see_invisible));
	r_read(fd, (char *) &detect_monster, sizeof(detect_monster));
	r_read(fd, (char *) &score_only, sizeof(score_only));
	r_read(fd, (char *) &m_moves, sizeof(m_moves));
	r_read(fd, (char *) &saved_time, sizeof(saved_time));

	if (read(fd, buf, 1) > 0) {
		clear();
		clean_up("extra characters in file");
	}

	md_gfmt(fname, &mod_time);	/* get file modification time */

	if (has_been_touched(&saved_time, &mod_time)) {
		clear();
		clean_up("sorry, file has been touched");
	}
	if (!md_df(fname)) {
		clean_up("cannot delete file");
	}
	msg_cleared = 0;
	ring_stats(0);
	close(fd);
}

int
ovmain(op, arg)
        int op;
        void *arg;
{
	switch (op) {
	case 1:
		real_save_game();
		break;
	case 2:
		real_save_into_file((char *) arg);
		break;
	case 3:
		real_restore((char *) arg);
		break;
	}
	return 0;
}
