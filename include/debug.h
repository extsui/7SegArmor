#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

void Debug_init(void);
void dprint(const char *fmt, ...);
void Debug_charSendendHandler(void);

#define DPRINTF(...)		dprint(__VA_ARGS__)

#endif /* DEBUG_H_INCLUDED */
