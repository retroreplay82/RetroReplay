Retro Replay Revert to v78 Safe State

Use this to undo v79.

Replace exactly:
game.c
tileset.chr

This reverts:
- v79 tile moving
- v79 broken/blinking baddies
- v79 broken/blinking powerups

This returns to:
- v78 slowdown balance
- safer enemy speed
- 4th slime starts later
- powerups and baddies back to the previous working tile setup

Note:
- This does not attempt another Game Over text fix yet. First get back to a playable state.
