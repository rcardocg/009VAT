/*
 * formato: 0b00100001
 * Offset [7:0] byte index inside the page 0001
 * VPN [15:8] Virtual Page Number 0010
 */

#include <stdio.h>
#include <stdlib.h>  // atoi, srand, rand, exit
#include <stdint.h>  // uint8_t, uint32_t
#include <stddef.h>  // NULL
#include <time.h>    // time

#define PAGE_SIZE   256      // cada página/frame mide 256 bytes
#define VA_MAX      0xFFFF   // dirección virtual máxima (16 bits)
#define OFFSET_BITS 8        // bits que ocupa el offset
#define NUM_FRAMES  100      // total de frames en la RAM física simulada

// Global: ram[] la usan ram_init, ram_print y allocate_frame
int ram[NUM_FRAMES];

/*
-------------------------------------------
------------------PARTE 1------------------
-------------------------------------------
Page size / frame size: 256 bytes
Offset [7:0] byte index inside the page
VPN [15:8] Virtual Page Number
*/

// Extrae los 8 bits bajos → byte dentro de la página
// Ejemplo: 0x03AB & 0xFF = 0xAB
uint8_t get_offset(uint32_t va) {
    return va & 0xFF;
}

// Extrae los 8 bits altos → número de página virtual
// Ejemplo: 0x03AB >> 8 = 0x03
// uint32_t porque necesitamos detectar valores > 0xFFFF antes de truncar
uint8_t get_vpn(uint32_t va) {
    return (va >> OFFSET_BITS) & 0xFF;  // & no && (& es bitwise, && es lógico)
}

// Valida la dirección. Retorna NULL si es válida, o el nombre del error si no.
const char *va_error(uint32_t va, uint8_t num_pages) {
    if (va > VA_MAX)
        return "VA_OUT_OF_RANGE";
    if (get_vpn(va) >= num_pages)
        return "VPN_OUT_OF_RANGE";
    return NULL;
}

/*
-------------------------------------------
------------------PARTE 2------------------
-------------------------------------------
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
-------------------------------------------
------------------PARTE 3------------------
-------------------------------------------
Fisher-Yates Shuffling:
1. iniciar en el final del array
2. generar indice random
3. swap i con j
4. disminuir tamaño
*/

void ram_init(uint8_t num_pages, unsigned int seed) {
    srand(seed);

    int ocupados = 10 + rand() % 51; // [10, 60]

    // dejar suficientes libres para el proceso
    int max_ocupados = NUM_FRAMES - num_pages;
    if (ocupados > max_ocupados)
        ocupados = max_ocupados;

    // indices[] es local: solo se necesita aquí para el shuffle
    int indices[NUM_FRAMES];
    for (int i = 0; i < NUM_FRAMES; i++)
        indices[i] = i;

    // Fisher-Yates: mezcla los índices sin repetición
    for (int i = NUM_FRAMES - 1; i > 0; i--) {
        int j = rand() % (i + 1); // j es un índice aleatorio entre 0 e i
        int tmp = indices[i];     // intercambiamos indices[i] con indices[j]
        indices[i] = indices[j];
        indices[j] = tmp;
    }

    // inicializar todos libres, luego marcar los primeros 'ocupados' del shuffle
    for (int i = 0; i < NUM_FRAMES; i++)
        ram[i] = 0;
    for (int i = 0; i < ocupados; i++)
        ram[indices[i]] = 1;
}

// Imprime el mapa de la RAM: F=libre, X=ocupado, 10 frames por fila
void ram_print(unsigned int seed) {
    int libres = 0, ocupados = 0;
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (ram[i] == 0) libres++;
        else             ocupados++;
    }

    printf("PHYSICAL RAM (%d frames) after random init (seed=%u):\n", NUM_FRAMES, seed);
    printf("FREE=%d OCCUPIED=%d\n", libres, ocupados);

    for (int i = 0; i < NUM_FRAMES; i++) {
        printf("%2d:%c ", i, ram[i] == 0 ? 'F' : 'X');
        if ((i + 1) % 10 == 0) printf("\n");
    }
    printf("\n");
}

// Busca el primer frame libre, lo marca ocupado y retorna su índice.
// Retorna -1 si no hay frames libres.
int allocate_frame(void) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (ram[i] == 0) {
            ram[i] = 1;
            return i;
        }
    }
    return -1;
}

// Page table: un arreglo de hasta 256 entradas (una por página virtual)
// Cada entrada sabe si es válida y a qué frame físico apunta
typedef struct {
    int valid; // 1 = esta página tiene un frame asignado, 0 = no
    int pfn;   // Physical Frame Number (solo válido si valid=1)
} PTE;

PTE page_table[256]; // global: la usan process_load y translate

// Asigna un frame físico a cada página virtual 0..num_pages-1
void process_load(uint8_t num_pages) {
    for (int i = 0; i < num_pages; i++) {
        page_table[i].valid = 0;
        page_table[i].pfn   = -1;
    }

    printf("Load process: V=%u -> ", num_pages);

    for (int i = 0; i < num_pages; i++) {
        int frame = allocate_frame();

        if (frame == -1) {
            printf("\nERROR: sin frames libres al cargar pagina %d\n", i);
            exit(1);
        }

        page_table[i].valid = 1;
        page_table[i].pfn   = frame;
        printf("VPN%d->PFN%d ", i, frame);
    }
    printf("\n\n");
}

/*
-------------------------------------------
------------------PARTE 4------------------
-------------------------------------------
*/

void translate(uint32_t va, uint8_t num_pages) {
    const char *err = va_error(va, num_pages);
    if (err != NULL) {
        // si hay error de rango se delega a va_print() de la parte 2
        va_print(va, num_pages);
        return;
    }

    uint8_t vpn    = get_vpn(va);    // parte 1
    uint8_t offset = get_offset(va); // parte 1

    if (!page_table[vpn].valid) {
        printf("VA=0x%04X (%5u)  VPN=0x%02X  OFF=0x%02X  ERROR=PAGE_NOT_MAPPED\n",
               va, va, vpn, offset);
        return;
    }

    // PA = PFN * PAGE_SIZE + offset
    int pfn = page_table[vpn].pfn;
    int pa  = pfn * PAGE_SIZE + offset;

    printf("VA=0x%04X (%5u)  VPN=0x%02X  OFF=0x%02X  PFN=%2d  PA=%5d\n",
           va, va, vpn, offset, pfn, pa);
}

// Lee el archivo línea por línea y traduce cada dirección
void translate_batch(const char *filename, uint8_t num_pages) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        printf("ERROR: no se pudo abrir el archivo %s\n", filename);
        exit(1);
    }

    printf("=== Task 4 - Traduccion de direcciones ===\n\n");

    uint32_t va;
    while (fscanf(f, "%u", &va) == 1)
        translate(va, num_pages);

    fclose(f);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <num_virtual_pages> <archivo_direcciones> [seed]\n", argv[0]);
        return 1;
    }

    uint8_t      num_pages = (uint8_t)atoi(argv[1]);
    const char  *filename  = argv[2];
    unsigned int seed      = (argc >= 4) ? (unsigned int)atoi(argv[3]) : (unsigned int)time(NULL);

    ram_init(num_pages, seed);
    ram_print(seed);
    process_load(num_pages);
    translate_batch(filename, num_pages);

    return 0;
}