#include "alarm.h"

#include "ZoneManager.h"



// -----------------------------------------------------------------------------
// Constructor / dependency attachment
// -----------------------------------------------------------------------------

ALARM::ALARM()
    : pZoneManager(nullptr),
      pZoneEngine(nullptr),
      pAny_sensor_array(nullptr),
      eArm_mode(USER_SELECT),
      sensor_state_updated_to_be_processd(false),
      _eCurrunt_state(BEGING),
      _ePrev_state(BEGING),
      fn_arm(nullptr),
      fn_disarm(nullptr),
      fn_zone_time_out(nullptr),
      fn_alarm_notify(nullptr),
      fn_set_rf_zone_re_enable(nullptr),
      fn_arm_faild(nullptr),
      fn_chime_zone_notify(nullptr),
      fn_creat_msg(nullptr),
      fn_entry_delay_time_out(nullptr),
      fn_exit_delay_time_out(nullptr),
      fn_intializ_sensors(nullptr),
      fn_alarm_calling(nullptr),
      fn_sms_loop_status(nullptr),
      fn_alarm_bell_time_out(nullptr),
      fn_get_remmote_rfid(nullptr),
      fn_set_sens_rfid(nullptr),
      fn_set_remote_id(nullptr),
      fn_get_sens_rfid(nullptr),
      fn_system_is_not_ready(nullptr),
      fn_system_is_ready(nullptr),
      fn_arm_can_not_be_done(nullptr),
      fn_alarm_snooze(nullptr),
      fn_exit_delay_timer_start(nullptr),
      fn_entry_delay_timer_start(nullptr),
      fn_chime_sound(nullptr),
      fn_zone_state_update_notify(nullptr),
      fn_save_event_infor(nullptr),
      rf_id_rx_updated(false),
      recived_rf_id{0},
      last_update_sensor_state(false),
      last_update_sensor_index(-1),
      _invorking_device(),
      user_id(0),
      _exit_delay_timer_en(false),
      _exit_delay_time_out(false),
      _entry_delay_time_out(false),
      _entry_delay_timer_en(false),
      _Timer_alarm_relay_time_out(true),
      _Arm_faild_detected(false),
      perimeter_only(false),
      Timer_RF_zone_reactive_en(false),
      Timer_alarm_clear_delay_en(false),
      Timer_exit_delay_interval(0),
      Timer_entry_delay_interval(0),
      Timer_alarm_relay_time_out(0) {}
      
void ALARM::attachZoneManager(ZoneManager* zm) {
    pZoneManager = zm;
    pAny_sensor_array = (pZoneManager != nullptr) ? pZoneManager->zones() : nullptr;
}


// -----------------------------------------------------------------------------
// Callback setters
// -----------------------------------------------------------------------------

