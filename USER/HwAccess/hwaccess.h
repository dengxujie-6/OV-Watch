#ifndef HWACCESS_H
#define HWACCESS_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_WIDTH  240U
#define LCD_HEIGHT 280U

typedef enum {
    HWACCESS_KEY_BACK = 0,
    HWACCESS_KEY_SCREEN,
} HwAccess_KeyId_t;

/**
 * @brief 更新 EM7028 原始 PPG 缓存。
 *
 * @param raw_ppg 当前原始 PPG。
 * @param raw_valid 1 表示本次原始值有效，0 表示无效。
 */
void HwAccess_Em7028_UpdateRawCache(uint16_t raw_ppg, uint8_t raw_valid);

/**
 * @brief 更新 EM7028 心率结果缓存。
 *
 * @param bpm 当前对外显示的 BPM。
 * @param hr_valid 1 表示 BPM 有效，0 表示当前仍处于测量或无效状态。
 */
void HwAccess_Em7028_UpdateHeartRateCache(uint8_t bpm, uint8_t hr_valid);

typedef void (*HwAccess_IsrHook_t)(void * context);

/**
 * @brief 涓夎酱鏈夌鍙锋暣鏁板悜閲忋€?
 */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} HwAccess_Vector3i16_t;

/**
 * @brief LCD 纭欢鎿嶄綔琛ㄣ€?
 *
 * LCD 妯″潡鍚戜换鍔″眰鏆撮湶鍒濆鍖栥€佹樉绀哄紑鍏冲拰鑳屽厜鎺у埗锛涘埛灞忕獥鍙ｅ拰鍍忕礌浼犺緭浠嶇敱 LVGL 绉绘灞備娇鐢ㄣ€?
 */
typedef struct Lcdstruct_typedef
{
    void (*init)(void);  /**< 鍒濆鍖?LCD 鍜岃Е鎽哥‖浠躲€?*/
    void (*deinit)(void);  /**< 鍙嶅垵濮嬪寲 LCD銆?*/
    void (*set_backlight)(uint8_t brightness);  /**< 璁剧疆 LCD 鑳屽厜浜害锛岃寖鍥?0~100銆?*/
    void (*display_on)(void);  /**< 鎵撳紑 LCD 鏄剧ず杈撳嚭銆?*/
    void (*display_off)(void);  /**< 鍏抽棴 LCD 鏄剧ず杈撳嚭銆?*/
} obj_Lcd;

/**
 * @brief 鎸夐敭纭欢鎿嶄綔琛ㄣ€?
 *
 * 浠诲姟灞傚彧鍏冲績鈥滃摢涓寜閿槸鍚︽寜涓嬧€濓紝涓嶇洿鎺ユ寔鏈?GPIO 绔彛銆佸紩鑴氬拰鏈夋晥鐢靛钩銆?
 */
typedef struct Keystruct_typedef
{
    void (*init)(void);  /**< 鍒濆鍖栨寜閿緭鍏ョ‖浠躲€?*/
    uint8_t (*is_pressed)(HwAccess_KeyId_t key);  /**< 璇诲彇鎸囧畾鎸夐敭褰撳墠鏄惁鎸変笅銆?*/
} obj_Key;

/**
 * @brief 鐢垫簮涓庡厖鐢电姸鎬佽闂帴鍙ｃ€?
 *
 * 浠诲姟灞傞€氳繃璇ユ帴鍙ｄ繚鎸?POWER_EN銆佽鍙?CHARG 鍜岀數姹犵數鍘嬶紝涓嶇洿鎺ヨ闂?GPIO/ADC銆?
 */
typedef struct Powerstruct_typedef
{
    void (*open)(void);
    void (*close)(void);  /**< 鎵撳紑骞朵繚鎸佺郴缁熺數婧愩€?*/
    uint8_t (*is_charging)(void);  /**< 璇诲彇鍏呯數妫€娴嬬姸鎬侊紝1 琛ㄧず楂樼數骞虫湁鏁堛€?*/
    void (*update_battery_cache)(void);  /**< 閲囨牱涓€娆＄數姹犵數鍘嬪苟鍒锋柊缂撳瓨锛岀敱浼犳劅鍣ㄤ换鍔″懆鏈熻皟鐢ㄣ€?*/
    uint16_t (*get_battery_voltage_mv)(void);  /**< 璇诲彇鐢垫睜鐢靛帇妫€娴嬪€硷紝鍗曚綅 mV銆?*/
    uint8_t (*get_battery_percent)(void);  /**< 浠庣紦瀛樿鍙栫數姹犵數閲忕櫨鍒嗘瘮锛岃寖鍥?0~100銆?*/
    uint8_t (*is_battery_valid)(void);  /**< 璇诲彇鐢垫睜缂撳瓨鏄惁宸茬粡鏈夎繃鏈夋晥閲囨牱銆?*/
} obj_Power;

