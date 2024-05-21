#include "ax25.h"

/**
 * Creates the address field of the AX.25 frame
 * @param out the output buffer with enough memory to hold the address field
 * @param dest_addr the destination callsign address
 * @param dest_ssid the destination SSID
 * @param src_addr the callsign of the source
 * @param src_ssid the source SSID
 */
size_t ax25_create_addr_field(uint8_t *out, const uint8_t *dest_addr, uint8_t dest_ssid,const uint8_t *src_addr, uint8_t src_ssid)
{
    uint16_t i = 0;

    for (i = 0; i < strnlen(dest_addr, AX25_MAX_ADDR_LEN); i++)
    {
        *out++ = dest_addr[i] << 1;
    }
    /*
     * Perhaps the destination callsign was smaller that the maximum allowed.
     * In this case the leftover bytes should be filled with space
     */
    for (; i < AX25_CALLSIGN_MAX_LEN; i++)
    {
        *out++ = ' ' << 1;
    }
    /* Apply SSID, reserved and C bit */
    /* FIXME: C bit is set to 0 implicitly */
    *out++ = ((0x0F & dest_ssid) << 1) | 0x60;
    //*out++ = ((0b1111 & dest_ssid) << 1) | 0b01100000;

    for (i = 0; i < strnlen(src_addr, AX25_MAX_ADDR_LEN); i++)
    {
        *out++ = dest_addr[i] << 1;
    }
    for (; i < AX25_CALLSIGN_MAX_LEN; i++)
    {
        *out++ = ' ' << 1;
    }
    /* Apply SSID, reserved and C bit. As this is the last address field
     * the trailing bit is set to 1.
     */
    /* FIXME: C bit is set to 0 implicitly */
    *out++ = ((0x0F & dest_ssid) << 1) | 0x61;
    //*out++ = ((0b1111 & dest_ssid) << 1) | 0b01100001;
    return (size_t)AX25_MIN_ADDR_LEN;
}

/**
 * this fn creates ax25 frames
 * @param out holds the created frame
 * @param info holds the info to be made into frame
 * @param info_len length of info passed
 * @param type the type of frame to be created (I,S,U,UI)
 * @param addr address field of the frame
 * @param addr_len length of addr
 * @param ctrl control field
 * @param ctrl_len lenght of ctrl field
 */
size_t ax25_create_frame(uint8_t *out, const uint8_t *info, size_t info_len, ax25_frame_type_t type, uint8_t *addr, size_t addr_len, uint16_t ctrl, size_t ctrl_len)
{
    // returns if info length passed is greater than allowed frame size
    if (info_len > AX25_MAX_FRAME_LEN)
    {
        return 0;
    }

    uint16_t i = 1; // index for out pointer

    /* adding initial flag*/
    out[0] = AX25_FLAG;
    /* adding address*/
    if (addr_len == AX25_MIN_ADDR_LEN || addr_len == AX25_MAX_ADDR_LEN)
    {
        memcpy(out + i, addr, addr_len);
        i += addr_len;
    }
    else
    {
        return 0;
    }

    /* adding control field */
    if (ctrl_len == AX25_MIN_CTRL_LEN || ctrl_len == AX25_MAX_CTRL_LEN)
    {
        memcpy(out + i, &ctrl, ctrl_len);
        i += ctrl_len;
    }
    else
    {
        return 0;
    }

    /* adding PID field. as there is no layer 3 being used this set to 0xF0 */
    
    if (type == AX25_I_FRAME || type == AX25_UI_FRAME)
    {
        out[i++] = 0xF0;
    }

    /* addign info into the out buffer */

    memcpy(out + i, info, info_len);
    i += info_len;

    /* Compute the FCS. Ignore the first flag byte */
    uint16_t fcs = ax25_fcs(out + 1, i - 1);
    /* The MS bits are sent first ONLY at the FCS field */
    out[i++] = (fcs >> 8) & 0xFF;
    out[i++] = fcs & 0xFF;

    /* final flag */
    out[i++] = AX25_FLAG;

    /**/

    return i;
}
/**
 * the main function to be called to create ax25 frames
 * @param out is the buffer to hold multiple ax.25 frames
 * @param in data to be encoded
 * @param len length of data to be encoded
 * @param type ax25 frame type (I,S,U,UI frame)
 */
int32_t ax25_encode(uint8_t *out, const uint8_t *in, size_t inlen,ax25_frame_type_t type)
{

    uint8_t *addr = (uint8_t *)malloc(sizeof(uint8_t) * AX25_MAX_ADDR_LEN);
    size_t addrlen = ax25_create_addr_field(addr,GRD_CALLSIGN,GRD_SSID,SAT_CALLSIGN,SAT_SSID);

    uint8_t ctrl;
    size_t ctrllen;

    /* FUTURE_SHASH_PROBLEMS : 1. create a control field function , 2. add ctrl for other frames */
    if (type==AX25_UI_FRAME)
    {
        ctrl=AX25_CTRL_UI;
        ctrllen=AX25_MIN_CTRL_LEN;
    }

    uint8_t interm_buffer[AX25_MAX_FRAME_LEN]={0};
    uint32_t framelen=0;
    framelen= ax25_create_frame(interm_buffer,in,inlen,type,addr,addrlen,ctrl,ctrllen);

    return framelen;
    

    
}
