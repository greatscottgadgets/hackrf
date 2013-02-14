#define W25Q80BV_PAGE_LEN     256U
#define W25Q80BV_NUM_PAGES    4096U
#define W25Q80BV_NUM_BYTES    1048576U

#define W25Q80BV_WRITE_ENABLE 0x06
#define W25Q80BV_CHIP_ERASE   0xC7
#define W25Q80BV_READ_STATUS1 0x05
#define W25Q80BV_PAGE_PROGRAM 0x02

#define W25Q80BV_STATUS_BUSY  0x01

void w25q80bv_setup(void);
void w25q80bv_chip_erase(void);
void w25q80bv_program(uint32_t addr, uint32_t len, const uint8_t* data);
