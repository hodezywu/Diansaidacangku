#include "sensor.h"
#include "ti_msp_dl_config.h" /* TI SysConfig 鐢熸垚鐨勫簳灞傛牳蹇冨ご鏂囦欢 */

/**
  * @brief  璇诲彇 CD4051 澶氳矾澶嶇敤鍣?8 璺紶鎰熷櫒涓殑鏌愪竴璺?
  * @param  channel: 0 ~ 7 (浠ｈ〃鎺㈠ご閫氶亾)
  * @retval 1 (妫€娴嬪埌榛戠嚎/鐧界嚎瑙﹀彂鐘舵€? 鎴?0
  */
uint8_t Sensor_Read_Single(uint8_t channel) 
{
    /* 1. 璁剧疆 A0 鎺у埗绾?*/
    if (channel & 0x01) {
        DL_GPIO_setPins(AD0_PORT, AD0_A0_PIN);
    } else {
        DL_GPIO_clearPins(AD0_PORT, AD0_A0_PIN);
    }

    /* 2. 璁剧疆 A1 鎺у埗绾?*/
    if (channel & 0x02) {
        DL_GPIO_setPins(AD0_PORT, AD0_A1_PIN);
    } else {
        DL_GPIO_clearPins(AD0_PORT, AD0_A1_PIN);
    }

    /* 3. 璁剧疆 A2 鎺у埗绾?*/
    if (channel & 0x04) {
        DL_GPIO_setPins(AD0_PORT, AD0_A2_PIN);
    } else {
        DL_GPIO_clearPins(AD0_PORT, AD0_A2_PIN);
    }

    /* 绛夊緟妯℃嫙寮€鍏冲拰姣旇緝杈撳嚭绋冲畾锛屽欢鏃朵笉鍐嶄緷璧栧浐瀹?2MHz涓婚銆?*/
    delay_cycles((CPUCLK_FREQ / 1000000U) * 50U);

    /* 5. 璇诲彇 PA7 (OUT寮曡剼) 鐨勭數骞?*/
    if (DL_GPIO_readPins(out_PORT, out_outpin_PIN) & out_outpin_PIN) {
        return 1;
    }
    return 0;
}

void Sensor_Read_All(uint8_t result[8])
{
    uint8_t channel;

    for (channel = 0U; channel < 8U; channel++) {
        result[channel] = Sensor_Read_Single(channel);
    }
}

