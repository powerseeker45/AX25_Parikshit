#ifndef AX25_H /* AX25_H */
#define AX25_H 

//#include <stdint.h> /* included in CONFIG_H */
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>


#include "utils.h"
#include "config.h"

/**
 * AX.25 related lengths
*/
#define AX25_MAX_FRAME_LEN 256

#define AX25_MAX_ADDR_LEN 28
#define AX25_MIN_ADDR_LEN 14

#define AX25_MIN_CTRL_LEN 1
#define AX25_MAX_CTRL_LEN 2
#define AX25_CALLSIGN_MAX_LEN 6
#define AX25_CALLSIGN_MIN_LEN 2

#define AX25_PREAMBLE_LEN 16
#define AX25_POSTAMBLE_LEN 16

/* AX.25 flag */
#define AX25_FLAG 0x7E

/**
 * AX.25 Frame types
 */
typedef enum
{
  AX25_I_FRAME, //!< AX25_I_FRAME Information frame
  AX25_S_FRAME, //!< AX25_S_FRAME Supervisory frame
  AX25_U_FRAME, //!< AX25_U_FRAME Unnumbered frame
  AX25_UI_FRAME /**!< AX25_UI_FRAME Unnumbered information frame */
} ax25_frame_type_t;


/**
 * AX.25 control fields
 * FUTURE_SHASH_PROBLEMS : need to add for other frame types
*/

static const uint8_t AX25_CTRL_UI= 0x03;

typedef enum
{
  AX25_ENC_ADDR_FAIL, AX25_ENC_CTRL_FAIL,AX25_ENC_FCS_FAIL,AX25_ENC_FAIL, AX25_ENC_OK
}ax25_encode_status_t;

/**
 * bit stuffing status

typedef enum
{
  AX25_ENC_FAIL, AX25_ENC_OK
} ax25_encode_status_t;

*/
typedef enum
{
  AX25_DEC_FAIL, AX25_DEC_OK
} ax25_decode_status_t;

/**
 * function definitions
*/

size_t ax25_create_addr_field(uint8_t *out, const uint8_t *dest_addr, uint8_t dest_ssid,const uint8_t *src_addr, uint8_t src_ssid);

uint16_t ax25_fcs(uint8_t *buffer, size_t len);

size_t ax25_create_frame(uint8_t *out, const uint8_t *info, size_t info_len, ax25_frame_type_t type, uint8_t *addr, size_t addr_len, uint16_t ctrl, size_t ctrl_len);

int32_t ax25_encode(uint8_t *out, const uint8_t *in, size_t inlen,ax25_frame_type_t type);


uint32_t ax25_recv(uint8_t *out, const uint8_t *in, size_t len);

ax25_decode_status_t ax25_decode (uint8_t *out, size_t *out_len, const uint8_t *ax25_frame,size_t len);



#endif /* AX25_H */