#include "funfs.h"
#include <string.h>

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

static char*
remove_spaces(const char* str)
{
	// count spaces
	uint32_t spaces = 0;

	if (str == NULL)
		return NULL;
	
	char* result = (char*)str;
	while (*result) {
		
		char c = *result;
		if (c == ' ' || c == '\t'){
			spaces++;
		}
		result++;
	}

	// allocate new array. Its length should be shorter for number of spaces
	size_t len = strlen(str);
	result = malloc(len - spaces);
	if (result == NULL)
		return result;
	
	// copy symbols into new array
	for (uint32_t from = 0, to = 0; from < len; ++from) {
		if ((str[from] != ' ') && (str[from] != '\t')) {
			result[to] = str[from];
			to++;
		} else {
			continue;
		}
	}

	return result;
}

static uint8_t
isHex(char c)
{
	if ((c >= '0') && (c <= '9')) {
		return (uint8_t)(c - '0');
	} else if ((c >= 'A') && (c <= 'F')) {
		return (uint8_t)(c - 'A');
	} else if ((c >= 'a') && (c <= 'f')) {
		return (uint8_t)(c - 'a');
	} else {
		return 0xff;
	}
}

static uint8_t
to_byte(uint8_t hn, uint8_t ln)
{
	return ((hn << 4) | (ln & 0x0F));
}

static uint8_t*
hex_to_bytes(const char* str)
{
	uint8_t hasError = 0;
	uint8_t* bytes = NULL;
	char* truncated = NULL;

	do {
		truncated = remove_spaces(str);
		if (truncated == NULL)
			break;

		size_t len = strlen(truncated);
		if ((len % 2) != 0) {
			hasError = 1;
			break;
		}
		
		bytes = malloc(len / 2);
		if (bytes == NULL) {
			hasError = 1;
			break;
		}
		
		uint8_t i = 0;
		uint8_t hn;
		uint8_t ln;
		char* ptr = truncated;
		do {
			hn = isHex(*ptr++);
			ln = isHex(*ptr++);
	
			if ((hn == 0xff) || (ln == 0xff)) {
				hasError = 1;
				break;
			} else {
				hn = to_byte(hn, ln);
				bytes[i++] = hn;
			}
			
		} while (*ptr);

	} while (0);

	free(truncated);
	if (hasError) {
		free(bytes);
		bytes = NULL;
	}
	
	return bytes;
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

static uint8_t
test_02(void)
{
	ffs_initialize();
	
	for (uint32_t i = 0; i < 10; ++i) {
		ffs_create_file(cmd_create_mf, sizeof(cmd_create_mf));
	}

	femu_close_flash();
	return 0;
}

static uint8_t
test_03(void)
{
	uint8_t* cmd_array[5];

	cmd_array[0] = hex_to_bytes("00112233445566778899");
	cmd_array[1]  = hex_to_bytes("AAFF");
	cmd_array[2]  = hex_to_bytes("aaff");
	cmd_array[3]  = hex_to_bytes("A A  B B");
	cmd_array[4]  = hex_to_bytes("a	a		b		b");

	for (uint8_t i = 0; i < 3; ++i) {
		free(cmd_array[i]);
	}

	return 0;
}

int
main(int argc, char* argv[])
{
	return test_03();
}