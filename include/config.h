#ifndef CONFIG_H  /* CONFIG_H */
#define CONFIG_H

#include <stdint.h>

/**
 * dest and sender address related details
 * FUTURE_PARIKSHIT_PROBLEMS : 1. callsign change
 *                         2. ssid is set to 0 need for change?
*/

static const char SAT_CALLSIGN[] = "PARSAT";
static const uint8_t SAT_SSID = 0;

static const char GRD_CALLSIGN[] = "ABCD";
static const uint8_t GRD_SSID = 0;




#endif /* CONFIG_H */