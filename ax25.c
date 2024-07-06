#include "ax25.h"
#include <stdio.h>
const uint8_t AX25_SYNC_FLAG_MAP_BIN[8] = {0, 1, 1, 1, 1, 1, 1, 0};

/*FUTURE_SHASH_PROBLEMS: add error codes for errors that can occur*/

/**
 * Creates the address field of the AX.25 frame
 * @param out the output buffer with enough memory to hold the address field
 * @param dest_addr the destination callsign address
 * @param dest_ssid the destination SSID
 * @param src_addr the callsign of the source
 * @param src_ssid the source SSID
 */

/**
 * DOC: for ground station put grd address first then sat 
 */
ax25_encode_status_t ax25_create_addr_field(uint8_t *out)
{

    uint16_t i = 0;

    if (out==NULL)
        return AX25_ENC_ADDR_FAIL;

    for (i = 0; i < strnlen(SAT_CALLSIGN, AX25_MAX_ADDR_LEN); i++)
    {
        *out++ = SAT_CALLSIGN[i] << 1;
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
    *out++ = ((0x0F & SAT_SSID) << 1) | 0x60;
    
    for (i = 0; i < strnlen(GRD_CALLSIGN, AX25_MAX_ADDR_LEN); i++)
    {
        *out++ = GRD_CALLSIGN[i] << 1;
    }
    for (; i < AX25_CALLSIGN_MAX_LEN; i++)
    {
        *out++ = ' ' << 1;
    }
    /* Apply SSID, reserved and C bit. As this is the last address field
     * the trailing bit is set to 1. as last bit of address.
     */
    /* FIXME: C bit is set to 0 implicitly */
    *out++ = ((0x0F & GRD_SSID) << 1) | 0x61;


    return AX25_ENC_OK;
}

/**
 * Calculates the FCS of the AX25 frame
 * @param buffer data buffer
 * @param len size of the buffer
 * @return the FCS of the buffer
 */
ax25_encode_status_t ax25_fcs(uint8_t *buffer, size_t len, uint16_t fcs)
{
    fcs = 0xFFFF;
    if (len<=0||len>AX25_MAX_FRAME_LEN)
    {
        return AX25_ENC_FAIL;
    }
    while (len--)
    {
        fcs = (fcs >> 8) ^ crc16_ccitt_table_reverse[(fcs ^ *buffer++) & 0xFF];
    }
    fcs=fcs ^ 0xFFFF;
    return AX25_ENC_OK;

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
size_t ax25_create_frame(uint8_t *out, const uint8_t *info, size_t info_len, uint16_t ctrl, size_t ctrl_len)
{   
    // returns if info length passed is greater than allowed frame size
    if (info_len > AX25_MAX_FRAME_LEN)
    {
        return 0;
    }

    uint16_t i = 0; // index for out pointer
    ax25_encode_status_t status=AX25_ENC_OK;
    uint16_t fcs=0xFFFF;

    /* adding initial flag*/
    out[i++] = AX25_FLAG;


    /* adding address*/
    
    status=ax25_create_addr_field((out+i));
    i++;

    if (status!=AX25_ENC_OK)
        return status;
    

    /* adding control field */
    if (ctrl_len == AX25_MIN_CTRL_LEN )
    {
        out[i++] = (uint8_t)(ctrl & 0xFF);
    }
    else if(ctrl_len == AX25_MAX_CTRL_LEN)
    {

        out[i++] = (uint8_t)(ctrl & 0xFF);        //lower byte
        out[i++] = (uint8_t)((ctrl >> 8) & 0xFF);   //upper byte

    }
    else
    {
        return AX25_ENC_CTRL_FAIL;
    }

    /* adding PID field. As there is no layer 3 being used this set to 0xF0 */
    out[i++] = 0xF0;

    /* adding info into the out buffer */
    for(int j=0;j<info_len;j++)
    {
        out[i++]=info[j];
    }

    /* Compute the FCS. Ignore the first flag byte */
    status = ax25_fcs(out + 1, i - 1,fcs);
    /* The MS bits are sent first ONLY at the FCS field */
    out[i++] = (fcs >> 8) & 0xFF;
    out[i++] = fcs & 0xFF;

    /* final flag */
    out[i++] = AX25_FLAG;

    

    return i;
}

ax25_encode_status_t ax25_bit_stuffing(uint8_t *out, size_t *out_len, const uint8_t *buffer, const size_t buffer_len)
{
    uint8_t bit;
    uint8_t prev_bit = 0;
    size_t out_idx = 0;
    size_t cont_1 = 0;
    size_t total_cont_1 = 0;
    size_t i;

    /* Leading FLAG field does not need bit stuffing */
    for (int j=0;j<8*sizeof(uint8_t);j++)
    {
        out[out_idx++]=AX25_SYNC_FLAG_MAP_BIN[j];
    }
    /*
    memcpy(out, AX25_SYNC_FLAG_MAP_BIN, 8 * sizeof(uint8_t));
    out_idx = 8;
    */
    /* Skip the leading and trailing FLAG field */
    buffer++;
    for (i = 0; i < 8 * (buffer_len - 2); i++)
    {
        bit = (buffer[i / 8] >> (i % 8)) & 0x1;
        out[out_idx++] = bit;

        /* Check if bit stuffing should be applied */
        if (bit & prev_bit)
        {
            cont_1++;
            total_cont_1++;
            if (cont_1 == 4)
            {
                out[out_idx++] = 0;
                cont_1 = 0;
            }
        }
        else
        {
            cont_1 = total_cont_1 = 0;
        }
        prev_bit = bit;

        /*
         * If the total number of continuous 1's is 15 the the frame should be
         * dropped
         */
        if (total_cont_1 >= 14)
        {
            return AX25_ENC_FAIL;
        }
    }

    /* Trailing FLAG field does not need bit stuffing */
    for (int j=0;j<8*sizeof(uint8_t);j++)
    {
        out[out_idx++]=AX25_SYNC_FLAG_MAP_BIN[j];
    }
    /*
    memcpy(out + out_idx, AX25_SYNC_FLAG_MAP_BIN, 8 * sizeof(uint8_t));
    out_idx += 8;*/

    *out_len = out_idx;
    return AX25_ENC_OK;
}

/**
 * the main function to be called to create ax25 frames
 * @param out is the buffer to hold multiple ax.25 frames
 * @param in data to be encoded
 * @param len length of data to be encoded
 * @param type ax25 frame type (I,S,U,UI frame)
 */
int32_t ax25_encode(uint8_t *out, const uint8_t *in, size_t inlen, ax25_frame_type_t type)
{
/**
 * Future_parikshit_problems : the out buffer is 1d or 2d 
*/
    uint8_t *addr = (uint8_t *)malloc(sizeof(uint8_t) * AX25_MAX_ADDR_LEN);
    size_t addrlen = ax25_create_addr_field(addr, GRD_CALLSIGN, GRD_SSID, SAT_CALLSIGN, SAT_SSID);

    uint8_t ctrl;
    size_t ctrllen;

    ax25_encode_status_t status;

    uint8_t interm_buffer[AX25_MAX_FRAME_LEN] = {0};
    uint8_t tmp_send_buf[AX25_MAX_FRAME_LEN * 8 + AX25_MAX_FRAME_LEN] = {0};
    uint32_t framelen = 0;
    size_t temp_len;
    size_t pad_bits = 0;

    /* FUTURE_SHASH_PROBLEMS : 1. create a control field function , 2. add ctrl for other frames */
    if (type == AX25_UI_FRAME)
    {
        ctrl = AX25_CTRL_UI;
        ctrllen = AX25_MIN_CTRL_LEN;
    }

    /*immidiate_shash_update: add as to make multiple, use interm buffer */
    framelen = ax25_create_frame(interm_buffer, in, inlen, type, addr, addrlen, ctrl, ctrllen);
/**
 * TESTING */
    for (int i = 0; i < framelen; i++)
    {
        printf("\n %x : %c : %d", interm_buffer[i], interm_buffer[i], interm_buffer[i]);
    }

    status = ax25_bit_stuffing(tmp_send_buf, &temp_len, interm_buffer, framelen);
    if (status != AX25_ENC_OK)
    {
        return -1;
    }

    memset(out, 0, temp_len / 8 * sizeof(uint8_t));
    /* Pack now the bits into full bytes */
    for (int i = 0; i < temp_len; i++)
    {
        out[i / 8] |= tmp_send_buf[i] << (7 - (i % 8));
    }
    pad_bits = 8 - (temp_len % 8);
    temp_len += pad_bits;
    out[temp_len / 8] &= (0xFF << pad_bits);

    return temp_len / 8;
}

ax25_decode_status_t ax25_decode(uint8_t *out, size_t *out_len, const uint8_t *ax25_frame, size_t len)
{
    size_t i;
    size_t frame_start = UINT_MAX;
    size_t frame_stop = UINT_MAX;
    uint8_t res;
    size_t cont_1 = 0;
    size_t received_bytes = 0;
    size_t bit_cnt = 0;
    uint8_t decoded_byte = 0x0;
    uint16_t fcs;
    uint16_t recv_fcs;

    /* Start searching for the SYNC flag */
    for (i = 0; i < len - sizeof(AX25_SYNC_FLAG_MAP_BIN); i++)
    {
        res = (AX25_SYNC_FLAG_MAP_BIN[0] ^ ax25_frame[i]) | (AX25_SYNC_FLAG_MAP_BIN[1] ^ ax25_frame[i + 1]) | (AX25_SYNC_FLAG_MAP_BIN[2] ^ ax25_frame[i + 2]) | (AX25_SYNC_FLAG_MAP_BIN[3] ^ ax25_frame[i + 3]) | (AX25_SYNC_FLAG_MAP_BIN[4] ^ ax25_frame[i + 4]) | (AX25_SYNC_FLAG_MAP_BIN[5] ^ ax25_frame[i + 5]) | (AX25_SYNC_FLAG_MAP_BIN[6] ^ ax25_frame[i + 6]) | (AX25_SYNC_FLAG_MAP_BIN[7] ^ ax25_frame[i + 7]);
        /* Found it! */
        if (res == 0)
        {
            printf("AX Start found at %d\n", i);
            frame_start = i;
            break;
        }
    }

    /* We failed to find the SYNC flag */
    if (frame_start == UINT_MAX)
    {
        // std::cout << "Frame start was not found" << std::endl;
        printf("AX Frame start was not found\n");
        return AX25_DEC_FAIL;
    }

    for (i = frame_start + sizeof(AX25_SYNC_FLAG_MAP_BIN);
         i < len - sizeof(AX25_SYNC_FLAG_MAP_BIN) + 1; i++)
    {
        /* Check if we reached the frame end */
        res = (AX25_SYNC_FLAG_MAP_BIN[0] ^ ax25_frame[i]) | (AX25_SYNC_FLAG_MAP_BIN[1] ^ ax25_frame[i + 1]) | (AX25_SYNC_FLAG_MAP_BIN[2] ^ ax25_frame[i + 2]) | (AX25_SYNC_FLAG_MAP_BIN[3] ^ ax25_frame[i + 3]) | (AX25_SYNC_FLAG_MAP_BIN[4] ^ ax25_frame[i + 4]) | (AX25_SYNC_FLAG_MAP_BIN[5] ^ ax25_frame[i + 5]) | (AX25_SYNC_FLAG_MAP_BIN[6] ^ ax25_frame[i + 6]) | (AX25_SYNC_FLAG_MAP_BIN[7] ^ ax25_frame[i + 7]);
        /* Found it! */
        if (res == 0)
        {
            printf("AX Stop found at %d\n", i);
            frame_stop = i;
            break;
        }

        if (ax25_frame[i])
        {
            cont_1++;
            decoded_byte |= 1 << bit_cnt;
            bit_cnt++;
        }
        else
        {
            /* If 5 consecutive 1's drop the extra zero*/
            if (cont_1 >= 5)
            {
                cont_1 = 0;
            }
            else
            {
                bit_cnt++;
                cont_1 = 0;
            }
        }

        /* Fill the fully constructed byte */
        if (bit_cnt == 8)
        {
            out[received_bytes++] = decoded_byte;
            bit_cnt = 0;
            decoded_byte = 0x0;
        }
    }

    if (frame_stop == UINT_MAX || received_bytes < AX25_MIN_ADDR_LEN)
    {
        printf("AX Wrong frame size\n");
        return AX25_DEC_FAIL;
    }

    /* Now check the CRC */
    fcs = ax25_fcs(out, received_bytes - sizeof(uint16_t));
    recv_fcs = (((uint16_t)out[received_bytes - 2]) << 8) | out[received_bytes - 1];

    if (fcs != recv_fcs)
    {
        printf("AX Wrong FCS\n");
        return AX25_DEC_FAIL;
    }

    *out_len = received_bytes - sizeof(uint16_t);
    return AX25_DEC_OK;
}

uint32_t ax25_recv(uint8_t *out, const uint8_t *in, size_t len)
{
    size_t i;
    size_t decode_len;
    ax25_decode_status_t status;
    uint8_t tmp_recv_buf[AX25_MAX_FRAME_LEN * 8 + AX25_MAX_FRAME_LEN] = {0};

    /* Apply one bit per byte for easy decoding */
    for (i = 0; i < len; i++)
    {
        tmp_recv_buf[8 * i] = (in[i] >> 7) & 0x1;
        tmp_recv_buf[8 * i + 1] = (in[i] >> 6) & 0x1;
        tmp_recv_buf[8 * i + 2] = (in[i] >> 5) & 0x1;
        tmp_recv_buf[8 * i + 3] = (in[i] >> 4) & 0x1;
        tmp_recv_buf[8 * i + 4] = (in[i] >> 3) & 0x1;
        tmp_recv_buf[8 * i + 5] = (in[i] >> 2) & 0x1;
        tmp_recv_buf[8 * i + 6] = (in[i] >> 1) & 0x1;
        tmp_recv_buf[8 * i + 7] = in[i] & 0x1;
    }

    /* Perform the actual decoding */
    status = ax25_decode(out, &decode_len, tmp_recv_buf, len * 8);
    if (status != AX25_DEC_OK)
    {
        return -1;
    }
    return (ssize_t)decode_len;
}

