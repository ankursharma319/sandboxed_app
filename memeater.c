#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    puts("Hello world\n");
    size_t n_blocks = 8;
    size_t block_size = 1024*1024*8;
    printf("Hello world, I will eat memory in %zu blocks of size %zu\n", n_blocks, block_size);
    printf("Press any key to continue\n");
    getchar();
    uint8_t* buf[n_blocks];
    for (size_t i=0; i<n_blocks; i++) {
        buf[i] = (uint8_t*) calloc(block_size, 1);
        printf("allocated block# i=%zu, filling it with rand data now\n", i);
        for (size_t j=0; j<block_size; j++) {
            buf[i][j] = rand() * UINT8_MAX;
        }
    }
    printf("Press any key to continue to the end\n");
    getchar();
    for (size_t i=0; i<n_blocks; i++) {
        uint8_t sum = 0;
        for (size_t j=0; j<block_size; j++) {
            sum += buf[i][j];
        }
        printf("sum at index i=%zu is %hhu\n", i, sum);
        free(buf[i]);
    }
    puts("bye world\n");
    return 0;
}

