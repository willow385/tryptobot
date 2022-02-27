import discord
import pycld2 as cld2
import os
import json
import random
import ctypes
from keep_alive import keep_alive
from dice import GamingDice

client = discord.Client()

recompile = "gcc -fPIC -shared tryptobot.c -o libtryptobot.so"
print(recompile)
os.system(recompile)
print("Recompiled `libtryptobot.so`.")
libc = ctypes.CDLL("libc.so.6")
libtrypto = ctypes.CDLL("./libtryptobot.so")
libtrypto.handle_message.argtypes = (ctypes.c_char_p,)
libtrypto.handle_message.restype = ctypes.POINTER(ctypes.c_char)

def load_commands():
  commands_list = json.loads(open("commands.json").read())["commands"]
  commands_list.sort(key=lambda x: x["command"])
  return commands_list


@client.event
async def on_ready():
  print("Tryptobot online")
  print(client.user)

mysterious_reminder_count = 0
previous_roll = "0d0"

@client.event
async def on_message(message):

  async def send(new_message):
    await message.channel.send(new_message)

  if message.author != client.user and message.author.name != "Carl-bot":
    if message.channel.id == 939263185712721950:
      lang = cld2.detect(message.content)[2][0][0]
      if lang == "ENGLISH":
        await message.delete()
        await message.author.send(
          "Il est interdit d'envoyer des messages "
          "en anglais sur le canal #langues-étrangères."
        )
    else
      try:
        global mysterious_reminder_count
        reminders = json.loads(
          open("mysterious_reminders.json", encoding="utf-8").read()
        )["reminders"]
        if "stn" in message.content or "stl" in message.content or "nor" in message.content:
          if mysterious_reminder_count == 2:
            await send(random.choice(reminders).replace("<USER>", message.author.mention))
          if mysterious_reminder_count >= 2:
            mysterious_reminder_count = 0
          else:
            mysterious_reminder_count += 1
      except:
        pass

    if len(message.content) > 0 and message.content[0] == '%':
      if message.content[1:6] == "bonk ":
        horny_user = message.content[6:]
        await message.channel.send(
          f"{horny_user} go to horny jail",
          file=discord.File("cheems.png")
        )

      # TODO reimplement in C
      elif message.content[1:6] == "roll ":
        dice = []
        global previous_roll
        dice_string = message.content[6:]
        dice_count = None
        dice_faces = None
        try:
          dice_info = dice_string.lower().split('d')
          dice_count = int(dice_info[0])
          dice_faces = int(dice_info[1])
          if dice_count < 50000 and dice_faces < 50000:
            for i in range(dice_count):
              dice.append(GamingDice(dice_faces))
            result = sum([die.roll() for die in dice])
            previous_roll = dice_string
            await send(f"Result of rolling {dice_count}d{dice_faces}: {result}")
          else:
            await send("Error: too many rolls or faces.")
        except:
          await send(f"Syntax error: `\"{dice_string}\"` is not valid dice notation.")

      # TODO reimplement in C
      elif message.content[1:7] == "reroll":
        dice_info = previous_roll.lower().split('d')
        dice_count = int(dice_info[0])
        dice_faces = int(dice_info[1])
        dice = [GamingDice(dice_faces) for i in range(dice_count)]
        result = sum([die.roll() for die in dice])
        await send(f"Result of rolling {dice_count}d{dice_faces}: {result}")

      elif message.content[1:5] == "add ":
        args = message.content.split(" ")
        a = int(args[1])
        b = int(args[2]) 
        await send(
          f"Result from calling C function `c_add_nums()`: {libtrypto.c_add_nums(a, b)}"
        )

      else:
        result = libtrypto.handle_message(
          ctypes.c_char_p(bytes(message.content, encoding="utf-8"))
        )
        await send(ctypes.cast(result, ctypes.c_char_p).value.decode())
        libc.free(result)


keep_alive()
token = os.environ.get("DISCORD_BOT_SECRET")
client.run(token)