void ALARM::set_fn_arm(_callbackFunction pFn) { fn_arm = pFn; }
void ALARM::set_fn_disarm(_callbackFunction pFn) { fn_disarm = pFn; }
void ALARM::set_fn_exit_delay_timer_start(_callbackFunctionType7 pFn) { fn_exit_delay_timer_start = pFn; }
void ALARM::set_fn_intializ_sensors(_callbackFunctionType4 pFn) { fn_intializ_sensors = pFn; }
void ALARM::set_fn_entry_delay_timer_start(_callbackFunctionType7 pFn) { fn_entry_delay_timer_start = pFn; }
void ALARM::set_fn_chime_sound(_callbackFunctionType7 pFn) { fn_chime_sound = pFn; }
void ALARM::set_fn_exit_delay_time_out(_callbackFunctionType3 pFn) { fn_exit_delay_time_out = pFn; }
void ALARM::set_fn_entry_delay_time_out(_callbackFunctionType3 pFn) { fn_entry_delay_time_out = pFn; }
void ALARM::set_fn_creat_alarm_msg(_callbackFunctionType3 pFn) { fn_creat_msg = pFn; }
void ALARM::set_fn_zone_state_update_notify(_callbackFunctionType8 pFn) { fn_zone_state_update_notify = pFn; }
void ALARM::set_fn_alarm_calling(_callbackFunctionType4 pFn) { fn_alarm_calling = pFn; }
void ALARM::set_fn_alarm_snooze(_callbackFunctionType7 pFn) { fn_alarm_snooze = pFn; }
void ALARM::set_fn_alarm_notify(_callbackFunctionType2 pFn) { fn_alarm_notify = pFn; }
void ALARM::set_fn_arm_faild(_callbackFunctionType3 pFn) { fn_arm_faild = pFn; }
void ALARM::set_fn_zone_status_update(_callbackFunctionType3 pFn) { fn_chime_zone_notify = pFn; }
void ALARM::set_fn_system_is_not_ready(_callbackFunctionType7 pFn) { fn_system_is_not_ready = pFn; }
void ALARM::set_fn_system_is_ready(_callbackFunctionType7 pFn) { fn_system_is_ready = pFn; }
void ALARM::set_fn_arm_can_not_be_done(_callbackFunctionType7 pFn) { fn_arm_can_not_be_done = pFn; }
void ALARM::set_fn_rf_zone_re_enable(_callbackFunctionType2 pFn) { fn_set_rf_zone_re_enable = pFn; }
void ALARM::set_fn_alarm_bell_time_out(_callbackFunctionType4 pFn) { fn_alarm_bell_time_out = pFn; }
void ALARM::set_fn_save_event_info(_callbackFunctionType9 pFn) { fn_save_event_infor = pFn; }
uint8_t ALARM::set_call_back_sms_loop_status(_callbackFunctionType4 pFn) { fn_sms_loop_status = pFn; return 0; }

// Legacy RF / remote callback setters
void ALARM::set_fn_get_rfid(_callbackFunctionType10 pFn) { fn_get_sens_rfid = pFn; }
void ALARM::set_fn_get_remote_id(_callbackFunctionType5 pFn) { fn_get_remmote_rfid = pFn; }
void ALARM::set_fn_set_remote_id(_callbackFunctionType6 pFn) { fn_set_remote_id = pFn; }
void ALARM::set_fn_set_rfid(_callbackFunctionType6 pFn) { fn_set_sens_rfid = pFn; }

// -----------------------------------------------------------------------------
// Debug / utility helpers
// -----------------------------------------------------------------------------

void ALARM::get_data() {
    if (pZoneManager == nullptr) return;

    char nameBuf[32];
    for (int i = 0; i < TOTAL_DEVICES; i++) {
        if (pZoneManager->getName(i, nameBuf, sizeof(nameBuf))) {
            Serial.println(nameBuf);
        }
    }
}

void ALARM::update_rx_rf_id_for_all(const char* rf_id_rx) {
    Serial.print(F("rf id from main fn>"));
    Serial.println(rf_id_rx);
    strncpy(recived_rf_id, rf_id_rx, sizeof(recived_rf_id) - 1);
    recived_rf_id[sizeof(recived_rf_id) - 1] = '\0';
    rf_id_rx_updated = true;
}

void ALARM::set_arm_mode(eARM_Mode mode) { eArm_mode = mode; }
eARM_Mode ALARM::get_arm_mode() { return eArm_mode; }



// -----------------------------------------------------------------------------
// Zone configuration / metadata via ZoneManager
// -----------------------------------------------------------------------------


void ALARM::clear_exit_zone() {
    if (pZoneManager == nullptr) return;
    for (int i = 0; i < TOTAL_DEVICES; i++) {
        pZoneManager->setExitDelay(i, false, false);
    }
    pZoneManager->save();
    pAny_sensor_array = pZoneManager->zones();
    sync_zone_manager_to_engine();
}

