#include "alarm.h"

#include "ZoneManager.h"
#include "mapping/zone_map.h"

#ifdef BIT_MASK_ENTRY_DELAY
#undef BIT_MASK_ENTRY_DELAY
#endif
#ifdef BIT_MASK_EXIT_DELAY
#undef BIT_MASK_EXIT_DELAY
#endif
#ifdef BIT_MASK_BYPASSED
#undef BIT_MASK_BYPASSED
#endif
#ifdef BIT_MASK_RF
#undef BIT_MASK_RF
#endif
#ifdef BIT_MASK_PERIMETER
#undef BIT_MASK_PERIMETER
#endif
#ifdef BIT_MASK_24H
#undef BIT_MASK_24H
#endif
#ifdef BIT_MASK_SILENT
#undef BIT_MASK_SILENT
#endif



// -----------------------------------------------------------------------------
// Constructor / dependency attachment
// -----------------------------------------------------------------------------

ALARM::ALARM()
    : pZoneManager(nullptr),
      pZoneEngine(nullptr),
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
      Timer_alarm_relay_time_out(0) {
    memset(zone_name_cache, 0, sizeof(zone_name_cache));
    memset(zone_open, 0, sizeof(zone_open));
    memset(zone_available, 0, sizeof(zone_available));
    memset(zone_enabled, 0, sizeof(zone_enabled));
    memset(zone_alarm, 0, sizeof(zone_alarm));
    memset(zone_last_alarm_ms, 0, sizeof(zone_last_alarm_ms));
}
      
void ALARM::attachZoneManager(ZoneManager* zm) {
    pZoneManager = zm;
    pZoneEngine = &zoneEngine;
    sync_zone_manager_to_engine();
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
    sync_zone_manager_to_engine();
}

void ALARM::sync_zone_manager_to_engine() {
    if ((pZoneManager == nullptr) || (pZoneEngine == nullptr)) return;
    pZoneManager->syncToEngine(*pZoneEngine);
}

void ALARM::clear_entry_zone() {
    if (pZoneManager == nullptr) return;
    for (int i = 0; i < TOTAL_DEVICES; i++) {
        pZoneManager->setEntryDelay(i, false, false);
    }
    pZoneManager->save();
    sync_zone_manager_to_engine();
}

void ALARM::set_sensor_name(uint8_t sensor_index, const char* sensor_name) {
    if (!zoneIndexValid(sensor_index) || (pZoneManager == nullptr) || (sensor_name == nullptr)) return;
    pZoneManager->setName(sensor_index, sensor_name);
}

void ALARM::set_sensor_bypassed(uint8_t sensor_index, bool state) {
    if (!zoneIndexValid(sensor_index) || (pZoneManager == nullptr)) return;
    pZoneManager->setBypassed(sensor_index, state);
    sync_zone_manager_to_engine();
}

void ALARM::set_sensor_24H(uint8_t sensor_index, bool state) {
    if (!zoneIndexValid(sensor_index) || (pZoneManager == nullptr)) return;
    pZoneManager->set24h(sensor_index, state);
    sync_zone_manager_to_engine();
}

void ALARM::set_sensor_en_de(uint8_t sensor_index, bool state) {
    if (!zoneIndexValid(sensor_index)) return;
    setZoneEnabled(sensor_index, state);
}

void ALARM::set_sensor_entry(uint8_t sensor_index, bool state) {
    if (!zoneIndexValid(sensor_index) || (pZoneManager == nullptr)) return;
    pZoneManager->setEntryDelay(sensor_index, state);
    sync_zone_manager_to_engine();
}

void ALARM::set_sensor_exit(uint8_t sensor_index, bool state) {
    if (!zoneIndexValid(sensor_index) || (pZoneManager == nullptr)) return;
    pZoneManager->setExitDelay(sensor_index, state);
    sync_zone_manager_to_engine();
}

bool ALARM::is_sensor_enable(uint8_t index) { return zoneIndexValid(index) && zoneIsEnabled(index); }
bool ALARM::is_sensor_bypass(uint8_t index) { return zoneIndexValid(index) && (pZoneManager != nullptr) && pZoneManager->isBypassed(index); }
bool ALARM::is_sensor_available(uint8_t index) { return zoneIndexValid(index) && zoneIsAvailable(index); }
bool ALARM::is_sensor_24h(uint8_t zone) { return zoneIndexValid(zone) && (pZoneManager != nullptr) && pZoneManager->is24h(zone); }
bool ALARM::is_sensor_RF(uint8_t zone) { return zoneIndexValid(zone) && (pZoneManager != nullptr) && pZoneManager->isRF(zone); }
bool ALARM::is_sensor_exit_zone(uint8_t zone) { return zoneIndexValid(zone) && (pZoneManager != nullptr) && pZoneManager->isExitDelay(zone); }
bool ALARM::is_sensor_entry_zone(uint8_t zone) { return zoneIndexValid(zone) && (pZoneManager != nullptr) && pZoneManager->isEntryDelay(zone); }

