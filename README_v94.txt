Retro Replay v94 Freeze Never Touches Music

Replace exactly:
game.c

Fix:
- Freeze power no longer stops music when activated.
- Freeze power no longer pauses music when activated.
- Freeze power no longer resumes or restarts music when it ends.
- The main gameplay song simply keeps playing underneath the freeze effect.

This should fix:
- music restarting after freeze
- music staying silent after freeze

No music.s change.
No tileset.chr change.
No sprite/art/dash/enemy/power-up changes.

Based on v90 final quieter main song setup.
