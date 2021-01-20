import socket

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    sock.connect(("192.168.11.19", 10001))
    sock.send(bytes.fromhex("270038415a0001ab98ac959891798737190f333b754088021c62846fef13252daa63c18e499ccb"))
