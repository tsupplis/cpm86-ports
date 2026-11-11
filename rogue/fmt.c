/*
 * fmt.c - small integer-only sprintf/printf for CP/M-86.
 *
 * The c86.lib versions link in the whole float-capable formatter, which
 * does not fit in the 64K code segment together with the game.
 * Supports %d %ld %s %c %% with optional 0 flag, width and 'l'.
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static char *
put_num(p, n, width, zero)
char *p;
long n;
int width, zero;
{
	char tmp[12];
	int i = 0, neg = 0;
	unsigned long u;

	if (n < 0) {
		neg = 1;
		u = -n;
	} else
		u = n;
	do {
		tmp[i++] = '0' + (int) (u % 10);
		u /= 10;
	} while (u);
	if (neg)
		tmp[i++] = '-';
	while (width-- > i)
		*p++ = zero ? '0' : ' ';
	while (i)
		*p++ = tmp[--i];
	return p;
}

static void
vfmt(buf, fmt, ap)
char *buf;
char *fmt;
va_list ap;
{
	char *p = buf, *s;
	int width, zero, lng;

	while (*fmt) {
		if (*fmt != '%') {
			*p++ = *fmt++;
			continue;
		}
		fmt++;
		zero = (*fmt == '0');
		width = 0;
		while (*fmt >= '0' && *fmt <= '9')
			width = width * 10 + *fmt++ - '0';
		lng = (*fmt == 'l');
		if (lng)
			fmt++;
		switch (*fmt) {
		case 'd':
			if (lng)
				p = put_num(p, va_arg(ap, long), width, zero);
			else
				p = put_num(p, (long) va_arg(ap, int), width, zero);
			break;
		case 's':
			s = va_arg(ap, char *);
			while (*s)
				*p++ = *s++;
			break;
		case 'c':
			*p++ = (char) va_arg(ap, int);
			break;
		case '%':
			*p++ = '%';
			break;
		}
		if (*fmt)
			fmt++;
	}
	*p = '\0';
}

int sprintf(char *buf, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfmt(buf, (char *) fmt, ap);
	va_end(ap);
	return strlen(buf);
}

int printf(const char *fmt, ...)
{
	va_list ap;
	char buf[256];

	va_start(ap, fmt);
	vfmt(buf, (char *) fmt, ap);
	va_end(ap);
	fputs(buf, stdout);
	return strlen(buf);
}
