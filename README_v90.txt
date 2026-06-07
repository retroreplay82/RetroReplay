Retro Replay v90 Final Main Song Quieter

Replace exactly:
game.c
music.s

Changes:
- Replaces the current main gameplay song with the uploaded final.s song.
- Embeds final.s directly into music.s as music_data_main.
- Quiets the song by lowering the main envelope from $cf to $c7 and softening sustain.
- Keeps title music setup if present.
- Clear/lose/game-over music still uses the original music data.
- No tileset.chr change.
- No sprite/art/dash/enemy/power-up changes.

If still too loud:
- Next softer version can use $c5.
If too quiet:
- Bring it back to $c9 or $ca.