void ALARM::clear_entry_zone() {
    if (pZoneManager == nullptr) return;
    for (int i = 0; i < TOTAL_DEVICES; i++) {
        pZoneManager->setEntryDelay(i, false, false);
    }
    pZoneManager->save();
    pAny_sensor_array = pZoneManager->zones();
    sync_zone_manager_to_engine();
}


int8_t ALARM::get_entry_zone_availablity() {
    for (int index = 0; index < TOTAL_DEVICES; index++) {
        if (pZoneManager->isEntryDelay(index)) return index;
    }
    return -1;
}

int8_t ALARM::get_exit_zone_availablity() {
    for (int index = 0; index < TOTAL_DEVICES; index++) {
        if (pZoneManager->isExitDelay(index)) return index;
    }
    return -1;
}

// -----------------------------------------------------------------------------
// Timer settings / state getters
// -----------------------------------------------------------------------------

uint8_t ALARM::get_bell_time_timer_interval() { return Timer_alarm_relay_time_out; }
uint8_t ALARM::get_entry_delay_time() { return Timer_entry_delay_interval; }
uint8_t ALARM::get_exit_delay_time() { return Timer_exit_delay_interval; }

void ALARM::set_system_state(eMain_state state, eInvoking_source invoker, uint8_t user_id) {
    _invorking_device = invoker;
    _eCurrunt_state = state;
    this->user_id = user_id;
}

void ALARM::set_entry_delay_timer_interval(uint8_t interval) {
    Timer_entry_delay_interval = interval;
    Timer_entry_delay.interval = interval * 1000;
}

void ALARM::set_exit_delay_timer_interval(uint8_t interval) {
    Timer_exit_delay_interval = interval;
    Timer_exit_delay.interval = interval * 1000;
}

void ALARM::set_bell_time_timer_interval(uint8_t interval) {
    Timer_alarm_relay_time_out = interval;
    Timer_alarm_relay.interval = interval * 1000;
}

eMain_state ALARM::get_system_state() { return _eCurrunt_state; }

// -----------------------------------------------------------------------------
// Zone runtime updates
// -----------------------------------------------------------------------------

void ALARM::Universal_zone_state_update(uint8_t zone_index) {
    if (!zoneIndexValid(zone_index)) {
        Serial.println(F("invalid zone index"));
        return;
    }

    if (zone >= TOTAL_DEVICES) {
        return;
    }

    last_update_sensor_index = zone;
    sensor_state_updated_to_be_processd = true;
}

// -----------------------------------------------------------------------------
// Main runtime state machine
// -----------------------------------------------------------------------------

