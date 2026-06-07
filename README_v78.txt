Retro Replay v78 Screen Fix + Enemy Slowdown

Replace exactly:
game.c
tileset.chr

Fixes:
- Restores the safe v76 tileset so GAME OVER / LEVEL CLEAR big text is not corrupted.
- Removes v77's custom fourth-slime CHR overwrite at tiles 0x62-0x65 because those tiles collided with big text/font graphics.
- Fourth slime now reuses the safe Angry Stack blob art with enemy palette 3.

Balance:
- Late-level enemy speed ramp is reduced.
- Nightmare slime starts at Stage 31 instead of Stage 21.
- Nightmare slime is slower and less constantly aggressive.
- Later enemies still get harder, but should not feel impossible.
