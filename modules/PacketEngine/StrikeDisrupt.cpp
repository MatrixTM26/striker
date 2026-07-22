#include "StrikeDisrupt.hpp"
#include "PacketEngine.hpp"
#include "StrikeMethod.hpp"
#include "RadioInterface.hpp"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

static const char* Tag = "StrikeDisrupt";
static uint8_t ActiveMethod = static_cast<uint8_t>(-1);

namespace StrikeDisrupt {
    void Start(EngineConfig* Cfg) {
        ActiveMethod = Cfg->Method;
        switch (ActiveMethod) {
            case 0: StrikeMethod::LaunchRogueAp(Cfg->Target); break;
            case 1: StrikeMethod::LaunchBroadcast(Cfg->Target, 1); break;
            case 2:
                StrikeMethod::LaunchRogueAp(Cfg->Target);
                StrikeMethod::LaunchBroadcast(Cfg->Target, 1);
                break;
            default: break;
        }
    }

    void Stop() {
        switch (ActiveMethod) {
            case 0:
                RadioInterface::StartManagementAp();
                RadioInterface::RestoreApMac();
                break;
            case 1: StrikeMethod::HaltBroadcast(); break;
            case 2:
                StrikeMethod::HaltBroadcast();
                RadioInterface::StartManagementAp();
                RadioInterface::RestoreApMac();
                break;
            default: break;
        }
    }
}