void ALARM::watcher() {
    if (pAny_sensor_array == nullptr) return;

    // RF zone reactive timer
    if (Timer_RF_zone_reactive_en && Timer_RF_zone_reactive_delay.Timer_run()) {
        Timer_RF_zone_reactive_en = false;
        for (uint8_t index = 0; index < TOTAL_DEVICES; index++) {
            if (pZoneManager->isRF(index)) {
                pAny_sensor_array[index].device_state &= ~(1 << BIT_MASK_LAST_STATE);
                pAny_sensor_array[index].device_state &= ~(1 << BIT_MASK_ALARM);
                pAny_sensor_array[index].device_state |= (1 << BIT_MASK_ENABLE);
                if (fn_set_rf_zone_re_enable != nullptr) fn_set_rf_zone_re_enable(index);
                Serial.println(F("RF zone Reactivated"));
            }
        }
    }

    // Alarm relay timeout
    if (!_Timer_alarm_relay_time_out && Timer_alarm_relay.Timer_run()) {
        _Timer_alarm_relay_time_out = true;
        // if (fn_alarm_bell_time_out != nullptr) fn_alarm_bell_time_out();
    }

    // Alarm clear delay
    if (Timer_alarm_clear_delay_en && Timer_alarm_clear_delay.Timer_run()) {
        Timer_alarm_clear_delay_en = false;
        enable_only_closed_sensors_as_it_is();
        clear_all_sensors_alarm_state();
        Serial.println(F("all zones alarm cleared"));
    }

    switch (_eCurrunt_state) {
        case DEACTIVE: {
            if (_ePrev_state != _eCurrunt_state) {
                Serial.println(F("SYS_STATE>DEACTIVE"));
                sensor_state_updated_to_be_processd = true;
                clear_all_sensors_alarm_state();

                if (_ePrev_state != TRANCITION && fn_disarm != nullptr) {
                    fn_disarm(user_id, "disarm", _invorking_device);
                }

                _ePrev_state = _eCurrunt_state;
                eArm_mode = USER_SELECT;
                if (fn_intializ_sensors != nullptr) fn_intializ_sensors();
            }

            if (sensor_state_updated_to_be_processd) {
                sensor_state_updated_to_be_processd = false;
                chime_sound();
                alarm_process_wired_24H();

                if (is_system_ready_to_arm() != -1) {
                    if (fn_system_is_not_ready != nullptr) fn_system_is_not_ready();
                } else {
                    if (fn_system_is_ready != nullptr) fn_system_is_ready();
                }
            }
        } break;

        case TRANCITION: {
            if (_ePrev_state != _eCurrunt_state) {
                _ePrev_state = _eCurrunt_state;
                Serial.println(F("SYS_STATE>TRANCITION"));
                if (fn_arm_faild != nullptr) fn_arm_faild("", is_system_ready_to_arm());
                Timer_arm_mode_waiting_time_out.previousMillis = millis();
                Timer_arm_mode_waiting_time_out.interval = 15000;
            }

            if (Timer_arm_mode_waiting_time_out.Timer_run()) {
                if (fn_arm_can_not_be_done != nullptr) fn_arm_can_not_be_done();
                _Arm_faild_detected = true;
                _eCurrunt_state = DEACTIVE;
            }

            if (eArm_mode == AS_ITIS_BYPASS) {
                _eCurrunt_state = SYS1_IDEAL;
                Serial.println(F("AS_ITIS_BYPASS"));
            } else if (eArm_mode == AS_ITIS_NO_BYPASS) {
                _eCurrunt_state = SYS1_IDEAL;
                Serial.println(F("AS_ITIS_NO_BYPASS"));
            }
        } break;

        case SYS1_IDEAL: {
            if (_ePrev_state != _eCurrunt_state) {
                Serial.println(F("SYS_STATE>SYS_IDEAL"));

                if (_ePrev_state == BEGING) {
                    eArm_mode = AS_ITIS_NO_BYPASS;
                }

                if (_ePrev_state == ALARM_CALLING) {
                    Serial.println(F("alarm snoozed"));
                    clear_all_sensors_alarm_state();
                    if (fn_alarm_snooze != nullptr) fn_alarm_snooze();
                    _exit_delay_timer_en = false;
                } else if ((_ePrev_state == DEACTIVE) || (_ePrev_state == BEGING) || (_ePrev_state == TRANCITION)) {
                    clear_all_sensors_alarm_state();
                    if (fn_intializ_sensors != nullptr) fn_intializ_sensors();

                    if ((is_system_ready_to_arm() != -1) && (eArm_mode == USER_SELECT)) {
                        _ePrev_state = _eCurrunt_state;
                        _eCurrunt_state = TRANCITION;
                        return;
                    } else {
                        _Arm_faild_detected = false;
                        int8_t exit_zone = get_exit_zone_availablity();
                        if (exit_zone != -1) {
                            _exit_delay_timer_en = true;
                            Timer_exit_delay.previousMillis = millis();
                            Timer_exit_delay.interval = Timer_exit_delay_interval * 1000;
                            if (fn_exit_delay_timer_start != nullptr) fn_exit_delay_timer_start();
                        }

                        enable_only_closed_sensors_as_it_is();
                        if (fn_arm != nullptr) fn_arm(user_id, "sensor", _invorking_device);
                    }
                }

                _ePrev_state = _eCurrunt_state;
            }

            if (sensor_state_updated_to_be_processd) {
                sensor_state_updated_to_be_processd = false;
                alarm_process_wired();
            }

            if (_exit_delay_timer_en && Timer_exit_delay.Timer_run()) {
                _exit_delay_timer_en = false;

                for (int index = 0; index < TOTAL_DEVICES; index++) {
                    if (pZoneManager->isExitDelay(index) && (pAny_sensor_array[index].device_state & (1 << BIT_MASK_LAST_STATE))) {
                        Serial.print(F("sensor exit delay>"));
                        Serial.println(index);
                        if (fn_alarm_notify != nullptr) fn_alarm_notify(index);
                        if (fn_exit_delay_time_out != nullptr) fn_exit_delay_time_out("Exit", index);
                        return;
                    }
                }

                if (fn_exit_delay_time_out != nullptr) fn_exit_delay_time_out("Exit", -1);
            }

            if (_entry_delay_timer_en) {
                Timer_RF_zone_reactive_en = false;

                if (Timer_entry_delay.Timer_run()) {
                    _entry_delay_timer_en = false;
                    for (int index = 0; index < TOTAL_DEVICES; index++) {
                        if (pAny_sensor_array[index].device_state & (1 << BIT_MASK_ALARM)) {
                            Timer_RF_zone_reactive_en = true;
                            if (fn_alarm_notify != nullptr) fn_alarm_notify(index);
                            _eCurrunt_state = ALARM_CALLING;
                            if (fn_entry_delay_time_out != nullptr) fn_entry_delay_time_out("no msg", _invorking_device);
                        }
                    }
                }
            }
        } break;

        case ALARM_SMS_SENDING: {
            if (_ePrev_state != _eCurrunt_state) {
                _ePrev_state = _eCurrunt_state;
                Serial.println(F("SYS_STATE>SMS_SENDING"));
            }
            _eCurrunt_state = ALARM_CALLING;
        } break;

        case ALARM_CALLING: {
            if (_ePrev_state != _eCurrunt_state) {
                _ePrev_state = _eCurrunt_state;
                Serial.println(F("SYS_STATE>ALARM_CALLING"));
                Timer_alarm_relay.previousMillis = millis();
                _Timer_alarm_relay_time_out = false;
                if (fn_alarm_calling != nullptr) fn_alarm_calling();
            }

            if (sensor_state_updated_to_be_processd) {
                sensor_state_updated_to_be_processd = false;
                alarm_process_wired();
            }
        } break;
    }
}

