#include <stdio.h>
#include "ax25.h"

int main()
{
    uint8_t payload[] = "hello from parikshit";
    size_t plen = sizeof(payload);
    int no_of_frames = 1;

    if (plen > AX25_MAX_FRAME_LEN)
    {
        no_of_frames = plen / AX25_MAX_FRAME_LEN + 1;
    }

    uint8_t **fin = (uint8_t **)malloc(AX25_MAX_FRAME_LEN * no_of_frames);
    uint8_t **fout = (uint8_t **)malloc(AX25_MAX_FRAME_LEN * no_of_frames);

    /*
    uint8_t frame[AX25_MAX_FRAME_LEN]={0};
    uint8_t outframe[AX25_MAX_FRAME_LEN]={0};
*/

    for (int i = 0; i < no_of_frames; i++)
    {
        uint32_t len = ax25_encode(fin[i], payload, plen, AX25_UI_FRAME);

        printf("\n encoded frame ");
        if (len == 0)
            printf("error");
        for (int j = 0; j < len; j++)
        {
            printf("\n %x : %c : %d", *((fin + i) + j), *((fin + i) + j), *((fin + i) + j));
        }
    }

    for (int i = 0; i < no_of_frames; i++)
    {
        uint32_t outlen = ax25_recv(fout[i], fin[i], sizeof(fin[i]));

        printf("\n decoded frame ");
        if (outlen == 0)
            printf("error");
        for (int j = 0; j < outlen; j++)
        {
            printf("\n %x : %c : %d", *((fin + i) + j), *((fin + i) + j), *((fin + i) + j));
        }
    }

    return 0;
}