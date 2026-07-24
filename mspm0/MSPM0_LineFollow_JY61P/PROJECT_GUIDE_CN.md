# MSPM0G3507 鐏板害寰抗 + JY61P 宸ョ▼璇存槑

## 1. 宸查厤缃紩鑴?
| 鍔熻兘 | MSPM0G3507 |
|---|---|
| 鐏板害 AD0/AD1/AD2/OUT | PA0 / PA1 / PA2 / PA7 |
| 宸﹁疆 Motor C锛歅WMC/CIN1/CIN2 | PA21 / PB6 / PB7 |
| 鍙宠疆 Motor B锛歅WMB/BIN1/BIN2 | PA22 / PB8 / PB9 |
| JY61P UART1 TX/RX | PA17 / PA18 |
| SWDIO/SWCLK | PA19 / PA20 |

JY61P 鎺ョ嚎锛?
- JY61P TX -> PA18锛圲ART1_RX锛?- JY61P RX -> PA17锛圲ART1_TX锛?- VCC -> 5V
- GND -> GND

## 2. CCS 瀵煎叆涓庣儳褰?
1. 瑙ｅ帇宸ョ▼銆?2. CCS锛歚Project -> Import CCS Projects`锛岄€夋嫨瑙ｅ帇鍚庣殑鐩綍銆?3. 纭宸插畨瑁?MSPM0 SDK 2.11.00.07 鍜?TI Arm Clang 5.1.1 LTS銆?4. 宸ョ▼鐩爣閰嶇疆宸茬粡閫夋嫨 `SEGGER J-Link Emulator` 鍜?MSPM0G3507銆?5. 鍏堢粰涓绘帶鏉挎甯镐緵鐢碉紝鍐嶆寜涓嬭〃鎺?J-Link 鐨?SWD 绾裤€?6. 鍏堟柇寮€鐢垫満涓荤數婧愭垨鏋剁┖杞﹁疆锛岀偣鍑婚敜瀛愬浘鏍?Build銆?7. 鐐瑰嚮 Debug Project锛涚▼搴忓仠鍦?`main` 鍚庣偣鍑?Continue銆?
COM8 鏄?J-Link CDC 铏氭嫙涓插彛锛屼笉鏄?CCS 鐑у綍绔彛銆?
J-Link 20 閽堝彛鍒颁富鎺ч《閮ㄨ皟璇曟帓閽堬細

| J-Link 20閽?| 涓绘帶鏉?|
|---|---|
| 1 VTref | 3V3 |
| 7 SWDIO | DIO锛圥A19锛?|
| 9 SWCLK | CLK锛圥A20锛?|
| 4/6/8/10/12 浠讳竴 GND | GND |
| 15 nRESET锛堝彲閫夛級 | RST |

`VTref` 鐢ㄤ簬璁?J-Link 璇嗗埆鐩爣鏉块€昏緫鐢靛帇锛涗富鎺у簲鐢辫嚜宸辩殑 USB-C 鎴栬溅涓?绋冲帇鐢垫簮渚涚數銆備笉瑕佹妸 J-Link 20 閽堝彛鐨?19 鑴?5V 鍚屾椂鎺ュ埌宸茬粡渚涚數鐨勪富鎺с€?
## 3. 棣栨娴嬭瘯

### JY61P

SysConfig 褰撳墠璁剧疆涓?9600銆?-N-1銆傝繍琛屽悗鍦?Expressions 涓瀵燂細

- `g_gyro_z_dps`
- `g_yaw_deg`
- `g_update_counter`

杩欎簺鍙橀噺鍦?`jy61p.c` 涓€傝嫢璁℃暟鍣ㄥ缁堜负0锛?
1. 妫€鏌?TX/RX 鏄惁浜ゅ弶锛?2. 妫€鏌ュ叡鍦帮紱
3. 纭 JY61P 褰撳墠娉㈢壒鐜囨槸鍚︿负115200锛?4. 濡傛灉鏄?15200锛屽湪 SysConfig 鐨?IMU_UART 涓敼涓?15200銆?
璁╁皬杞﹁溅澶村悜鍙虫棆杞椂锛宍JY61P_GetGyroZ()` 搴斾负姝ｃ€傚鏋滀负璐燂紝鎶?`jy61p.c` 鐨?`JY61P_GYRO_Z_SIGN` 鏀规垚 `-1.0f`銆?
### 鐢垫満

鏋剁┖杞﹁疆鍚庣粰 `Motor_SetDifferential(150, 150)`锛屼袱杞兘搴斾娇杞﹀悜鍓嶃€?鏌愪竴杞弽杞椂锛屽彧淇敼 `motor.c` 椤堕儴瀵瑰簲鏂瑰悜瀹忥細

- `LEFT_FORWARD_CIN1_HIGH`
- `RIGHT_FORWARD_BIN1_HIGH`

### 鐏板害

鍦?Expressions 涓瀵?main 鐨?`sensor[0]`锝瀈sensor[7]`銆?
- 鑻ラ粦绾夸负0锛屼慨鏀?`LINE_ACTIVE_LEVEL` 涓?銆?- `sensor[0]` 蹇呴』鏄溅浣撳乏渚э紱鑻ュ乏鍙崇浉鍙嶏紝灏?main.c 鐨勬潈閲嶆暟缁勫弽杞€?- 瀹樻柟妯″潡鎺ョ嚎浣跨敤5V渚涚數锛涢娆′笂鐢典粛寤鸿鐢ㄤ竾鐢ㄨ〃娴嬮噺 OUT 楂樼數骞炽€侽UT
  鐩存帴杩涘叆 MSPM0 鏃朵笉寰楄秴杩囧櫒浠惰緭鍏ュ厑璁歌寖鍥达紝寮傚父鏃跺厛鏂數骞跺鍔犵數骞宠浆鎹€?
## 4. 璋冨弬椤哄簭

1. 璋?`BASE_SPEED`锛屼繚璇佽兘绋冲畾璧锋銆?2. 璋?`LINE_KP`锛屼繚璇佸亸绾垮悗鑳藉洖姝ｃ€?3. 璋?`RATE_KP`锛屽鍔犻檧铻鸿閫熷害闃诲凹銆?4. 璋?`LINE_KD`锛屽噺灏戝乏鍙虫憜鍔ㄣ€?5. 鏈€鍚庡皬骞呭鍔?`YAW_KP`锛屾敼鍠勭洿绾胯埅鍚戙€?
濡傛灉瓒婁慨瓒婂亸锛屽厛妫€鏌ユ帰澶村乏鍙抽『搴忓拰鐢垫満鏂瑰悜锛屽啀妫€鏌ラ檧铻烘璐熷彿锛屼笉瑕佸厛
鐩茬洰澧炲ぇ PID銆?
## 5. 缂栫爜鍣?
褰撳墠宸ョ▼灏氭湭鍚敤缂栫爜鍣ㄣ€侲2A/E2B/E3A/E3B 鏄┍鍔ㄦ澘淇″彿鍚嶏紝涓嶆槸涓绘帶
寮曡剼鍙枫€傚缓璁悗缁疄闄呮帴绾匡細

| 缂栫爜鍣?| 寤鸿涓绘帶寮曡剼 |
|---|---|
| 宸?Motor C E3A/E3B | PA24 / PA25 |
| 鍙?Motor B E2A/E2B | PA26 / PA27 |

鎺ョ嚎纭鍚庡啀澧炲姞GPIO杈规部涓柇鍜屽乏鍙宠疆PI閫熷害鐜€?
