import ctypes
import os

# find lib on linux or windows
libc = ctypes.CDLL("libc.so.6")

os.system("gcc -fPIC -shared tryptobot.c -o libtryptobot.so")
libtrypto = ctypes.CDLL("./libtryptobot.so")

libtrypto.handle_message.argtypes = (ctypes.c_char_p,)
libtrypto.handle_message.restype = ctypes.POINTER(ctypes.c_char)

result = libtrypto.handle_message(ctypes.c_char_p(bytes("%test hello  world fuck the     police 69,420", encoding="utf-8")))
print(ctypes.cast(result, ctypes.c_char_p).value.decode())
libc.free(result)
