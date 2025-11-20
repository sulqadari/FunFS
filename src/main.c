#include "funfs.h"

uint8_t tempData[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
uint8_t tempAnsr[16];

static void
print_hex(uint8_t* data, uint32_t len)
{
	for (uint32_t i = 0; i < len; ++i) {
		if ((i != 0) && ((i % 16) == 0)) {
			printf("\n");
		}
	
		printf("%02x ", data[i]);
	}
	printf("\n");
}

static uint8_t
test_01(void)
{
	femu_open_flash();

	printf("INode size: %d\n", sizeof(Inode));
	for (uint32_t i = 0; i < FLASH_SIZE_TOTAL; i += sizeof(tempAnsr)) {
		if(femu_read(i, tempAnsr, sizeof(tempAnsr)) != fmr_Ok) {
			fprintf(stderr, "EFFOR: failed to read from index %d\n", i);
			return 1;
		}
		print_hex(tempAnsr, sizeof(tempAnsr));
	}

	if (femu_read(FLASH_SIZE_TOTAL - 3, tempAnsr, sizeof(tempAnsr)) == fmr_Ok) {
		fprintf(stderr, "ERROR: failure expected because of boundary violation.\n");
		return 1;
	}

	for (uint32_t i = 0; i < FLASH_SIZE_TOTAL; i += sizeof(tempData)) {
		if(femu_write(i, tempData, sizeof(tempData)) != fmr_Ok) {
			fprintf(stderr, "EFFOR: failed to write at index %d\n", i);
			return 1;
		}
	}

	if (femu_write(FLASH_SIZE_TOTAL - 3, tempData, sizeof(tempData)) == fmr_Ok) {
		fprintf(stderr, "EFFOR: Error expected.\n");
		return 1;
	}

	femu_close_flash();
	return 0;
}

static uint8_t cmd_create_mf[] = {
	(uint8_t)0x62,(uint8_t)0x2D,
	(uint8_t)0x83,(uint8_t)0x02,(uint8_t)0x3F,(uint8_t)0x00,
	(uint8_t)0x82,(uint8_t)0x01,(uint8_t)0x38,
	(uint8_t)0x8A,(uint8_t)0x01,(uint8_t)0x01,
	(uint8_t)0x8D,(uint8_t)0x02,(uint8_t)0x40,(uint8_t)0x03,
	(uint8_t)0x8C,(uint8_t)0x07,(uint8_t)0x6F,(uint8_t)0xFF,(uint8_t)0xFF,(uint8_t)0xFF,(uint8_t)0xFF,(uint8_t)0xFF,(uint8_t)0xFF,
	(uint8_t)0xAB,(uint8_t)0x14,
	(uint8_t)0x84,(uint8_t)0x01,(uint8_t)0xDA,
	(uint8_t)0x97,(uint8_t)0x00,
	(uint8_t)0x84,(uint8_t)0x01,(uint8_t)0x26,
	(uint8_t)0x97,(uint8_t)0x00,
	(uint8_t)0x84,(uint8_t)0x01,(uint8_t)0x28,
	(uint8_t)0x97,(uint8_t)0x00,
	(uint8_t)0x84,(uint8_t)0x01,(uint8_t)0x24,
	(uint8_t)0x97,(uint8_t)0x00
};

/* FIXME
00000000  10 00 00 00 c0 90 55 56  be ba fe ca 01 00 00 00  |......UV........|
00000010  60 00 00 00 d8 90 55 56  be ba fe ca 00 00 00 00  |`.....UV........|
00000020  60 00 00 00 d8 90 55 56  00 d2 d8 c8 20 00 00 00  |`.....UV.... ...|
00000030  ac 8f 55 56 28 c2 ff ff  5b 6e 55 56 dc c1 ff ff  |..UV(...[nUV....|
00000040  40 90 55 56 2f 00 00 00  fc 6d 55 56 bc 70 55 56  |@.UV/....mUV.pUV|
00000050  c9 70 55 56 ac 8f 55 56  ff ff ff ff ff ff ff ff  |.pUV..UV........|
00000060  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
00000070  00 01 00 00 d0 90 55 56  00 00 00 3f 00 00 00 3f  |......UV...?...?|
00000080  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|

*/
static uint8_t
test_02(void)
{
	ffs_initialize();
	ffs_create_file(cmd_create_mf, sizeof(cmd_create_mf));
	ffs_create_file(cmd_create_mf, sizeof(cmd_create_mf));
	femu_close_flash();
	return 0;
}

int
main(int argc, char* argv[])
{
	return test_02();
}