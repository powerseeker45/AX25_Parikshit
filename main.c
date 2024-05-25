#include <stdio.h>
#include "ax25.h"

int main()
{
    uint8_t payload[]="hello from parikshit";
    size_t plen=sizeof(payload);
    uint8_t frame[AX25_MAX_FRAME_LEN]={0};

    uint32_t len= ax25_encode(frame,payload,plen,AX25_UI_FRAME);

    if (len==0)
        printf("error");
    for(int i=0;i<len;i++)
    {
        printf("\n %x : %c : %d",frame[i],frame[i],frame[i]);
    }
    
    
    return 0;

}