import discord
import pycld2 as cld2
import os
import ctypes
from keep_alive import keep_alive

recompile = "gcc -fPIC -shared tryptobot.c -o libtryptobot.so"
print(recompile)
os.system(recompile)
print("Recompiled `libtryptobot.so`.")
libc = ctypes.CDLL("libc.so.6")
libtrypto = ctypes.CDLL("./libtryptobot.so")
libtrypto.handle_message.argtypes = (ctypes.c_char_p,)
libtrypto.handle_message.restype = ctypes.POINTER(ctypes.c_char)

client = discord.Client()
@client.event
async def on_ready():
  print("Tryptobot online")
  print(client.user)

@client.event
async def on_message(message):
  if message.author != client.user and message.author.name != "Carl-bot":
    if message.channel.id == 939263185712721950:
      lang = cld2.detect(message.content)[2][0][0]
      if lang == "ENGLISH":
        await message.delete()
        await message.author.send(
          "Il est interdit d'envoyer des messages "
          "en anglais sur le canal #langues-étrangères."
        )
    if len(message.content) > 0 and message.content[0] == '%':
      if message.content[1:6] == "bonk ":
        horny_user = message.content[6:]
        await message.channel.send(
          f"{horny_user} go to horny jail",
          file=discord.File("cheems.png")
        )
      else:
        result = libtrypto.handle_message(
          ctypes.c_char_p(bytes(message.content, encoding="utf-8"))
        )
        await message.channel.send(
          ctypes.cast(result, ctypes.c_char_p).value.decode()
        )
        libc.free(result)

keep_alive()
token = os.environ.get("DISCORD_BOT_SECRET")
client.run(token)