import secrets

class GamingDice:
  def __init__(self, faces_count):
    self._faces = faces_count
  
  def roll(self):
    return 1 + secrets.randbelow(self._faces)
