from tests_common import *

def main():

	udp_connect() #("127.0.0.1", 8485)
	print("Connecting to IP 127.0.0.1 at port 8485")

	udp_apdu_send(hex_to_bytes("00e0 0000") + len_val("622D 8302 3F00 8201 38 8A01 01 8D02 4003 8C07 6FFFFFFFFFFFFF"), "create MF")
	udp_apdu_send(hex_to_bytes("00e0 0000") + len_val("6217 8302 3F01 8202 0100 8002 000A 8A01 01 8C06 6BFFFFFF1111"), "create EF 3F01")
	udp_apdu_send(hex_to_bytes("00a4 0800") + len_val("3F00 3F01"), "select 3F01")
	#udp_apdu_send(hex_to_bytes("00d0 0000") + len_val("01"), "write 01")
	#udp_apdu_send(hex_to_bytes("00d0 0001") + len_val("02"), "write 01")
	udp_apdu_send(hex_to_bytes("00d0 0000") + len_val("030405"), "write 030405")
	udp_apdu_send(hex_to_bytes("00d0 0001") + len_val("131415"), "write 131415")
	udp_apdu_send(hex_to_bytes("00b0 0000 0A"), "read 3F01")

	udp_apdu_send(hex_to_bytes("00d0 0000") + len_val("11"), "write 11")
	udp_apdu_send(hex_to_bytes("00d0 0001") + len_val("121314"), "write 121314")
	udp_apdu_send(hex_to_bytes("00b0 0000 0A"), "read 3F01")
	
	udp_disconnect()

main()