// -----------------------------------------------------------------------------
// Alarm helpers / core logic
// -----------------------------------------------------------------------------

void ALARM::enable_only_closed_sensors_as_it_is() {
    if (pAny_sensor_array == nullptr) return;

    for (int scanning_index = 0; scanning_index < TOTAL_DEVICES; scanning_index++) {
        Serial.printf_P(PSTR("Zone ID>>%d "), scanning_index);

        if (pAny_sensor_array[scanning_index].device_state & (1 << BIT_MASK_BYPASSED)) {
            Serial.println(F("BYPASSED"));
            continue;
        }

        if (pAny_sensor_array[scanning_index].device_type & (1 << BIT_MASK_RF)) {
            Serial.print(F("RF_ZONE."));
        } else if (!(pAny_sensor_array[scanning_index].device_state & (1 << BIT_MASK_AVAILABLE))) {
            Serial.println(F("UNAVAILABLE."));
            continue;
        }

        if (pAny_sensor_array[scanning_index].device_state & (1 << BIT_MASK_LAST_STATE)) {
            pAny_sensor_array[scanning_index].device_state &= ~(1 << BIT_MASK_ENABLE);
            Serial.println(F("DISABLE"));
        } else {
            pAny_sensor_array[scanning_index].device_state |= (1 << BIT_MASK_ENABLE);
            Serial.println(F("ENABLE"));
        }
    }
}

