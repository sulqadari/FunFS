from tests_common import *

def main():
	print("Connecting to...")
	udp_connect()

	udp_apdu_send(hex_to_bytes("00e0 0000") + len_val("622D 8302 3F00 8201 38 8A01 01 8D02 4003 8C07 6FFFFFFFFFFFFF"))
	udp_apdu_send(hex_to_bytes("00e0 0000") + len_val("6217 8302 3F01 8202 0100 8002 0100 8A01 01 8C06 6BFFFFFF1111"))
	udp_apdu_send(hex_to_bytes("00a4 0800") + len_val("3F00 3F01"))
	udp_apdu_send(hex_to_bytes("00b0 0000 20"))
	udp_apdu_send(hex_to_bytes("00d0 0000") + len_val("0102030405060708090A 1112131415161718191A"))
	udp_apdu_send(hex_to_bytes("00b0 0000 20"))
	udp_apdu_send(hex_to_bytes("00a4 0800") + len_val("3F00 3F02"), 0x6A82)

	udp_disconnect()

main()