import pypicorom as rom
from struct import pack
from typing import Optional, List
import sys

CMD_WRITE_BYTES = 1
CMD_WRITE_WORDS = 2
CMD_READ_BYTES = 3
CMD_READ_WORDS = 4
CMD_FILL_BYTES = 5
CMD_FILL_WORDS = 6

class PicoError(Exception):
    pass

class Fill:
    def __init__(self, value):
        self.value = value

class Pico:
    def __init__(self, name):
        self.pico = rom.open(name)
        self.pico.start_comms(0x1ffff)
    
    def do_cmd(self, cmd : int, arg0 : int, arg1 : int, data : Optional[bytes] = None, response_size : int = 0) -> bytearray:
        if data is None:
            data = b''
        header = pack("<BIIH", cmd, arg0, arg1, len(data))
        self.pico.write(header + data)

        resp = self.pico.read_exact(response_size + 1, timeout=5)
        resp_data = resp[0:response_size]
        if resp[response_size] != cmd:
            raise PicoError(f"Incorrect tail on response, expected: {cmd} got: {resp[response_size]}")
        
        return resp_data
        
    def read_bytes(self, addr : int, count : int) -> bytearray:
        return self.do_cmd(CMD_READ_BYTES, addr, count, None, count)

    def read_words(self, addr : int, count : int) -> List[int]:
        data = self.do_cmd(CMD_READ_WORDS, addr, count, None, count * 2)
        ints = []
        for high, low in zip(data[0::2], data[1::2]):
            ints.append(low << 8 | high)
        return ints

    def write_bytes(self, addr : int, data: bytes):
        self.do_cmd(CMD_WRITE_BYTES, addr, 0, data, 0)

    def write_words(self, addr : int, data: List[int]):
        words = bytearray()
        for x in data:
            words += pack("<H", x)
        self.do_cmd(CMD_WRITE_WORDS, addr, 0, words, 0)

    def fill_bytes(self, addr: int, count: int, value: int):
        byte = bytearray([value])
        self.do_cmd(CMD_FILL_BYTES, addr, count, byte, 0)

    def fill_words(self, addr: int, count: int, value: int):
        word = pack("<H", value)
        self.do_cmd(CMD_FILL_WORDS, addr, count, word, 0)

class MemoryByteView:
    def __init__(self, pico: Pico, base_addr: int):
        self.pico = pico
        self.base_addr = base_addr
    
    def __getitem__(self, key):
        if type(key) == int:
            if key < 0:
                raise IndexError()
            resp = self.pico.read_bytes(self.base_addr + key, 1)
            return resp[0]
        elif type(key) == slice:
            if key.start < 0 or key.stop < key.start:
                raise IndexError()
            return self.pico.read_bytes(key.start + self.base_addr, key.stop - key.start)
        else:
            raise TypeError()

    def __setitem__(self, key, value):
        if type(key) == int:
            if key < 0:
                raise IndexError()
            self.pico.write_bytes(self.base_addr + key, bytes([value]))
        elif type(key) == slice:
            if type(value) == Fill:
                self.pico.fill_bytes(self.base_addr + key.start, key.stop - key.start, value.value) 
            else:
                self.pico.write_bytes(self.base_addr + key.start, value)
        else:
            raise TypeError()

class MemoryWordView:
    def __init__(self, pico: Pico, base_addr: int):
        self.pico = pico
        self.base_addr = base_addr
    
    def __getitem__(self, key):
        if type(key) == int:
            if key < 0:
                raise IndexError()
            resp = self.pico.read_words(key + self.base_addr, 1)
            return resp[0]
        elif type(key) == slice:
            if key.start < 0 or key.stop < key.start:
                raise IndexError()
            return self.pico.read_words(key.start + self.base_addr, int((key.stop - key.start) / 2))
        else:
            raise TypeError()

    def __setitem__(self, key, value):
        if type(key) == int:
            if key < 0:
                raise IndexError()
            self.pico.write_words(key + self.base_addr, [value])
        elif type(key) == slice:
            if type(value) == Fill:
                self.pico.fill_words(key.start + self.base_addr, int((key.stop - key.start) / 2), value.value) 
            else:
                self.pico.write_words(key.start + self.base_addr, value)
        else:
            raise TypeError()

if __name__ == '__main__':
    pico = Pico('cpu_low')

    memw = MemoryWordView(pico, 0xe0000000)
    memw[0] = 0xffff

