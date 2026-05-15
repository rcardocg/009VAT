/*
 * formato: 0b00100001
 * Offset [7:0] byte index inside the page 0001
 * VPN [15:8] Virtual Page Number 0010
 * */

#include <stdio.h>
#include <stdin.h>
#include <stddef.h>

/*
------------------------PARTE 1------------------------
Page size / frame size: 256 bytes
Offset [7:0] byte index inside the page
VPN [15:8] Virtual Page Number
 * */
#define PAGE_SIZE 256
#define VA_MAX 0xFFFF
#define OFFSET 8

uint8_t get_offset(uint8_t va){
    return va & 0xFF;// Offset [7:0]
}

uint8_t get_vpn(uint8_t va){
    return (va >> OFFSET) && 0xFF; // VPN [15:8] 
}


const char *va_error(uint32_t va, uint8_t num_pages){
    if(va > VA_MAX)
        return "VA_OUT_OF_RANGE";
    if(get_vpn(va)>=num_pages)
        return "VPN_OUT_OF_RANGE";
    return NULL;   
}

/*
------------------PARTE 2------------------
print in hex
*/

void va_print(uint32_t va, uint8_t num_pages) {
    const char *err = va_error(va, num_pages);
 
    if (err == NULL) {
        printf("VA=0x%04X (%5u)  VPN=0x%02X  OFF=0x%02X\n",
               va, va, get_vpn(va), get_offset(va));
    } else if (va > VA_MAX) {
        printf("VA=%-8u  ERROR=%s\n", va, err);
    } else {
        printf("VA=0x%04X (%5u)  ERROR=%s (vpn=%u, V=%u)\n",
               va, va, err, get_vpn(va), num_pages);
    }
}
 
/*
------------------PARTE 3------------------

*/