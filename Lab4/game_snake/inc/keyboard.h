#ifndef MINIOS_KEYBOARD_H
#define MINIOS_KEYBOARD_H

#include "type.h"
#define KEYBOARD_PORT 0X60
void	add_keyboard_buf(u8 ch);
u8 getch(void);

#endif /* MINIOS_KEYBOARD_H */