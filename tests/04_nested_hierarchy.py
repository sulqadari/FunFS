from tests_common import *

def main():

	udp_connect() #("127.0.0.1", 8485)
	print("Connecting to IP 127.0.0.1 at port 8485")

	# create
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "          8201 38   8302 3F00 8A01 01 8D02 4003" + tlv("8C", "6FFFFFFFFFFFFF")))), "create MF")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "8002 000C 8202 0100 8302 3F01 8A01 01"           + tlv("8C", "6BFFFFFF1111")))), "create EF 3F01")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "8002 000C 8202 0100 8302 3F02 8A01 01"           + tlv("8C", "6BFFFFFF1111")))), "create EF 3F02")

	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "          8201 38   8302 4F00 8A01 01 8D02 4003" + tlv("8C", "6FFFFFFFFFFFFF")))), "create  4F00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "8002 000C 8202 0100 8302 4F01 8A01 01"           + tlv("8C", "6BFFFFFF1111")))), "create EF 4F01")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "8002 000C 8202 0100 8302 4F02 8A01 01"           + tlv("8C", "6BFFFFFF1111")))), "create EF 4F02")

	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "          8201 38   8302 5F00 8A01 01 8D02 4003" + tlv("8C", "6FFFFFFFFFFFFF")))), "create  5F00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "8002 000C 8202 0100 8302 5F01 8A01 01"           + tlv("8C", "6BFFFFFF1111")))), "create EF 5F01")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "8002 000C 8202 0100 8302 5F02 8A01 01"           + tlv("8C", "6BFFFFFF1111")))), "create EF 5F02")

	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "          8201 38   8302 6F00 8A01 01 8D02 4003" + tlv("8C", "6FFFFFFFFFFFFF")))), "create  6F00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "8002 000C 8202 0100 8302 6F01 8A01 01"           + tlv("8C", "6BFFFFFF1111")))), "create EF 6F01")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "8002 000C 8202 0100 8302 6F02 8A01 01"           + tlv("8C", "6BFFFFFF1111")))), "create EF 6F02")

	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "          8201 38   8302 7F00 8A01 01 8D02 4003" + tlv("8C", "6FFFFFFFFFFFFF")))), "create  7F00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "8002 000C 8202 0100 8302 7F01 8A01 01"           + tlv("8C", "6BFFFFFF1111")))), "create EF 7F01")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "8002 000C 8202 0100 8302 7F02 8A01 01"           + tlv("8C", "6BFFFFFF1111")))), "create EF 7F02")
	
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "          8201 38   8302 8F00 8A01 01 8D02 4003" + tlv("8C", "6FFFFFFFFFFFFF")))), "create  8F00")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "8002 000C 8202 0100 8302 8F01 8A01 01"           + tlv("8C", "6BFFFFFF1111")))), "create EF 8F01")
	udp_apdu_send(hex_to_bytes("00e0 0000" + len_val( tlv("62", "8002 000C 8202 0100 8302 8F02 8A01 01"           + tlv("8C", "6BFFFFFF1111")))), "create EF 8F02")

	# write
	udp_apdu_send(hex_to_bytes("00a4 0800" + len_val("3F00 3F01")), "select 3F01")
	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("111111111111111111111111")), "write data")
	
	udp_apdu_send(hex_to_bytes("00a4 0000" + len_val("3F02")), "select 3F02")
	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("222222222222222222222222")), "write data")

	udp_apdu_send(hex_to_bytes("00a4 0800" + len_val("4F00 4F01")), "select 3F01")
	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("333333333333333333333333")), "write data")
	
	udp_apdu_send(hex_to_bytes("00a4 0000" + len_val("4F02")), "select 4F02")
	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("444444444444444444444444")), "write data")

	udp_apdu_send(hex_to_bytes("00a4 0800" + len_val("5F00 5F01")), "select 3F01")
	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("555555555555555555555555")), "write data")
	
	udp_apdu_send(hex_to_bytes("00a4 0000" + len_val("5F02")), "select 5F02")
	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("666666666666666666666666")), "write data")

	udp_apdu_send(hex_to_bytes("00a4 0800" + len_val("6F00 6F01")), "select 3F01")
	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("777777777777777777777777")), "write data")
	
	udp_apdu_send(hex_to_bytes("00a4 0000" + len_val("6F02")), "select 6F02")
	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("888888888888888888888888")), "write data")

	udp_apdu_send(hex_to_bytes("00a4 0800" + len_val("7F00 7F01")), "select 3F01")
	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("999999999999999999999999")), "write data")
	
	udp_apdu_send(hex_to_bytes("00a4 0000" + len_val("7F02")), "select 7F02")
	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("AAAAAAAAAAAAAAAAAAAAAAAA")), "write data")

	udp_apdu_send(hex_to_bytes("00a4 0800" + len_val("8F00 8F01")), "select 3F01")
	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("BBBBBBBBBBBBBBBBBBBBBBBB")), "write data")
	
	udp_apdu_send(hex_to_bytes("00a4 0000" + len_val("8F02")), "select 8F02")
	udp_apdu_send(hex_to_bytes("00d0 0000" + len_val("CCCCCCCCCCCCCCCCCCCCCCCC")), "write data")

	# read
	udp_apdu_send(hex_to_bytes("00a4 0800" + len_val("3F00 3F01")), "select 3F01")
	udp_apdu_send(hex_to_bytes("00b0 0000 0C"), "read data")

	udp_apdu_send(hex_to_bytes("00a4 0000" + len_val("3F02")), "select 3F02")
	udp_apdu_send(hex_to_bytes("00b0 0000 0C"), "read data")

	udp_apdu_send(hex_to_bytes("00a4 0800" + len_val("4F00 4F01")), "select 4F01")
	udp_apdu_send(hex_to_bytes("00b0 0000 0C"), "read data")
	
	udp_apdu_send(hex_to_bytes("00a4 0000" + len_val("4F02")), "select 4F02")
	udp_apdu_send(hex_to_bytes("00b0 0000 0C"), "read data")

	udp_apdu_send(hex_to_bytes("00a4 0800" + len_val("5F00 5F01")), "select 5F01")
	udp_apdu_send(hex_to_bytes("00b0 0000 0C"), "read data")
	
	udp_apdu_send(hex_to_bytes("00a4 0000" + len_val("5F02")), "select 5F02")
	udp_apdu_send(hex_to_bytes("00b0 0000 0C"), "read data")

	udp_apdu_send(hex_to_bytes("00a4 0800" + len_val("6F00 6F01")), "select 6F01")
	udp_apdu_send(hex_to_bytes("00b0 0000 0C"), "read data")
	
	udp_apdu_send(hex_to_bytes("00a4 0000" + len_val("6F02")), "select 6F02")
	udp_apdu_send(hex_to_bytes("00b0 0000 0C"), "read data")

	udp_apdu_send(hex_to_bytes("00a4 0800" + len_val("7F00 7F01")), "select 7F01")
	udp_apdu_send(hex_to_bytes("00b0 0000 0C"), "read data")
	
	udp_apdu_send(hex_to_bytes("00a4 0000" + len_val("7F02")), "select 7F02")
	udp_apdu_send(hex_to_bytes("00b0 0000 0C"), "read data")

	udp_apdu_send(hex_to_bytes("00a4 0800" + len_val("8F00 8F01")), "select 8F01")
	udp_apdu_send(hex_to_bytes("00b0 0000 0C"), "read data")
	
	udp_apdu_send(hex_to_bytes("00a4 0000" + len_val("8F02")), "select 8F02")
	udp_apdu_send(hex_to_bytes("00b0 0000 0C"), "read data")

	udp_disconnect()

main()