/**
 * @brief 澶栭儴纭欢鐪嬮棬鐙楁搷浣滆〃銆?
 *
 * PB1 Dog_EN 浣庣數骞虫墦寮€銆侀珮鐢靛钩鍏抽棴锛汸B2 WDI 閫氳繃鍛ㄦ湡缈昏浆瀹屾垚鍠傜嫍銆?
 */
typedef struct Watchdogstruct_typedef
{
    void (*init)(void);  /**< 鍒濆鍖栧閮ㄧ湅闂ㄧ嫍 GPIO锛岄粯璁や繚鎸佸叧闂€?*/
    void (*enable)(void);  /**< 鎵撳紑澶栭儴纭欢鐪嬮棬鐙椼€?*/
    void (*disable)(void);  /**< 鍏抽棴澶栭儴纭欢鐪嬮棬鐙椼€?*/
    void (*feed)(void);  /**< 缈昏浆 WDI 瀹屾垚涓€娆″杺鐙椼€?*/
} obj_Watchdog;

/**
 * @brief 澶栭儴 PROM 璇诲啓鎿嶄綔琛ㄣ€?
 *
 * PROM 搴曞眰浣跨敤 PA11/PA12 杞欢 I2C 鍜?BL24C02F EEPROM 椹卞姩锛屼笂灞傚彧閫氳繃瀛楄妭鍦板潃璁块棶銆?
 */
typedef struct Promstruct_typedef
{
    void (*init)(void);  /**< 鍒濆鍖?PROM 涓撶敤 I2C 鎬荤嚎銆?*/
    int (*probe)(void);  /**< 鎺㈡祴 PROM 鏄惁鍝嶅簲 ACK銆?*/
    int (*read)(uint8_t addr, uint8_t * data, uint16_t len);  /**< 浠?PROM 杩炵画璇诲彇瀛楄妭銆?*/
    int (*write)(uint8_t addr, const uint8_t * data, uint16_t len);  /**< 鍚?PROM 杩炵画鍐欏叆瀛楄妭銆?*/
    int (*read_byte)(uint8_t addr, uint8_t * value);  /**< 璇诲彇 PROM 鍗曞瓧鑺傘€?*/
    int (*write_byte)(uint8_t addr, uint8_t value);  /**< 鍐欏叆 PROM 鍗曞瓧鑺傘€?*/
} obj_Prom;

/**
 * @brief 钃濈墮妯″潡鎿嶄綔琛ㄣ€?
 *
 * 涓婂眰鍙〃杈锯€滄墦寮€妯″潡鈥濆拰鈥滈€氳繃钃濈墮涓插彛鏀跺彂鏁版嵁鈥濈殑闇€姹傦紝涓嶆毚闇?USART1銆?
 * PA8/PA9/PA10 鎴?HAL UART 鍙ユ焺绛夊簳灞傜粏鑺傘€?
 */
typedef struct Bluetoothstruct_typedef
{
    void (*init)(void);  /**< 鍒濆鍖栬摑鐗?EN 寮曡剼鍜?USART1銆?*/
    void (*enable)(void);  /**< 鎷夐珮 BlueTooth_EN锛屾墦寮€钃濈墮妯″潡銆?*/
    void (*disable)(void);  /**< 鎷変綆 BlueTooth_EN锛屽叧闂摑鐗欐ā鍧椼€?*/
    uint8_t (*is_enabled)(void);  /**< 璇诲彇 BlueTooth_EN 褰撳墠杈撳嚭鐘舵€併€?*/
    int (*send)(const uint8_t * data, uint16_t len, uint32_t timeout_ms);  /**< 闃诲鍙戦€佹暟鎹€?*/
    int (*send_dma)(const uint8_t * data, uint16_t len);  /**< 闈為樆濉?DMA 鍙戦€侊紝瀹屾垚鐢?DMA 涓柇鏍囪銆?*/
    uint8_t (*is_tx_busy)(void);  /**< 鏌ヨ DMA 鍙戦€佹槸鍚︿粛鍦ㄨ繘琛屻€?*/
    uint8_t (*take_tx_done)(void);  /**< 璇诲彇骞舵竻闄?DMA 鍙戦€佸畬鎴愭爣蹇椼€?*/
    void (*register_tx_complete_hook)(HwAccess_IsrHook_t hook, void * context);  /**< DMA TX 鐎瑰本鍨氶崶鐐剁殶濞夈劌鍞介妴?*/
    void (*register_error_hook)(HwAccess_IsrHook_t hook, void * context);  /**< UART 闁挎瑨顕ら崶鐐剁殶濞夈劌鍞介妴?*/
    int (*receive)(uint8_t * data, uint16_t len, uint32_t timeout_ms);  /**< 闃诲鎺ユ敹鏁版嵁銆?*/
} obj_BlueTooth;