bool ALARM::is_sensor_ready(uint8_t zone) {
    if (!zoneIndexValid(zone)) return false;
    if ((pZoneManager != nullptr) && pZoneManager->isRF(zone)) return true;
    return !zoneIsOpen(zone) || ((pZoneManager != nullptr) && pZoneManager->isBypassed(zone));
}

char* ALARM::get_sensor_name(uint8_t index) {
    if (!zoneIndexValid(index)) return nullptr;

    zone_name_cache[index][0] = '\0';
    if ((pZoneManager != nullptr) && pZoneManager->getName(index, zone_name_cache[index], sizeof(zone_name_cache[index]))) {
        return zone_name_cache[index];
    }
    return zone_name_cache[index];
}


int8_t ALARM::get_entry_zone_availablity() {
    for (int index = 0; index < TOTAL_DEVICES; index++) {
        if (!isZoneActive(index)) continue;
        if (pZoneManager->isEntryDelay(index)) return index;
    }
    return -1;
}

int8_t ALARM::get_exit_zone_availablity() {
    for (int index = 0; index < TOTAL_DEVICES; index++) {
        if (!isZoneActive(index)) continue;
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

    last_update_sensor_index = zone_index;

    const ZoneState state = zoneEngine.getState(zone_index);
    if (state == ZS_OPEN) {
        setZoneOpen(zone_index, true);
        setZoneAvailable(zone_index, true);
    } else if (state == ZS_CLOSE) {
        setZoneOpen(zone_index, false);
        setZoneAvailable(zone_index, true);
    } else {
        setZoneOpen(zone_index, false);
        setZoneAvailable(zone_index, false);
    }

    last_update_sensor_state = (state == ZS_OPEN);
    sensor_state_updated_to_be_processd = true;
}

// -----------------------------------------------------------------------------
// Main runtime state machine
// -----------------------------------------------------------------------------

void ALARM::watcher() {
    if ((pZoneManager == nullptr) || (pZoneEngine == nullptr)) return;

    // RF zone reactive timer
    if (Timer_RF_zone_reactive_en && Timer_RF_zone_reactive_delay.Timer_run()) {
        Timer_RF_zone_reactive_en = false;
        for (uint8_t index = 0; index < TOTAL_DEVICES; index++) {
            if (!isZoneActive(index)) continue;
            if (pZoneManager->isRF(index)) {
                setZoneOpen(index, false);
                setZoneAlarmed(index, false);
                setZoneEnabled(index, true);
                setZoneAvailable(index, true);
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
                alarm_process_wired(last_update_sensor_index);
            }

            if (_exit_delay_timer_en && Timer_exit_delay.Timer_run()) {
                _exit_delay_timer_en = false;

                for (int index = 0; index < TOTAL_DEVICES; index++) {
                    if (!isZoneActive(index)) continue;
                    if (pZoneManager->isExitDelay(index) && zoneIsOpen(index)) {
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
                        if (!isZoneActive(index)) continue;
                        if (zoneIsAlarmed(index)) {
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
                alarm_process_wired(last_update_sensor_index);
            }
        } break;
    }
}

// -----------------------------------------------------------------------------
// Alarm helpers / core logic
// -----------------------------------------------------------------------------

void ALARM::enable_only_closed_sensors_as_it_is() {
    if (pZoneManager == nullptr) return;

    for (int scanning_index = 0; scanning_index < TOTAL_DEVICES; scanning_index++) {
        if (!isZoneActive(scanning_index)) continue;
        Serial.printf_P(PSTR("Zone ID>>%d "), scanning_index);

        if (pZoneManager->isBypassed(scanning_index)) {
            Serial.println(F("BYPASSED"));
            continue;
        }

        if (pZoneManager->isRF(scanning_index)) {
            Serial.print(F("RF_ZONE."));
        } else if (!zoneIsAvailable(scanning_index)) {
            Serial.println(F("UNAVAILABLE."));
            continue;
        }

        if (zoneIsOpen(scanning_index)) {
            setZoneEnabled(scanning_index, false);
            Serial.println(F("DISABLE"));
        } else {
            setZoneEnabled(scanning_index, true);
            Serial.println(F("ENABLE"));
        }
    }
}

int8_t ALARM::is_system_ready_to_arm() {
    if (pZoneManager == nullptr) return -1;

    for (int i = 0; i < TOTAL_DEVICES; i++) {
        if (!isZoneActive(i)) continue;
        if (zoneIsOpen(i) && !pZoneManager->isBypassed(i)) {
            return i;
        }
    }
    return -1;
}

void ALARM::clear_all_sensors_alarm_state() {
    Serial.println(F("All sensors alarm clear"));
    for (int i = 0; i < TOTAL_DEVICES; i++) {
        setZoneAlarmed(i, false);
    }
}

void ALARM::alarm_process_wired_24H() {
    if (!zoneIndexValid(last_update_sensor_index)) return;

    if ((pZoneManager != nullptr) && pZoneManager->isBypassed(last_update_sensor_index)) {
        return;
    }

    if (zoneIsOpen(last_update_sensor_index) &&
        (pZoneManager != nullptr) && pZoneManager->is24h(last_update_sensor_index)) {
        zone_last_alarm_ms[last_update_sensor_index] = millis();
        setZoneAlarmed(last_update_sensor_index, true);
        if (fn_alarm_notify != nullptr) fn_alarm_notify(last_update_sensor_index);
        _eCurrunt_state = ALARM_CALLING;
    } else if (zoneIsOpen(last_update_sensor_index)) {
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
    uint8_t bit_mask_a = 0, bit_mask_b = 0, bit_mask_c = 0, bit_mask_d = 0, bit_mask_e = 0, bit_mask_f = 0;
    for (uint8_t index = 0; index < ZONE_COUNT; index++) {
        if ((0 <= index) && (index < 8)) {
            if (getZoneBitValue(bit_mask, index)) zone0_7 |= (1 << bit_mask_a);
            else zone0_7 &= ~(1 << bit_mask_a);
            bit_mask_a++;
        } else if ((8 <= index) && (index < 16)) {
            if (getZoneBitValue(bit_mask, index)) zone8_15 |= (1 << bit_mask_b);
            else zone8_15 &= ~(1 << bit_mask_b);
            bit_mask_b++;
        } else if ((16 <= index) && (index < 24)) {
            if (getZoneBitValue(bit_mask, index)) zone16_23 |= (1 << bit_mask_c);
            else zone16_23 &= ~(1 << bit_mask_c);
            bit_mask_c++;
        } else if ((24 <= index) && (index < 32)) {
            if (getZoneBitValue(bit_mask, index)) zone24_31 |= (1 << bit_mask_d);
            else zone24_31 &= ~(1 << bit_mask_d);
            bit_mask_d++;
        } else if ((32 <= index) && (index < 40)) {
            if (getZoneBitValue(bit_mask, index)) zone32_39 |= (1 << bit_mask_e);
            else zone32_39 &= ~(1 << bit_mask_e);
            bit_mask_e++;
        } else if ((40 <= index) && (index < 48)) {
            if (getZoneBitValue(bit_mask, index)) zone40_47 |= (1 << bit_mask_f);
            else zone40_47 &= ~(1 << bit_mask_f);
            bit_mask_f++;
        }
    }
}

void ALARM::chime_sound() {
    uint8_t scanning_index = last_update_sensor_index;
    if (!zoneIndexValid(scanning_index)) return;

    if ((pZoneManager != nullptr) && pZoneManager->isBypassed(scanning_index)) return;

    if (!zoneIsAvailable(scanning_index)) {
        if ((pZoneManager == nullptr) || !pZoneManager->isRF(scanning_index)) {
            return;
        }
    }

    if ((pZoneManager == nullptr) || !pZoneManager->isChime(scanning_index)) {
        Serial.println(F("NO CHIME"));
        return;
    }

    if (zoneIsOpen(scanning_index)) {
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

    ZoneState state = zoneEngine.getState(zone);

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
    if ((pZoneManager != nullptr) && pZoneManager->isBypassed(index)) {
        LOG_PRINTLN(F("BYPASSED"));
        return true;
    }

    if ((pZoneManager != nullptr) && pZoneManager->isRF(index)) {
        LOG_PRINT(F("RF_ZONE."));
    } else if (!zoneIsAvailable(index)) {
        LOG_PRINTLN(F("N/A."));
        return true;
    }

    return false;
}

void ALARM::process_open_sensor(uint8_t index, bool& alarm_enable) {
    LOG_PRINT(F(">>OPEND"));

    const bool is_enabled = zoneIsEnabled(index);
    const bool is_alarm   = zoneIsAlarmed(index);
    const bool is_entry   = (pZoneManager != nullptr) && pZoneManager->isEntryDelay(index);
    const bool is_exit    = (pZoneManager != nullptr) && pZoneManager->isExitDelay(index);

    if (!is_enabled) {
        LOG_PRINTLN(F(">>>DISABLED/"));
        return;
    }

    LOG_PRINT(F(">>>Enable/"));

    if (!is_entry && !is_exit) {
        LOG_PRINT(F("ENTRY-NO/EXIT-NO"));
        if (!is_alarm) {
            setZoneAlarmed(index, true);
            LOG_PRINT(F(">NO PREV ALARM - ALARM ENABLED"));
            alarm_enable = true;
        } else {
            LOG_PRINTLN(F(">ALREADY ALARM OR DISABLE"));
        }
    } else if (is_entry && is_exit) {
        LOG_PRINT(F("ENTRY-YES/EXIT-YES"));
        if (!_exit_delay_timer_en && !_entry_delay_timer_en) {
            LOG_PRINTLN(F("ENTRY_TIMER_ACTIVATED"));
            setZoneAlarmed(index, true);
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
                setZoneAlarmed(index, true);
                _entry_delay_timer_en = true;
                Timer_entry_delay.previousMillis = millis();
            }
            if (_exit_delay_timer_en) {
                LOG_PRINTLN(F("entry delay only zone trigger when exit delay timer running"));
                setZoneAlarmed(index, true);
                alarm_enable = true;
            }
        } else {
            LOG_PRINTLN(F("ALREADY ALARM OR DISABLED"));
        }
    } else if (!is_entry && is_exit) {
        LOG_PRINT(F("ENTRY-NO/EXIT-YES"));
        if (!is_alarm && !_exit_delay_timer_en) {
            LOG_PRINTLN(F("exit delay only zone triggered (no timer running)"));
            setZoneAlarmed(index, true);
            alarm_enable = true;
        }
    }
}

void ALARM::process_closed_sensor(uint8_t index) {
    LOG_PRINT(F(">>CLOSED"));
    LOG_PRINTLN(F(">>>will be enabled if arm mode is AS_ITIS_NO_BYPASS"));

    if (eArm_mode == AS_ITIS_NO_BYPASS) {
        setZoneEnabled(index, true);
        LOG_PRINTLN(F("ENABLED"));

        if (is_system_ready_to_arm() != -1) {
            if (fn_system_is_not_ready != nullptr) fn_system_is_not_ready();
        } else {
            if (fn_system_is_ready != nullptr) fn_system_is_ready();
        }
    }
}

void ALARM::handle_alarm_trigger(uint8_t index) {
    zone_last_alarm_ms[index] = millis();
    _eCurrunt_state = ALARM_CALLING;
    _Timer_alarm_relay_time_out = false;
    Timer_alarm_relay.previousMillis = millis();

    setZoneAlarmed(index, true);
    if (fn_alarm_notify != nullptr) fn_alarm_notify(index);

    if (perimeter_only) {
        if ((pZoneManager != nullptr) && pZoneManager->isPerimeter(index)) {
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

bool ALARM::zoneIsOpen(uint8_t index) const {
    return zoneIndexValid(index) && zone_open[index];
}

bool ALARM::zoneIsAvailable(uint8_t index) const {
    return zoneIndexValid(index) && zone_available[index];
}

bool ALARM::zoneIsEnabled(uint8_t index) const {
    return zoneIndexValid(index) && zone_enabled[index];
}

bool ALARM::zoneIsAlarmed(uint8_t index) const {
    return zoneIndexValid(index) && zone_alarm[index];
}

void ALARM::setZoneOpen(uint8_t index, bool isOpen) {
    if (!zoneIndexValid(index)) return;
    zone_open[index] = isOpen;
}

void ALARM::setZoneAvailable(uint8_t index, bool isAvailable) {
    if (!zoneIndexValid(index)) return;
    zone_available[index] = isAvailable;
}

void ALARM::setZoneEnabled(uint8_t index, bool isEnabled) {
    if (!zoneIndexValid(index)) return;
    zone_enabled[index] = isEnabled;
}

void ALARM::setZoneAlarmed(uint8_t index, bool isAlarmed) {
    if (!zoneIndexValid(index)) return;
    zone_alarm[index] = isAlarmed;
}

bool ALARM::getZoneBitValue(uint8_t bit_mask, uint8_t index) const {
    if (!zoneIndexValid(index)) return false;

    switch (bit_mask) {
        case BIT_MASK_EXIT_DELAY:
            return (pZoneManager != nullptr) && pZoneManager->isExitDelay(index);
        case BIT_MASK_ENTRY_DELAY:
            return (pZoneManager != nullptr) && pZoneManager->isEntryDelay(index);
        case BIT_MASK_ENABLE:
            return zone_enabled[index];
        case BIT_MASK_BYPASSED:
            return (pZoneManager != nullptr) && pZoneManager->isBypassed(index);
        case BIT_MASK_ALARM:
            return zone_alarm[index];
        case BIT_MASK_LAST_STATE:
            return zone_open[index];
        case BIT_MASK_AVAILABLE:
            return zone_available[index];
        default:
            return false;
    }
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

