from tests_common import *

def main():

	udp_connect() #("127.0.0.1", 8485)
	print("Connecting to IP 127.0.0.1 at port 8485")

	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "          8201 38   8302 3F00 8A01 01 8D02 4003" + tlv("8C", "6FFFFFFFFFFFFF")))), "create MF")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "8002 000C 8202 0100 8302 3F01 8A01 01"           + tlv("8C", "6BFFFFFF1111")))), "create EF 3F01")

	udp_apdu_send(hex_to_bytes("00a4 0800" + len_val("3F00 3F01")), "select 3F01")
	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("0102")), "write 0102")
	udp_apdu_send(hex_to_bytes("00d0 0002" + len_val("0304")), "write 0304")
	udp_apdu_send(hex_to_bytes("00d0 0004" + len_val("0506")), "write 0506")
	udp_apdu_send(hex_to_bytes("00d0 0006" + len_val("0708")), "write 0708")
	udp_apdu_send(hex_to_bytes("00d0 0008" + len_val("090A")), "write 090A")
	udp_apdu_send(hex_to_bytes("00b0 0000 0A"), "read 3F01")

	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("11121314")), "write 11121314")
	udp_apdu_send(hex_to_bytes("00d0 0004" + len_val("15161718191A")), "write 15161718191A")
	udp_apdu_send(hex_to_bytes("00b0 0000 0A"), "read 3F01")
	
	udp_disconnect()

main()