int8_t ALARM::is_system_ready_to_arm() {
    if (pAny_sensor_array == nullptr) return -1;

    for (int i = 0; i < TOTAL_DEVICES; i++) {
        if ((pAny_sensor_array[i].device_state & (1 << BIT_MASK_LAST_STATE)) &&
            !(pAny_sensor_array[i].device_state & (1 << BIT_MASK_BYPASSED))) {
            return i;
        }
    }
    return -1;
}

void ALARM::clear_all_sensors_alarm_state() {
    if (pAny_sensor_array == nullptr) return;

    Serial.println(F("All sensors alarm clear"));
    for (int i = 0; i < TOTAL_DEVICES; i++) {
        pAny_sensor_array[i].device_state &= ~(1 << BIT_MASK_ALARM);
    }
}

void ALARM::alarm_process_wired_24H() {
    if (!zoneIndexValid(last_update_sensor_index)) return;

    if (pAny_sensor_array[last_update_sensor_index].device_state & (1 << BIT_MASK_BYPASSED)) {
        return;
    }

    if ((pAny_sensor_array[last_update_sensor_index].device_state & (1 << BIT_MASK_LAST_STATE)) &&
        (pAny_sensor_array[last_update_sensor_index].device_type & (1 << BIT_MASK_24H))) {
        pAny_sensor_array[last_update_sensor_index].last_updated_time_stamp = millis();
        pAny_sensor_array[last_update_sensor_index].device_state |= (1 << BIT_MASK_ALARM);
        if (fn_alarm_notify != nullptr) fn_alarm_notify(last_update_sensor_index);
        _eCurrunt_state = ALARM_CALLING;
    } else if (pAny_sensor_array[last_update_sensor_index].device_state & (1 << BIT_MASK_LAST_STATE)) {
        if (fn_chime_zone_notify != nullptr) fn_chime_zone_notify("", last_update_sensor_index);
    }
}



void ALARM::any_zone_bitmask_parameter_to_bytes(uint8_t bit_mask,
                                                uint8_t& zone0_7,
                                                uint8_t& zone8_15,
                                                uint8_t& zone16_23,
                                                uint8_t& zone24_31,
                                                uint8_t& zone32_39,
                                                uint8_t& zone40_47) {
    if (pAny_sensor_array == nullptr) return;

    uint8_t bit_mask_a = 0, bit_mask_b = 0, bit_mask_c = 0, bit_mask_d = 0, bit_mask_e = 0, bit_mask_f = 0;
    for (uint8_t index = 0; index < 48; index++) {
        if ((0 <= index) && (index < 8)) {
            if (pAny_sensor_array[index].device_state & (1 << bit_mask)) zone0_7 |= (1 << bit_mask_a);
            else zone0_7 &= ~(1 << bit_mask_a);
            bit_mask_a++;
        } else if ((8 <= index) && (index < 16)) {
            if (pAny_sensor_array[index].device_state & (1 << bit_mask)) zone8_15 |= (1 << bit_mask_b);
            else zone8_15 &= ~(1 << bit_mask_b);
            bit_mask_b++;
        } else if ((16 <= index) && (index < 24)) {
            if (pAny_sensor_array[index].device_state & (1 << bit_mask)) zone16_23 |= (1 << bit_mask_c);
            else zone16_23 &= ~(1 << bit_mask_c);
            bit_mask_c++;
        } else if ((24 <= index) && (index < 32)) {
            if (pAny_sensor_array[index].device_state & (1 << bit_mask)) zone24_31 |= (1 << bit_mask_d);
            else zone24_31 &= ~(1 << bit_mask_d);
            bit_mask_d++;
        } else if ((32 <= index) && (index < 40)) {
            if (pAny_sensor_array[index].device_state & (1 << bit_mask)) zone32_39 |= (1 << bit_mask_e);
            else zone32_39 &= ~(1 << bit_mask_e);
            bit_mask_e++;
        } else if ((40 <= index) && (index < 48)) {
            if (pAny_sensor_array[index].device_state & (1 << bit_mask)) zone40_47 |= (1 << bit_mask_f);
            else zone40_47 &= ~(1 << bit_mask_f);
            bit_mask_f++;
        }
    }
}

