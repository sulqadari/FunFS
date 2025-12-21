from tests_common import *

def main():

	udp_connect() #("127.0.0.1", 8485)
	print("Connecting to IP 127.0.0.1 at port 8485")

	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val(tlv("62",tlv("83", "3F00") +tlv("82", "38") +tlv("8A", "01") +tlv("8D", "4003") +tlv("8C", "7F01020304050607")))),"create MF")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val(tlv("62",tlv("83", "4F00") +tlv("82", "38") +tlv("8A", "01") +tlv("8D", "4003") +tlv("8C", "3F010203040506")))),"create DF 4F00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val(tlv("62",tlv("83", "5F00") +tlv("82", "38") +tlv("8A", "01") +tlv("8D", "4003") +tlv("8C", "1F0102030405")))),"create DF 5F00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val(tlv("62",tlv("83", "6F00") +tlv("82", "38") +tlv("8A", "01") +tlv("8D", "4003") +tlv("8C", "0F01020304")))),"create DF 6F00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val(tlv("62",tlv("83", "7F00") +tlv("82", "38") +tlv("8A", "01") +tlv("8D", "4003") +tlv("8C", "07010203")))),"create DF 7F00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val(tlv("62",tlv("83", "8F00") +tlv("82", "38") +tlv("8A", "01") +tlv("8D", "4003") +tlv("8C", "030102")))),"create DF 8F00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val(tlv("62",tlv("83", "9F00") +tlv("82", "38") +tlv("8A", "01") +tlv("8D", "4003") +tlv("8C", "0101")))),"create DF 9F00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val(tlv("62",tlv("83", "AF00") +tlv("82", "38") +tlv("8A", "01") +tlv("8D", "4003") +tlv("8C", "00")))),"create DF AF00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val(tlv("62",tlv("83", "BF00") +tlv("82", "38") +tlv("8A", "01") +tlv("8D", "4003") +tlv("8C", "25818283")))),"create DF BF00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val(tlv("62",tlv("83", "CF00") +tlv("82", "38") +tlv("8A", "01") +tlv("8D", "4003") +tlv("8C", "51818283")))),"create DF CF00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val(tlv("62",tlv("83", "DF00") +tlv("82", "38") +tlv("8A", "01") +tlv("8D", "4003") +tlv("8C", "26818283")))),"create DF DF00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val(tlv("62",tlv("83", "EF00") +tlv("82", "38") +tlv("8A", "01") +tlv("8D", "4003") +tlv("8C", "2E81828384")))),"create DF EF00")
	# udp_apdu_send(hex_to_bytes("00e0 0000") + len_val("622D 8302 3F00 8201 38 8A01 01 8D02 4003 8C07 3F010203040506"), "create MF")
	# udp_apdu_send(hex_to_bytes("00e0 0000") + len_val("622C 8302 3F00 8201 38 8A01 01 8D02 4003 8C06 1F0102030405"), "create MF")
	# udp_apdu_send(hex_to_bytes("00e0 0000") + len_val("622B 8302 3F00 8201 38 8A01 01 8D02 4003 8C05 0F01020304"), "create MF")
	# udp_apdu_send(hex_to_bytes("00e0 0000") + len_val("622A 8302 3F00 8201 38 8A01 01 8D02 4003 8C04 07010203"), "create MF")

	# udp_apdu_send(hex_to_bytes("00e0 0000") + len_val("6217 8302 3F01 8202 0100 8002 000A 8A01 01 8C06 6B8504931211"), "create EF 3F01")
	# udp_apdu_send(hex_to_bytes("00a4 0800") + len_val("3F00 3F01"), "select 3F01")
	# udp_apdu_send(hex_to_bytes("00d0 0000") + len_val("0102"), "write 0102")
	# udp_apdu_send(hex_to_bytes("00d0 0002") + len_val("0304"), "write 0304")
	# udp_apdu_send(hex_to_bytes("00d0 0004") + len_val("0506"), "write 0506")
	# udp_apdu_send(hex_to_bytes("00d0 0006") + len_val("0708"), "write 0708")
	# udp_apdu_send(hex_to_bytes("00d0 0008") + len_val("090A"), "write 090A")
	# udp_apdu_send(hex_to_bytes("00b0 0000 0A"), "read 3F01")

	# udp_apdu_send(hex_to_bytes("00d0 0000") + len_val("11121314"), "write 11121314")
	# udp_apdu_send(hex_to_bytes("00d0 0004") + len_val("15161718191A"), "write 15161718191A")
	# udp_apdu_send(hex_to_bytes("00b0 0000 0A"), "read 3F01")
	
	udp_disconnect()

main()