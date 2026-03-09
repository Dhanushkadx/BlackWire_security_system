#pragma once

// Refactored ALARM class
// ----------------------
// This version keeps the original alarm decision logic as close as possible,
// but moves zone configuration / name storage responsibility to ZoneManager.
//
// Responsibilities now:
// - ALARM       : runtime alarm policy and state machine
// - ZoneManager : zone configuration, names, SPIFFS-backed persistence
// - ZoneEngine  : live zone runtime processing / debounced state source

#include "Arduino.h"
#include "TimerSW.h"
#include "logger.h"
#include "typex.h"
#include "zone_engine.h"

extern ZoneEngine zoneEngine;

class ZoneManager;
class ZoneEngine;

extern "C" {
    typedef void (*_callbackFunction)(uint8_t, const char*, eInvoking_source);
    typedef void (*_callbackFunctionType2)(uint8_t);
    typedef bool (*_callbackFunctionType3)(const char*, int);
    typedef uint8_t (*_callbackFunctionType4)(void);
    typedef char* (*_callbackFunctionType5)(uint8_t);
    typedef void (*_callbackFunctionType6)(uint8_t, const char*);
    typedef void (*_callbackFunctionType7)(void);
    typedef void (*_callbackFunctionType8)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
    typedef void (*_callbackFunctionType9)(uint8_t, uint8_t, char*, char*);
    typedef int8_t (*_callbackFunctionType10)(const char*);
}

typedef enum arm_mode {
    USER_SELECT = 0,
    AS_ITIS_NO_BYPASS,
    AS_ITIS_BYPASS,
} eARM_Mode;

#define card_A_zone__starting_number 8
#define card_B_zone__starting_number 16

class ALARM {
public:
    ALARM();

    // Dependency attachment
    void attachZoneManager(ZoneManager* zm);

    // Main runtime loop
    void watcher();

    // System state
    eMain_state get_system_state();
    void set_system_state(eMain_state state, eInvoking_source invoker, uint8_t user_id);
    void set_arm_mode(eARM_Mode mode);
    eARM_Mode get_arm_mode();

    // Zone state notification entry point (called by external layers)
    void Universal_zone_state_update(uint8_t last_update_sensor_index);

    // Alarm processing blocks
    void alarm_process_wired(uint8_t zone);
    void alarm_process_wired_24H(uint8_t zone);
    void chime_sound();

    // Zone index mapping helper
    int8_t get_absolute_zone_number(eInvoking_source card_id, uint8_t relative_zone_number);

    // Utility for broadcasting one bit field across 48 zones
    void any_zone_bitmask_parameter_to_bytes(uint8_t bit_mask_of_para,
                                             uint8_t& zone0_7,
                                             uint8_t& zone8_15,
                                             uint8_t& zone16_23,
                                             uint8_t& zone24_31,
                                             uint8_t& zone32_39,
                                             uint8_t& zone40_47);

    // RF / remote helper functions (legacy hooks kept because other code may still use them)
    void update_rx_rf_id_for_all(const char* rf_id_rx);


    // ---- Alarm related callbacks ----
    void set_fn_disarm(_callbackFunction pFn);
    void set_fn_exit_delay_timer_start(_callbackFunctionType7 pFn);
    void set_fn_intializ_sensors(_callbackFunctionType4 pFn);
    void set_fn_entry_delay_timer_start(_callbackFunctionType7 pFn);
    void set_fn_chime_sound(_callbackFunctionType7 pFn);
    void set_fn_arm_faild(_callbackFunctionType3 pFn);
    void set_fn_exit_delay_time_out(_callbackFunctionType3 pFn);
    void set_fn_arm_can_not_be_done(_callbackFunctionType7 pFn);
    void set_fn_arm(_callbackFunction pFn);
    void set_fn_zone_status_update(_callbackFunctionType3 pFn);
    void set_fn_zone_state_update_notify(_callbackFunctionType8 pFn);
    void set_fn_system_is_not_ready(_callbackFunctionType7 pFn);
    void set_fn_system_is_ready(_callbackFunctionType7 pFn);
    void set_fn_entry_delay_time_out(_callbackFunctionType3 pFn);
    void set_fn_rf_zone_re_enable(_callbackFunctionType2 pFn);
    void set_fn_alarm_calling(_callbackFunctionType4 pFn);
    void set_fn_creat_alarm_msg(_callbackFunctionType3 pFn);
    void set_fn_alarm_notify(_callbackFunctionType2 pFn);
    void set_fn_alarm_snooze(_callbackFunctionType7 pFn);
    uint8_t set_call_back_sms_loop_status(_callbackFunctionType4 pFn);
    void set_fn_alarm_bell_time_out(_callbackFunctionType4 pFn);
    void set_fn_save_event_info(_callbackFunctionType9 pFn);


    // // ---- Zone configuration / metadata API ----
    // // These now talk to ZoneManager instead of directly owning persistent storage.
    // void set_sensor_name(uint8_t sensor_index, char* sensor_name);
    // void set_sensor_bypassed(uint8_t sensor_index, bool state);
    // void set_sensor_24H(uint8_t sensor_index, bool state);
    // void set_sensor_en_de(uint8_t sensor_index, bool state);
    // void set_sensor_entry(uint8_t sensor_index, bool state);
    // void set_sensor_exit(uint8_t sensor_index, bool state);
     void clear_entry_zone();
     void clear_exit_zone();

