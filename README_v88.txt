Retro Replay v88 Main Gameplay Song

Replace exactly:
game.c
music.s

Changes:
- Uses uploaded mainsong(1).s as the main in-game gameplay song.
- Embeds it directly into music.s as music_data_main.
- Gameplay switches to music_data_main and plays song 0 when the level starts.
- Clear/lose/game-over screens switch back to the original music_data.
- Keeps the current title intro setup if you are using v87.
- No tileset.chr change.
- No sprite/art/dash/enemy/power-up changes.
