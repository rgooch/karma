#include <karma_imc.h>
# 1 "" 
flag imc_24to8 (image_size, image_reds, image_greens, image_blues, stride24,
		out_image, stride8, max_colours, speed, pack_desc, packet)
unsigned int image_size;
unsigned char *image_reds;
unsigned char *image_greens;
unsigned char *image_blues;
int stride24;
unsigned char *out_image;
int stride8;
unsigned int max_colours;
unsigned int speed;
packet_desc **pack_desc;
char **packet;
{
    return ( (flag) 0 );
}