    // bool is_sensor_enable(uint8_t index);
    // bool is_sensor_bypass(uint8_t index);
    // bool is_sensor_available(uint8_t index);
    // bool is_sensor_ready(uint8_t zone);
    // bool is_sensor_24h(uint8_t zone);
    // bool is_sensor_RF(uint8_t zone);
    // bool is_sensor_exit_zone(uint8_t zone);
    // bool is_sensor_entry_zone(uint8_t zone);
    // char* get_sensor_name(uint8_t index);

    int8_t get_exit_zone_availablity();
    int8_t get_entry_zone_availablity();
    int8_t is_system_ready_to_arm();

    uint8_t get_entry_delay_time();
    uint8_t get_exit_delay_time();
    uint8_t get_bell_time_timer_interval();

    void set_entry_delay_timer_interval(uint8_t interval);
    void set_exit_delay_timer_interval(uint8_t interval);
    void set_bell_time_timer_interval(uint8_t interval);

    // Public helpers used by alarm_process_wired()
    bool is_sensor_skipped(uint8_t index);
    void process_open_sensor(uint8_t index, bool& alarm_enable);
    void process_closed_sensor(uint8_t index);
    void handle_alarm_trigger(uint8_t index);

    // Debug helper
    void get_data();

private:
    // Convenience access to the borrowed zone array
    inline bool zoneIndexValid(uint8_t index) const { return (pAny_sensor_array != nullptr) && (index < TOTAL_DEVICES); }

    // Internal runtime helpers
    void enable_only_closed_sensors_as_it_is();
    void clear_all_sensors_alarm_state();

private:
    // Dependencies
    ZoneManager* pZoneManager;
    ZoneEngine*  pZoneEngine;

    // Borrowed pointer to ZoneManager-owned array.
    // Kept so original alarm logic can stay mostly unchanged.
    MY_SENS* pAny_sensor_array;

    // Arm mode / state machine
    eARM_Mode  eArm_mode;
    bool       sensor_state_updated_to_be_processd;
    eMain_state _eCurrunt_state;
    eMain_state _ePrev_state;

    // Main callbacks
    _callbackFunction fn_arm;
    _callbackFunction fn_disarm;

    _callbackFunctionType2  fn_zone_time_out;
    _callbackFunctionType2  fn_alarm_notify;
    _callbackFunctionType2  fn_set_rf_zone_re_enable;

    _callbackFunctionType3  fn_arm_faild;
    _callbackFunctionType3  fn_chime_zone_notify;
    _callbackFunctionType3  fn_creat_msg;
    _callbackFunctionType3  fn_entry_delay_time_out;
    _callbackFunctionType3  fn_exit_delay_time_out;

    _callbackFunctionType4  fn_intializ_sensors;
    _callbackFunctionType4  fn_alarm_calling;
    _callbackFunctionType4  fn_sms_loop_status;
    _callbackFunctionType4  fn_alarm_bell_time_out;

    _callbackFunctionType5  fn_get_remmote_rfid;
    _callbackFunctionType6  fn_set_sens_rfid;
    _callbackFunctionType6  fn_set_remote_id;
    _callbackFunctionType10 fn_get_sens_rfid;

    _callbackFunctionType7  fn_system_is_not_ready;
    _callbackFunctionType7  fn_system_is_ready;
    _callbackFunctionType7  fn_arm_can_not_be_done;
    _callbackFunctionType7  fn_alarm_snooze;
    _callbackFunctionType7  fn_exit_delay_timer_start;
    _callbackFunctionType7  fn_entry_delay_timer_start;
    _callbackFunctionType7  fn_chime_sound;

    _callbackFunctionType8  fn_zone_state_update_notify;
    _callbackFunctionType9  fn_save_event_infor;

    // Runtime scratch state
    bool     rf_id_rx_updated;
    char     recived_rf_id[10];
    bool     last_update_sensor_state;
    int8_t   last_update_sensor_index;
    eInvoking_source _invorking_device;
    uint8_t  user_id;

    // Timers / state flags
    bool _exit_delay_timer_en;
    bool _exit_delay_time_out;
    bool _entry_delay_time_out;
    bool _entry_delay_timer_en;
    bool _Timer_alarm_relay_time_out;
    bool _Arm_faild_detected;
    bool perimeter_only;
    bool Timer_RF_zone_reactive_en;
    bool Timer_alarm_clear_delay_en;

    TimerSW Timer_exit_delay;
    TimerSW Timer_RF_zone_reactive_delay;
    TimerSW Timer_alarm_clear_delay;
    TimerSW Timer_arm_mode_waiting_time_out;
    TimerSW Timer_entry_delay;
    TimerSW Timer_alarm_relay;

    uint8_t Timer_exit_delay_interval;
    uint8_t Timer_entry_delay_interval;
    uint8_t Timer_alarm_relay_time_out;
};