/**
 * @brief AHT21 娓╂箍搴︿紶鎰熷櫒缂撳瓨璁块棶鎺ュ彛銆?
 *
 * Sensor_Task 璐熻矗鍛ㄦ湡瑙﹀彂 update_cache()锛孶I 灞傚彧璇诲彇缂撳瓨鍊硷紝涓嶇洿鎺ヨ闂?I2C 鎬荤嚎銆?
 */
typedef struct Aht21struct_typedef
{
    int (*init)(void);  /**< 鍒濆鍖?AHT21 鍙婂叾杞欢 I2C 鎬荤嚎銆?*/
    int (*probe)(void);  /**< 鎺㈡祴 AHT21 鏄惁鍝嶅簲榛樿 I2C 鍦板潃銆?*/
    int (*update_cache)(void);  /**< 閲囨牱涓€娆℃俯婀垮害骞跺埛鏂?HwAccess 缂撳瓨銆?*/
    int16_t (*get_temperature_x10_c)(void);  /**< 璇诲彇缂撳瓨娓╁害锛屽崟浣?0.1 鎽勬皬搴︺€?*/
    uint16_t (*get_humidity_x10_percent)(void);  /**< 璇诲彇缂撳瓨婀垮害锛屽崟浣?0.1%RH銆?*/
    uint8_t (*is_valid)(void);  /**< 缂撳瓨鏄惁宸叉湁涓€娆℃湁鏁?AHT21 閲囨牱銆?*/
} obj_Aht21;

/**
 * @brief LSM303DLHC 鍔犻€熷害璁″拰纾佸姏璁＄紦瀛樿闂帴鍙ｃ€?
 *
 * Sensor_Task 鍛ㄦ湡鍒锋柊缂撳瓨锛沀I 鎴栦笟鍔″眰鍙鍙栧凡缁忔崲绠楀ソ鐨勪笁杞存暟鎹€?
 */
typedef struct Lsm303dlhcstruct_typedef
{
    int (*init)(void);  /**< 鍒濆鍖?LSM303DLHC 鍔犻€熷害璁″拰纾佸姏璁°€?*/
    int (*probe)(void);  /**< 鎺㈡祴 LSM303DLHC 鏄惁鍝嶅簲銆?*/
    int (*update_cache)(void);  /**< 閲囨牱涓€娆″姞閫熷害鍜岀鍔涜骞跺埛鏂扮紦瀛樸€?*/
    int (*get_accel_mg)(HwAccess_Vector3i16_t * value);  /**< 璇诲彇缂撳瓨鍔犻€熷害锛屽崟浣?mg銆?*/
    int (*get_mag_mgauss)(HwAccess_Vector3i16_t * value);  /**< 璇诲彇缂撳瓨纾佸満锛屽崟浣嶆楂樻柉銆?*/
    uint8_t (*is_valid)(void);  /**< 缂撳瓨鏄惁宸叉湁涓€娆℃湁鏁?LSM303DLHC 閲囨牱銆?*/
} obj_Lsm303dlhc;

/**
 * @brief MPU6050 鍔犻€熷害璁°€侀檧铻轰华鍜屾俯搴︾紦瀛樿闂帴鍙ｃ€?
 *
 * Sensor_Task 鍛ㄦ湡鍒锋柊缂撳瓨锛沀I 鎴栦笟鍔″眰鍙鍙栧凡缁忔崲绠楀ソ鐨勪笁杞存暟鎹紝
 * 涓嶇洿鎺ヨ闂?I2C 鎬荤嚎锛屼篃涓嶇洿鎺ュ寘鍚?BSP 澶存枃浠躲€?
 */
