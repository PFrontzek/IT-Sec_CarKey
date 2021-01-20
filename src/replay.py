import socket

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    sock.connect(("10.1.0.29", 10001))
    sock.send(bytes.fromhex("27000c2c0700ffb4ed8f120a19f189ace36b82f1f0c92ef46f615a6a3feccc7b7067a4b9e4ae6a"))
