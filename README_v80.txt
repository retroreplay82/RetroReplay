Retro Replay v80 Pause Power Legend

Replace exactly:
game.c

Changes:
- Adds a pause-screen power-up legend.
- Uses existing in-game sprites only:
  snowflake/freeze, lightning, skull clear, and chip.
- Adds control reminder: A USE / B DASH.
- Does not change tileset.chr.
- Does not move tiles.
- Does not alter enemy/power-up art.
- Keeps v78 slowdown balance and current gameplay.

Note:
- This intentionally avoids touching the Game Over tile issue so baddies/powerups stay stable.
