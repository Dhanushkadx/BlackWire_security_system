#pragma once

namespace UniversalCmd {

// Example: SYS=LOG
static constexpr char kSysLog[] = "SYS=LOG";

// Example: SYS=REBOOT
static constexpr char kSysReboot[] = "SYS=REBOOT";

// Example: SYS=BLOCK
static constexpr char kSysBlock[] = "SYS=BLOCK";

// Example: SYS=RESET
static constexpr char kSysReset[] = "SYS=RESET";

// Example: SYS=STATUS
static constexpr char kSysStatus[] = "SYS=STATUS";

// Example: NET=AP,1
static constexpr char kNetApEnable[] = "NET=AP,1";

// Example: NET?
static constexpr char kNetQuery[] = "NET?";

// Example: AUTH=1234
static constexpr char kAuthPrefix[] = "AUTH=";

// Example: ZONE=03,NAME,Front Door
static constexpr char kZonePrefix[] = "ZONE=";

// Example: ZONE=03,NAME,Front Door
static constexpr char kZoneNameTag[] = ",NAME,";

// Example: ZONE=03,EXIT,0
static constexpr char kZoneParamExample[] = "ZONE=03,EXIT,0";

// Example: ARM=HOME
static constexpr char kArmHome[] = "ARM=HOME";

// Example: ARM=DISARM
static constexpr char kArmDisarm[] = "ARM=DISARM";

// Example: ARM=PANIC
static constexpr char kArmPanic[] = "ARM=PANIC";

// Example: ARM=ALARM
static constexpr char kArmAlarm[] = "ARM=ALARM";

// Example: PHONE=01,+94712345678
static constexpr char kPhoneSet[] = "PHONE=";

// Example: PHONE.SMS=01,1
static constexpr char kPhoneSms[] = "PHONE.SMS=";

// Example: PHONE.CALL=01,1
static constexpr char kPhoneCall[] = "PHONE.CALL=";

// Example: RF=LEARN_B
static constexpr char kRfLearnB[] = "RF=LEARN_B";

// Example: RF.ID=1234567890
static constexpr char kRfId[] = "RF.ID=";

// Example: INFO=ZONE,03
static constexpr char kInfoPrefix[] = "INFO=";

// Example: CFG=ENTRY_DELAY,10
static constexpr char kCfgEntryDelay[] = "CFG=ENTRY_DELAY,";

// Example: CFG=EXIT_DELAY,15
static constexpr char kCfgExitDelay[] = "CFG=EXIT_DELAY,";

// Example: OUT=2,0
static constexpr char kOutputPrefix[] = "OUT=";

// Example: POWER?
static constexpr char kPowerQuery[] = "POWER?";

// Example: BUZZ=CHIME
static constexpr char kBuzzChime[] = "BUZZ=CHIME";

// Example: SIREN=1
static constexpr char kSirenPrefix[] = "SIREN=";

}  // namespace UniversalCmd
