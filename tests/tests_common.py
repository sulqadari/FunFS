import socket
import sys

# 1. UDP client machinery
# 2. hex-string to bytes converter
# 3. bytes to hex-string converter
# 4. a structure that resembles APDU protocol

udp_params = ("127.0.0.1", 8485)
udp_socket = None

def len_val(hex_str: str):
	result = hex_to_bytes(hex_str)
	return [len(result)] + result

def hex_to_bytes(hex_str: str):
	bytes_list = []
	firstNibble = True
	bt = 0
	for i in range(0, len(hex_str)):
		ch = hex_str[i] 
		if (ch == '0'):                nibble = 0
		elif (ch == '1'):              nibble = 1
		elif (ch == '2'):              nibble = 2
		elif (ch == '3'):              nibble = 3
		elif (ch == '4'):              nibble = 4
		elif (ch == '5'):              nibble = 5
		elif (ch == '6'):              nibble = 6
		elif (ch == '7'):              nibble = 7
		elif (ch == '8'):              nibble = 8
		elif (ch == '9'):              nibble = 9
		elif (ch == 'A' or ch == 'a'): nibble = 10
		elif (ch == 'B' or ch == 'b'): nibble = 11
		elif (ch == 'C' or ch == 'c'): nibble = 12
		elif (ch == 'D' or ch == 'd'): nibble = 13
		elif (ch == 'E' or ch == 'e'): nibble = 14
		elif (ch == 'F' or ch == 'f'): nibble = 15
		else:                          continue
		if firstNibble:
			firstNibble = False
			bt = nibble << 4
		else:
			bt += nibble
			bytes_list.append(bt)
			firstNibble = True

	return bytes_list

def byte_to_hex(ar: bytearray):
	rst = ''
	for a in ar:
		rst += "{:02x}".format(a)
	return rst

def udp_connect():
	global udp_socket
	udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	udp_socket.connect(udp_params)
	
def udp_disconnect():
	udp_socket.close()

def udp_apdu_send(cdata, expected_sw = 0x9000):
	print(">> " + byte_to_hex(cdata))

	udp_socket.send(bytearray(cdata))
	rdata     = udp_socket.recv(263)       # get array with responce
	
	ret_val   = rdata[:-2]                 # a list of bytes (except SW) to be returned to the user
	actual_sw = int.from_bytes(rdata[-2:]) # get SW bytes
	rdata     = rdata[:-2]                 # truncate SW bytes from incoming array
	rdata     = byte_to_hex(rdata)         # convert byte array into string

	print(f"<< {rdata} {actual_sw:X}")
	
	if actual_sw != expected_sw:
		print(f"ERROR: expected {expected_sw:X}, got {actual_sw:X}")
		
		udp_disconnect()
		sys.exit(1)
	
	return ret_val