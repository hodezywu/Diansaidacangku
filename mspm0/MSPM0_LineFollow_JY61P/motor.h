#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdint.h>

/* 鍚姩鐢垫満 PWM銆?*/
void Motor_Init(void);

/* 鍋滄涓よ矾鐢垫満骞跺皢鏂瑰悜杈撳叆鎷変綆銆?*/
void Motor_Stop(void);

/* 宸﹀彸杞樊閫熸帶鍒讹紝鑼冨洿 -500锝?00銆?*/
void Motor_SetDifferential(int left_speed, int right_speed);

#endif /* __MOTOR_H */