void ALARM::chime_sound() {
    uint8_t scanning_index = last_update_sensor_index;
    if (!zoneIndexValid(scanning_index)) return;

    if (pAny_sensor_array[scanning_index].device_state & (1 << BIT_MASK_BYPASSED)) return;

    if (!(pAny_sensor_array[scanning_index].device_state & (1 << BIT_MASK_AVAILABLE))) {
        if (!(pAny_sensor_array[scanning_index].device_type & (1 << BIT_MASK_RF))) {
            return;
        }
    }

    if (!(pAny_sensor_array[scanning_index].device_type & (1 << BIT_MASK_SILENT))) {
        Serial.println(F("NO CHIME"));
        return;
    }

    if (pAny_sensor_array[scanning_index].device_state & (1 << BIT_MASK_LAST_STATE)) {
        if (fn_chime_sound != nullptr) fn_chime_sound();
    }
}





void ALARM::alarm_process_wired(uint8_t zone) {
 

   if (zone >= TOTAL_DEVICES) {
        return;
    }

    LOG_LOGF_P(PSTR("ID>>%d "), zone);

    if (is_sensor_skipped(zone)) {
        return;
    }

    bool alarm_enable = false;

    ZoneState state = gZoneEngine.getState(zone);

    if (state == ZS_OPEN) {
        process_open_sensor(zone, alarm_enable);
    } else {
        process_closed_sensor(zone);
    }

    if (alarm_enable) {
        handle_alarm_trigger(zone);
    }
}

bool ALARM::is_sensor_skipped(uint8_t index) {
    auto& sensor = pAny_sensor_array[index];

    if (sensor.device_state & (1 << BIT_MASK_BYPASSED)) {
        LOG_PRINTLN(F("BYPASSED"));
        return true;
    }

    if (sensor.device_type & (1 << BIT_MASK_RF)) {
        LOG_PRINT(F("RF_ZONE."));
    } else if (!(sensor.device_state & (1 << BIT_MASK_AVAILABLE))) {
        LOG_PRINTLN(F("N/A."));
        return true;
    }

    return false;
}

void ALARM::process_open_sensor(uint8_t index, bool& alarm_enable) {
    auto& sensor = pAny_sensor_array[index];

    LOG_PRINT(F(">>OPEND"));

    const bool is_enabled = (sensor.device_state & (1 << BIT_MASK_ENABLE)) != 0;
    const bool is_alarm   = (sensor.device_state & (1 << BIT_MASK_ALARM)) != 0;
    const bool is_entry   = (sensor.device_state & (1 << BIT_MASK_ENTRY_DELAY)) != 0;
    const bool is_exit    = (sensor.device_state & (1 << BIT_MASK_EXIT_DELAY)) != 0;

    if (!is_enabled) {
        LOG_PRINTLN(F(">>>DISABLED/"));
        return;
    }

    LOG_PRINT(F(">>>Enable/"));

    if (!is_entry && !is_exit) {
        LOG_PRINT(F("ENTRY-NO/EXIT-NO"));
        if (!is_alarm) {
            sensor.device_state |= (1 << BIT_MASK_ALARM);
            LOG_PRINT(F(">NO PREV ALARM - ALARM ENABLED"));
            alarm_enable = true;
        } else {
            LOG_PRINTLN(F(">ALREADY ALARM OR DISABLE"));
        }
    } else if (is_entry && is_exit) {
        LOG_PRINT(F("ENTRY-YES/EXIT-YES"));
        if (!_exit_delay_timer_en && !_entry_delay_timer_en) {
            LOG_PRINTLN(F("ENTRY_TIMER_ACTIVATED"));
            sensor.device_state |= (1 << BIT_MASK_ALARM);
            _entry_delay_timer_en = true;
            if (fn_entry_delay_timer_start != nullptr) fn_entry_delay_timer_start();
            Timer_entry_delay.previousMillis = millis();
        } else {
            LOG_PRINTLN(F(">MAY BE ALREADY_ALARM/DISABLE/ EXIT_TIMER_RUNNING/ ENTRY_TIMER_RUNNING"));
        }
    } else if (is_entry && !is_exit) {
        LOG_PRINT(F("ENTRY-YES/EXIT-NO"));
        if (!is_alarm) {
            if (!_entry_delay_timer_en) {
                LOG_PRINTLN(F("ENTRY_TIMER_ACTIVATED"));
                sensor.device_state |= (1 << BIT_MASK_ALARM);
                _entry_delay_timer_en = true;
                Timer_entry_delay.previousMillis = millis();
            }
            if (_exit_delay_timer_en) {
                LOG_PRINTLN(F("entry delay only zone trigger when exit delay timer running"));
                sensor.device_state |= (1 << BIT_MASK_ALARM);
                alarm_enable = true;
            }
        } else {
            LOG_PRINTLN(F("ALREADY ALARM OR DISABLED"));
        }
    } else if (!is_entry && is_exit) {
        LOG_PRINT(F("ENTRY-NO/EXIT-YES"));
        if (!is_alarm && !_exit_delay_timer_en) {
            LOG_PRINTLN(F("exit delay only zone triggered (no timer running)"));
            sensor.device_state |= (1 << BIT_MASK_ALARM);
            alarm_enable = true;
        }
    }
}

