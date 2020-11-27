#ifndef __PNX_ALLOC_H
#define __PNX_ALLOC_H

typedef struct spi_msg_buff
{
	size_t	size;
	dma_addr_t dma_buffer;
	void* io_buffer;
	void *saved_ll;
	u32 saved_ll_dma;
} spi_pnx_msg_buff_t;
void *pnx_memory_alloc(size_t size, int base);
void pnx_memory_free(const void *data);
void* pnx_get_buffer( struct device *device, void* void_buff );
void pnx_release_buffer( struct device *device, void* buffer );
#endif
