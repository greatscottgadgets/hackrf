#define W25Q80BV_WRITE_ENABLE 0x06
#define W25Q80BV_CHIP_ERASE   0xC7
#define W25Q80BV_READ_STATUS1 0x05
#define W25Q80BV_PAGE_PROGRAM 0x02

#define W25Q80BV_STATUS_BUSY  0x01

void w25q80bv_setup(void);
void w25q80bv_page_program(void);
