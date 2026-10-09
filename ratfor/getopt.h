#ifndef GETOPT_H
#define GETOPT_H

extern char	*optarg;	/* Global argument pointer. */
extern int	optind77;	/* Global argv index. */

int our_getopt(int argc, char **argv, const char *optstring);

#endif /* GETOPT_H */
