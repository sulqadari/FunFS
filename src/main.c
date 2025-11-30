#include "iso7816.h"
#include "flash_emu.h"
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
	size_t source_len = strlen(str);

	// subtract the number of specrs from initial length and add one for '\0' sign
	size_t len = (source_len - spaces) + 1;
	result = malloc(len);
	if (result == NULL)
		return result;
	
	// copy symbols into new array
	for (uint32_t from = 0, to = 0; from < source_len; ++from) {
		if ((str[from] != ' ') && (str[from] != '\t')) {
			result[to] = str[from];
			to++;
		} else {
			continue;
		}
	}
	result[len - 1] = '\0';
	return result;
}

static uint8_t
isHex(char c)
{
	if ((c >= '0') && (c <= '9')) {
		return (uint8_t)c - 48;
	} else if ((c >= 'A') && (c <= 'F')) {
		return (uint8_t)c - 55;
	} else if ((c >= 'a') && (c <= 'f')) {
		return (uint8_t)c - 87;
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
hex_to_bytes(const char* str, uint16_t* outLen)
{
	uint8_t hasError = 0;
	uint8_t* bytes = NULL;
	char* truncated = NULL;
	*outLen = 0;
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
		*outLen = len / 2;
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
	emu_open_flash();

	printf("INode size: %d\n", sizeof(INode));
	for (uint32_t i = 0; i < FLASH_SIZE_TOTAL; i += sizeof(tempAnsr)) {
		if(emu_read(i, tempAnsr, sizeof(tempAnsr)) != fmr_Ok) {
			fprintf(stderr, "EFFOR: failed to read from index %d\n", i);
			return 1;
		}
		print_hex(tempAnsr, sizeof(tempAnsr));
	}

	if (emu_read(FLASH_SIZE_TOTAL - 3, tempAnsr, sizeof(tempAnsr)) == fmr_Ok) {
		fprintf(stderr, "ERROR: failure expected because of boundary violation.\n");
		return 1;
	}

	for (uint32_t i = 0; i < FLASH_SIZE_TOTAL; i += sizeof(tempData)) {
		if(emu_write(i, tempData, sizeof(tempData)) != fmr_Ok) {
			fprintf(stderr, "EFFOR: failed to write at index %d\n", i);
			return 1;
		}
	}

	if (emu_write(FLASH_SIZE_TOTAL - 3, tempData, sizeof(tempData)) == fmr_Ok) {
		fprintf(stderr, "EFFOR: Error expected.\n");
		return 1;
	}

	emu_close_flash();
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
	iso_initialize();
	
	for (uint32_t i = 0; i < 10; ++i) {
		iso_create_file(cmd_create_mf, sizeof(cmd_create_mf));
	}

	emu_close_flash();
	return 0;
}

static uint8_t
test_03(void)
{
	uint8_t* cmd_array[5];
	uint16_t len = 0;
	// cmd_array[0] = hex_to_bytes("00112233445566778899");
	cmd_array[1]  = hex_to_bytes("AA FF", &len);
	cmd_array[2]  = hex_to_bytes("aaff ", &len);
	cmd_array[3]  = hex_to_bytes("A A  B B", &len);
	cmd_array[4]  = hex_to_bytes("a	a		b		b", &len);

	for (uint8_t i = 0; i < 3; ++i) {
		free(cmd_array[i]);
	}

	return 0;
}

typedef struct {
	uint8_t* cmd;
	uint32_t len;
} cmd_t;

static ISO_SW
create_flat_dir_hierarchy(void)
{
	ISO_SW result = SW_UNKNOWN;
	uint16_t len = 0;
	cmd_t cmds[5];
	uint8_t idx = 0;
	do {
		if (iso_initialize() != SW_OK) {
			printf("ERROR: failed to initialize file system\n");
			break;
		}

		cmds[idx].cmd = hex_to_bytes("622D 8302 3F00 8201 38 8A01 01 8D02 4003 8C07 6FFFFFFFFFFFFF AB14 8401DA9700840126970084012897008401249700", &len);
		cmds[idx++].len = len;
		cmds[idx].cmd = hex_to_bytes("6236 8302 4F00 8201 38 8407 A0000002471001 8D02 4003 8A01 01 8C07 6FFFFFFFFFFFFF AB14 8401DA9700840126970084012897008401249700", &len);
		cmds[idx++].len = len;
		cmds[idx].cmd = hex_to_bytes("6236 8302 5F00 8201 38 8407 A0000002471001 8D02 4003 8A01 01 8C07 6FFFFFFFFFFFFF AB14 8401DA9700840126970084012897008401249700", &len);
		cmds[idx++].len = len;

		for (uint8_t i = 0; i < idx; ++i) {
			if (iso_create_file(cmds[i].cmd, cmds[i].len) != SW_OK) {
				printf("ERROR: failed cmd No %d\n", i);
				break;
			}

			if (iso_select_by_name(0x3F00) != SW_OK) {
				printf("ERROR: failed to select MF\n");
				break;
			}
		}
		
		for (uint8_t i = 0; i < idx; ++i) {
			free(cmds[i].cmd);
			cmds[i].len = 0;
		}

		result = SW_OK;
	} while (0);
	return result;
}

static ISO_SW
create_dir_hierarchy(void)
{
	ISO_SW result = SW_UNKNOWN;
	uint16_t len = 0;
	cmd_t cmds[5];
	uint8_t idx = 0;
	do {
		if (iso_initialize() != SW_OK) {
			printf("ERROR: failed to initialize file system\n");
			break;
		}

		cmds[idx].cmd = hex_to_bytes("622D 8302 3F00 8201 38 8A01 01 8D02 4003 8C07 6FFFFFFFFFFFFF AB14 8401DA9700840126970084012897008401249700", &len);
		cmds[idx++].len = len;
		cmds[idx].cmd = hex_to_bytes("6236 8302 4F00 8201 38 8407 A0000002471001 8D02 4003 8A01 01 8C07 6FFFFFFFFFFFFF AB14 8401DA9700840126970084012897008401249700", &len);
		cmds[idx++].len = len;
		cmds[idx].cmd = hex_to_bytes("6236 8302 5F00 8201 38 8407 A0000002471001 8D02 4003 8A01 01 8C07 6FFFFFFFFFFFFF AB14 8401DA9700840126970084012897008401249700", &len);
		cmds[idx++].len = len;

		for (uint8_t i = 0; i < idx; ++i) {
			if (iso_create_file(cmds[i].cmd, cmds[i].len) != SW_OK) {
				printf("ERROR: failed cmd No %d\n", i);
				break;
			}
		}
		
		for (uint8_t i = 0; i < idx; ++i) {
			free(cmds[i].cmd);
			cmds[i].len = 0;
		}

		result = SW_OK;
	} while (0);
	return result;
}

static emu_Result
test_04(void)
{
	emu_Result result = fmr_Ok;

	result = create_dir_hierarchy();

	emu_close_flash();
	return result;
}

static ISO_SW
test_05(void)
{
	ISO_SW result = SW_UNKNOWN;
	
	do {
		if (create_dir_hierarchy() != SW_OK) {
			break;
		}

		uint16_t len = 0;
		cmd_t cmds[10];
		uint8_t idx = 0;
		
		cmds[idx].cmd = hex_to_bytes("3F00 4F00 5F00", &len);
		cmds[idx++].len = len;

		cmds[idx].cmd = hex_to_bytes("4F00", &len);
		cmds[idx++].len = len;

		cmds[idx].cmd = hex_to_bytes("5F00", &len);
		cmds[idx++].len = len;

		cmds[idx].cmd = hex_to_bytes("3F00", &len);
		cmds[idx++].len = len;

		for (uint8_t i = 0; i < idx; ++i) {
			if (cmds[i].cmd == NULL) {
				printf("ERROR: cmd No %d is NULL\n", i);
				break;
			}
			if (iso_select_by_path(cmds[i].cmd, cmds[i].len)) {
				printf("ERROR: failed cmd No %d\n", i);
				break;
			}
		}

		for (uint8_t i = 0; i < idx; ++i) {
			free(cmds[i].cmd);
			cmds[i].len = 0;
		}
		result = SW_OK;
	} while (0);
	
	
	emu_close_flash();
	return result;
}

static emu_Result
test_06(void)
{
	ISO_SW result = SW_UNKNOWN;
	do {
		
		if (create_flat_dir_hierarchy() != SW_OK) {
			break;
		}
		
		if (iso_select_by_name(0x3F00) != SW_OK) {
			printf("ERROR: failed to select MF\n");
			break;
		}
		if (iso_select_by_name(0x4F00) != SW_OK) {
			printf("ERROR: failed to select 4F00\n");
			break;
		}
		if (iso_select_by_name(0x5F00) == SW_OK) {
			printf("ERROR: 5F00 isn't under 4F00, but was selected\n");
			break;
		}
		if (iso_select_by_name(0x3F00) != SW_OK) {
			printf("ERROR: failed to select MF\n");
			break;
		}
		if (iso_select_by_name(0x5F00) != SW_OK) {
			printf("ERROR: failed to select 5F00\n");
			break;
		}
		result = SW_OK;
	} while (0);

	emu_close_flash();
	return result;
}

static emu_Result
test_07(void)
{
	ISO_SW result = SW_UNKNOWN;
	uint16_t len = 0;
	cmd_t cmds[5];
	uint8_t idx = 0;
	do {
		if (iso_initialize() != SW_OK) {
			printf("ERROR: failed to initialize file system\n");
			break;
		}

		cmds[idx].cmd = hex_to_bytes("622D 8302 3F00 8201 38 8A01 01 8D02 4003 8C07 6FFFFFFFFFFFFF AB14 8401DA9700840126970084012897008401249700", &len);
		cmds[idx++].len = len;
		cmds[idx].cmd = hex_to_bytes("6217 8302 1111 8202 0100 8002 003F 8A01 01 8C06 6BFFFFFF1111", &len);
		cmds[idx++].len = len;
		cmds[idx].cmd = hex_to_bytes("6217 8302 1112 8202 0100 8002 007F 8A01 01 8C06 6BFFFFFF1111", &len);
		cmds[idx++].len = len;
		
		for (uint8_t i = 0; i < idx; ++i) {
			if (iso_create_file(cmds[i].cmd, cmds[i].len) != SW_OK) {
				printf("ERROR: failed cmd No %d\n", i);
				break;
			}
		}
		
		for (uint8_t i = 0; i < idx; ++i) {
			free(cmds[i].cmd);
			cmds[i].len = 0;
		}
		result = SW_OK;
	} while (0);

	emu_close_flash();
	return result;
}

int
main(int argc, char* argv[])
{
	// test_04();
	test_05();
	test_06();
	test_07();

	return 0;
}