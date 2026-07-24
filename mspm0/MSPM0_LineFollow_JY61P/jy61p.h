#ifndef JY61P_H
#define JY61P_H

#include <stdbool.h>
#include <stdint.h>

/* 浣胯兘 SysConfig 涓?IMU_UART 鐨勬帴鏀朵腑鏂€?*/
void JY61P_Init(void);

/* 宸叉敹鍒板苟閫氳繃鏍￠獙鐨勮閫熷害鎴栬搴﹀抚鍚庤繑鍥?true銆?*/
bool JY61P_IsValid(void);

/*
 * 杩斿洖 Z 杞磋閫熷害锛坉eg/s锛夈€傚畨瑁呮柟鍚戠浉鍙嶆椂淇敼 jy61p.c 涓? * JY61P_GYRO_Z_SIGN锛屼娇灏忚溅鍚戝彸鏃嬭浆鏃惰鍊间负姝ｃ€? */
float JY61P_GetGyroZ(void);

/* 杩斿洖 JY61P 绉垎寰楀埌鐨?Z 杞磋搴︼紙deg锛夈€?*/
float JY61P_GetYaw(void);

/* 姣忔敹鍒颁竴涓湁鏁堢殑 0x52/0x53 甯ч€掑锛岀敤浜庢娴嬩覆鍙ｆ槸鍚﹀仠姝㈡洿鏂般€?*/
uint32_t JY61P_GetUpdateCounter(void);

#endif

