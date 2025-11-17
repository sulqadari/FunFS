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

int
main(int argc, char* argv[])
{
	mmg_init();
	printf("INode size: %d\n", sizeof(Inode));
	for (uint32_t i = 0; i < FLASH_SIZE_TOTAL; i += sizeof(tempAnsr)) {
		if(mmg_read(i, tempAnsr, sizeof(tempAnsr)) != fmr_Ok) {
			fprintf(stderr, "EFFOR: failed to read from index %d\n", i);
			return 1;
		}
		print_hex(tempAnsr, sizeof(tempAnsr));
	}

	if (mmg_read(FLASH_SIZE_TOTAL - 3, tempAnsr, sizeof(tempAnsr)) == fmr_Ok) {
		fprintf(stderr, "ERROR: failure expected because of boundary violation.\n");
		return 1;
	}

	for (uint32_t i = 0; i < FLASH_SIZE_TOTAL; i += sizeof(tempData)) {
		if(mmg_write(i, tempData, sizeof(tempData)) != fmr_Ok) {
			fprintf(stderr, "EFFOR: failed to write at index %d\n", i);
			return 1;
		}
	}

	if (mmg_write(FLASH_SIZE_TOTAL - 3, tempData, sizeof(tempData)) == fmr_Ok) {
		fprintf(stderr, "EFFOR: Error expected.\n");
		return 1;
	}

	close_flash();
	return 0;
}