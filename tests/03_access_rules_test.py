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

	udp_disconnect()

main()