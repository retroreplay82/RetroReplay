Retro Replay v98 Restore Title Intro Music

Replace exactly:
game.c
music.s

Fix:
- Restores the title-screen intro music.
- Embeds intro(1).s into music.s as music_data_untitled.
- Title screen initializes music_data_untitled and plays song 0.
- After title, the game restores the original music_data before gameplay.
- Keeps current final main gameplay song setup.
- Keeps v97 4th/Nightmare Slime color fix.
- No tileset.chr change.
- No dash/enemy speed/power-up logic changes.