typedef struct Mpu6050struct_typedef
{
    int (*init)(void);  /**< 鍒濆鍖?MPU6050 骞堕厤缃粯璁ら噺绋嬨€?*/
    int (*probe)(void);  /**< 鎺㈡祴 MPU6050 鏄惁鍝嶅簲銆?*/
    int (*update_cache)(void);  /**< 閲囨牱涓€娆″叚杞村拰娓╁害鏁版嵁骞跺埛鏂扮紦瀛樸€?*/
    int (*get_accel_mg)(HwAccess_Vector3i16_t * value);  /**< 璇诲彇缂撳瓨鍔犻€熷害锛屽崟浣?mg銆?*/
    int (*get_gyro_x10_dps)(HwAccess_Vector3i16_t * value);  /**< 璇诲彇缂撳瓨瑙掗€熷害锛屽崟浣?0.1dps銆?*/
    int16_t (*get_temperature_x10_c)(void);  /**< 璇诲彇缂撳瓨娓╁害锛屽崟浣?0.1 鎽勬皬搴︺€?*/
    uint8_t (*is_valid)(void);  /**< 缂撳瓨鏄惁宸叉湁涓€娆℃湁鏁?MPU6050 閲囨牱銆?*/
    uint32_t (*get_step_count)(void);  /**< 璇诲彇鍩轰簬 MPU6050 鍔犻€熷害缂撳瓨浼扮畻鐨勬鏁般€?*/
    void (*reset_step_count)(void);  /**< 娓呴浂 MPU6050 璁℃鐘舵€佸拰姝ユ暟銆?*/
} obj_Mpu6050;
/**
 * @brief EM7028 蹇冪巼缂撳瓨璁块棶鎺ュ彛銆?
 *
 * 蹇冪巼浠诲姟璐熻矗鍚姩/鍋滄閲囨牱骞跺懆鏈熻皟鐢?update_cache()锛?
 * UI 椤甸潰鍙鍙栫紦瀛樺悗鐨勫師濮嬪€煎拰浼扮畻 bpm銆?
 */
typedef struct Em7028struct_typedef
{
    int (*init)(void);
    int (*probe)(void);
    int (*start)(void);
    int (*stop)(void);
    int (*read_raw)(uint16_t * value);
    int (*update_cache)(void);
    int (*get_probe_status)(void);
    int (*read_reg)(uint8_t reg, uint8_t * value);
    uint8_t (*get_pid)(void);
    int (*get_last_i2c_status)(void);
    uint32_t (*get_last_i2c_error)(void);
    uint16_t (*get_raw)(void);
    uint8_t (*get_bpm)(void);
    uint8_t (*is_valid)(void);
    uint8_t (*is_running)(void);
} obj_Em7028;

/**
 * @brief 椤跺眰纭欢璁块棶瀵硅薄銆?
 *
 * 鍚庣画鏂板纭欢妯″潡鏃讹紝搴斾互妯″潡鎿嶄綔琛ㄧ殑褰㈠紡鍔犲叆杩欓噷锛屼换鍔″眰鍜?UI 灞傜粺涓€閫氳繃璇ュ璞¤闂‖浠躲€?
 */
typedef struct hwaccess_typedef
{
    obj_Lcd lcd;  /**< LCD 鎿嶄綔琛ㄣ€?*/
    obj_Key key;  /**< 鎸夐敭鎿嶄綔琛ㄣ€?*/
    obj_Power power;  /**< 鐢垫簮涓庡厖鐢电姸鎬佽闂帴鍙ｃ€?*/
    obj_Watchdog watchdog;  /**< 澶栭儴纭欢鐪嬮棬鐙楁搷浣滆〃銆?*/
    obj_Prom prom;  /**< 澶栭儴 PROM 璇诲啓鎿嶄綔琛ㄣ€?*/
    obj_BlueTooth bluetooth;  /**< 钃濈墮妯″潡鎿嶄綔琛ㄣ€?*/
    obj_Aht21 aht21;  /**< AHT21 娓╂箍搴︿紶鎰熷櫒缂撳瓨璁块棶鎺ュ彛銆?*/
    obj_Lsm303dlhc lsm303dlhc;  /**< LSM303DLHC 鍔犻€熷害璁″拰纾佸姏璁＄紦瀛樿闂帴鍙ｃ€?*/
    obj_Mpu6050 mpu6050;  /**< MPU6050 鍏酱 IMU 缂撳瓨璁块棶鎺ュ彛銆?*/
    obj_Em7028 em7028;  /**< EM7028 蹇冪巼缂撳瓨璁块棶鎺ュ彛銆?*/
} obj_HwAccess;

/**
 * @brief 鍏ㄥ眬纭欢璁块棶鍏ュ彛銆?
 */
extern obj_HwAccess HwAccess;

#ifdef __cplusplus
}
#endif

#endif