void ALARM::process_closed_sensor(uint8_t index) {
    auto& sensor = pAny_sensor_array[index];

    LOG_PRINT(F(">>CLOSED"));
    LOG_PRINTLN(F(">>>will be enabled if arm mode is AS_ITIS_NO_BYPASS"));

    if (eArm_mode == AS_ITIS_NO_BYPASS) {
        sensor.device_state |= (1 << BIT_MASK_ENABLE);
        LOG_PRINTLN(F("ENABLED"));

        if (is_system_ready_to_arm() != -1) {
            if (fn_system_is_not_ready != nullptr) fn_system_is_not_ready();
        } else {
            if (fn_system_is_ready != nullptr) fn_system_is_ready();
        }
    }
}

void ALARM::handle_alarm_trigger(uint8_t index) {
    auto& sensor = pAny_sensor_array[index];

    sensor.last_updated_time_stamp = millis();
    _eCurrunt_state = ALARM_CALLING;
    _Timer_alarm_relay_time_out = false;
    Timer_alarm_relay.previousMillis = millis();

    sensor.device_state |= (1 << BIT_MASK_ALARM);
    if (fn_alarm_notify != nullptr) fn_alarm_notify(index);

    if (perimeter_only) {
        if (sensor.device_type & (1 << BIT_MASK_PERIMETER)) {
            LOG_PRINT(F("alarm detected perimeter only>> "));
            Timer_alarm_clear_delay.interval = 10000;
        } else {
            LOG_PRINT(F("not a perimeter zone alarm canceled "));
            return;
        }
    } else {
        LOG_PRINTLN(F("alarm detected all arm>> "));
        Timer_alarm_clear_delay.interval = 10000;
    }

    Timer_alarm_clear_delay.previousMillis = millis();
    Timer_alarm_clear_delay_en = true;
}

// -----------------------------------------------------------------------------
// Misc helpers
// -----------------------------------------------------------------------------

int8_t ALARM::get_absolute_zone_number(eInvoking_source card_id, uint8_t relative_zone_number) {
    switch (card_id) {
        case CARD_A: return relative_zone_number;
        case CARD_B: return relative_zone_number + card_A_zone__starting_number;
        case CARD_C: return relative_zone_number + card_A_zone__starting_number + card_B_zone__starting_number;
        default:
            Serial.println(F("unknown card id"));
            return -1;
    }
}

