import socket
import struct
import hashlib
from dataclasses import dataclass
from enum import IntEnum
import time


class Action(IntEnum):
    OPEN_DOOR = 1
    CLOSE_DOOR = -1


@dataclass
class Car:
    key: bytes
    sequence: int


@dataclass
class Package:
    length: int
    time: int
    sequence: int
    action: Action
    signature: bytes
    raw_data: bytes

    def get_data(self) -> bytes:
        return self.raw_data[0:7]

    def validate_time(self) -> bool:
        t = time.gmtime()
        t = t.tm_sec + 60 * t.tm_min + 60 * 60 * int(t.tm_hour / 4)
        t_min = (t - 20) % (60 * 60 * 6)
        t_max = (t + 20) % (60 * 60 * 6)
        return t_min < self.time < t_max

    def validate_sequence(self, car: Car) -> bool:
        return car.sequence < self.sequence < (car.sequence + 1000) % 2 ** 16

    def validate_signature(self, car: Car) -> bool:
        m = hashlib.sha256()
        m.update(car.key)
        m.update(self.get_data())
        return m.digest() == self.signature

    def validate(self, car: Car) -> bool:
        return self.validate_signature(car) and self.validate_time() and self.validate_sequence(car)

    @classmethod
    def get_struct_fmt(cls) -> str:
        return "< H H H b 32s"

    @classmethod
    def from_bytes(cls, data: bytes):
        o = cls(*struct.unpack(cls.get_struct_fmt(), data), raw_data=data)
        o.action = Action(o.action)
        return o


cars = [
        Car(
        key=bytes.fromhex("14523170731EC3E1F8F01D30688B915A4C8877CA50D59F6D1A09F01A40E5EF59"),
        sequence=0,
    ),
    Car(
        key=bytes.fromhex("14523170731EC3E1F8F01D30688B915A4C8877CA50D59F6D1A09F01A40E5EF58"),
        sequence=0,
    ),
        Car(
        key=bytes.fromhex("14523170731EC3E1F8F01D30688B915A4C8877CA50D59F6D1A09F01A40E5EF50"),
        sequence=0,
    ),
]


def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        while True:
            try:
                sock.bind(("10.1.0.29", 10001))
                break
            except Exception as e:
                print(e)
                time.sleep(0.5)
        print("ready")
        sock.listen(0)
        while True:
            connection, client_address = sock.accept()
            try:
                print("\n\n")
                print("connection from", client_address)
                data = connection.recv(1024)
                print(f"received: {data.hex()}")
                try:
                    p = Package.from_bytes(data)
                    print(p)
                    for car in cars:
                        if p.validate(car):
                            car.seq = p.sequence
                            print(p.action)
                except Exception as e:
                    print(e)
            finally:
                connection.close()


if __name__ == "__main__":
    main()
