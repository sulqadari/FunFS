from tests_common import *

def main():

	udp_connect()
	print(f"Connecting to IP {udp_params.index(0)} at port {udp_params.index(1)}")

	udp_apdu_send(hex_to_bytes("00e0 0000") + len_val("622D 8302 3F00 8201 38 8A01 01 8D02 4003 8C07 6FFFFFFFFFFFFF"))
	udp_apdu_send(hex_to_bytes("00e0 0000") + len_val("6217 8302 3F01 8202 0100 8002 0009 8A01 01 8C06 6BFFFFFF1111"))
	udp_apdu_send(hex_to_bytes("00a4 0800") + len_val("3F00 3F01"))
	udp_apdu_send(hex_to_bytes("00d0 0000") + len_val("010203"))
	udp_apdu_send(hex_to_bytes("00d0 0000") + len_val("04"))
	udp_apdu_send(hex_to_bytes("00d0 0001") + len_val("05"))
	udp_apdu_send(hex_to_bytes("00d0 0002") + len_val("06"))
	udp_apdu_send(hex_to_bytes("00d0 0003") + len_val("070809"))
	# udp_apdu_send(hex_to_bytes("00d0 0001") + len_val("0506070809"))
	
	udp_apdu_send(hex_to_bytes("00b0 0000 09"))

	udp_disconnect()

main()