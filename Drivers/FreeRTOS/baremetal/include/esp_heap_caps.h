/* Minimal stub: the kernel-addition prototypes only need the type names. */
#ifndef BAREMETAL_ESP_HEAP_CAPS_H
#define BAREMETAL_ESP_HEAP_CAPS_H

#include <stddef.h>
#include <stdint.h>

#define MALLOC_CAP_EXEC      (1 << 0)
#define MALLOC_CAP_32BIT     (1 << 1)
#define MALLOC_CAP_8BIT      (1 << 2)
#define MALLOC_CAP_DMA       (1 << 3)
#define MALLOC_CAP_SPIRAM    (1 << 10)
#define MALLOC_CAP_INTERNAL  (1 << 11)
#define MALLOC_CAP_DEFAULT   (1 << 12)

void *heap_caps_malloc(size_t size, uint32_t caps);
void *heap_caps_calloc(size_t n, size_t size, uint32_t caps);
void *heap_caps_realloc(void *ptr, size_t size, uint32_t caps);
void heap_caps_free(void *ptr);
size_t heap_caps_get_free_size(uint32_t caps);

#endif /* BAREMETAL_ESP_HEAP_CAPS_H */
