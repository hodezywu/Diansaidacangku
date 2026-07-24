#ifndef __SENSOR_H
#define __SENSOR_H

#include <stdint.h>

/* 璇诲彇鍗曡矾浼犳劅鍣ㄦ暟鎹嚱鏁板０鏄?*/
uint8_t Sensor_Read_Single(uint8_t channel);

/* 涓€娆¤鍙栧叓璺紶鎰熷櫒锛宺esult[0]锝瀝esult[7] 瀵瑰簲閫氶亾0锝?銆?*/
void Sensor_Read_All(uint8_t result[8]);

#endif /* __SENSOR_H */

