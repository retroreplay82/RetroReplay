Retro Replay v89 Main Song Lower Volume

Replace exactly:
game.c
music.s

Changes:
- Lowers only the uploaded main gameplay song volume.
- Main gameplay song envelope changed from very hot $cf to softer $c8.
- Original title music, clear music, lose music, game-over music, sound effects, and graphics are untouched.
- No tileset.chr change.
- No gameplay/dash/enemy/power-up changes.

If it is still too loud, the next step is an even softer v89b using $c6.
If it becomes too quiet, go halfway with $ca or $cb.
