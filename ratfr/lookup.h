
#ifndef LOOKUP_H
#define LOOKUP_H

/*
 * from K&R "The C Programming language"
 * Table lookup routines
 * structure and definitions
 *
 */

					/* basic table entry */
struct hashlist {
	char	*name;
	char	*def;
	struct	hashlist *next;		/* next in chain     */
};

#define HASHMAX	100			/* size of hashtable */

					/* hash table itself */

struct hashlist *lookup(const char *s);
struct hashlist *install(const char *name, char *def);

#endif /* LOOKUP_H */
