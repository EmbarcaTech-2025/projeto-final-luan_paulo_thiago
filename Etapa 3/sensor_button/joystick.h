#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdbool.h>

#define JOY_X 26
#define JOY_Y 27
#define JOY_SW 22

void joystick_init(void);
void joystick_update_limit(int *temp_limit, bool active);
bool is_setting_mode_active(void);

#endif