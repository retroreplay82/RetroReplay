// Retro Replay: Pixel Panic
// This is a heavily modded fork from the great Chase NES Demo by Shiru.
// Retro-Replay.com
// Special thanks to screenname bigbastard25 for his help with this ROM.
//
// Major new features added:
// - Full Retro Replay visual retheme with new title screen, HUD, sprites, and palettes.
// - 50-stage structure using 14 layouts; Level 14 micro fit test.
// - Testing-only title-screen level select is enabled in this build.
// - GET READY countdown before each stage.
// - Level clear, well done, game over, score, lives, items, power HUD, and high score screens.
// - Pause screen with power-up legend and Retro-Replay.com branding.
// - New enemy slime art with multiple colors, late-game Nightmare Slime, and speed/aggression scaling.
// - Power-up system with Frozen Disk, Lightning Disk, Skull Clear, and B-button dash boost.
// - Testing-only Glitch Bomb mechanic: B picks up/drops stable fuse-flash bombs; otherwise B dashes.
// - Dropped code-chip collectibles and power-up drops.
// - Custom title intro music and quieter uploaded main gameplay song.
//
// Library includes
#include "neslib.h"
#include <string.h>

//#link "famitone2.s"
//#link "music.s"
//#link "sounds.s"

// CHR data
//#resource "tileset.chr"
//#link "tileset.s"

//include nametables for all the screens such as title or game over

#include "title_nam.h"
#include "level_nam.h"
#include "gameover_nam.h"

extern const void sound_data[];
extern const void music_data[];
extern const void music_data_main[];
extern const void music_data_untitled[];

#include "welldone_nam.h"

//include nametables for levels

#include "level1_nam.h"
#include "level2_nam.h"
#include "level3_nam.h"
#include "level4_nam.h"
#include "level5_nam.h"
#include "level6_nam.h"
#include "level7_nam.h"
#include "level8_nam.h"
#include "level9_nam.h"
#include "level10_nam.h"
// v160 ROM save: level11_nam.h was byte-identical to level1_nam.h, so use level1_nam as the patch base.

//game uses 12:4 fixed point calculations for enemy movements

#define FP_BITS  4

//max size of the game map

#define MAP_WDT      16
#define MAP_WDT_BIT    4
#define MAP_HGT      13

//macro for calculating map offset from screen space, as
//the map size is smaller than screen to save some memory

#define MAP_ADR(x,y)  ((((y)-2)<<MAP_WDT_BIT)|(x))

//size of a map tile

#define TILE_SIZE    16
#define TILE_SIZE_BIT  4

//movement directions, match to the gamepad buttons bits to
//simplify some things

#define DIR_NONE    0
#define DIR_LEFT    PAD_LEFT
#define DIR_RIGHT    PAD_RIGHT
#define DIR_UP      PAD_UP
#define DIR_DOWN    PAD_DOWN

//tile numbers for certain things in the game map
//i.e. which object corresponds to a character in the tileset

#define TILE_PLAYER    0x10
#define TILE_ENEMY1    0x11
#define TILE_ENEMY2    0x12
#define TILE_ENEMY3    0x13
#define TILE_ENEMY4    0x14  // v77: spawned late-game Nightmare Slime
#define TILE_WALL    0x40
#define TILE_EMPTY    0x44
#define TILE_ITEM    0x45
#define TILE_DROP    0x51  // Pixel Panic: Code Chip sprite art starts here, 2x2 meta sprite
#define TILE_ORB     0x55  // Pixel Panic: Frozen Disk power-up sprite art starts here, 2x2 metasprite
#define TILE_LIGHTNING_ORB 0x5a  // Lightning Bolt power-up sprite art starts here, 2x2 metasprite
#define TILE_SKULL_ORB     0x5e  // Skull Clear power-up sprite art starts here, 2x2 metasprite

#define CHIP_MAX              8
#define CODE_CHIP_BASE        3
#define CODE_CHIP_DROP_CHANCE 2
#define ORB_DROP_CHANCE       8   // debug-friendly: power-up appears quickly
#define FREEZE_DISK_TIME     300  // about 5 seconds at 60 fps
#define FREEZE_CHIME_PERIOD   60   // chime once per second while frozen
#define LIGHTNING_TIME       300  // about 5 seconds at 60 fps
#define LIGHTNING_SHOT_RATE   15  // auto-fire every quarter second
#define SKULL_CLEAR_TIME    180  // about 3 seconds with no slimes on board
#define DASH_BOOST_SPEED    (4<<FP_BITS)
#define DASH_BOOST_TIME     60
#define DASH_BOOST_MAX_TILES 2
#define POWER_FREEZE           1
#define POWER_LIGHTNING        2
#define POWER_SKULL_CLEAR      3
#define SHOT_MAX              3
#define SHOT_SPEED            5
#define SHOT_TILE             0x59
#define BOMB_NONE             0
#define BOMB_FLOOR            1
#define BOMB_HELD             2
#define BOMB_TIME           240  // about 4 seconds at 60 fps
#define BOMB_BLAST_TIME      24
#define BOMB_RADIUS          34
#define BOMB_PICKUP_RANGE    18
#define BOMB_BREAK_RADIUS     1  // breaks interior walls in a 3x3 tile area
#define BOMB_DROP_CHANCE     12
#define BOMB_TILE_A        0xe8
#define BOMB_TILE_B        0xe9
#define BOMB_TILE_C        0xea
#define BOMB_TILE_D        0xeb
#define BOMB_RED_TILE_A    0xec
#define BOMB_RED_TILE_B    0xed
#define BOMB_RED_TILE_C    0xee
#define BOMB_RED_TILE_D    0xef
#define GATE_TILE_A        0xf0
#define GATE_TILE_B        0xf1
#define GATE_TILE_C        0xf2
#define GATE_TILE_D        0xf3

#define SCORE_CART    1
#define SCORE_CHIP    1
#define SCORE_ORB     1
#define SCORE_ENEMY   1
#define SCORE_LEVEL   0

//number of levels in the game

#define LEVEL_LAYOUTS  14
#define LEVELS_ALL     50

//numbers for screens that are displayed by the same function as the
//level number

#define SCREEN_GAMEOVER  (LEVELS_ALL+0)
#define SCREEN_WELLDONE  (LEVELS_ALL+1)

//total number of moving characters on the screen

#define PLAYER_MAX  5  // v77: player + three normal slimes + late-game Nightmare Slime

//sound effect numbers, it is easier to use meaningful defines than
//remember actual numbers of the effects

#define SFX_START    0
#define SFX_ITEM    1
#define SFX_RESPAWN1  2
#define SFX_RESPAWN2  3

//sub songs numbers in the music.ftm

#define MUSIC_LEVEL      0
#define MUSIC_GAME      1
#define MUSIC_CLEAR      2
#define MUSIC_GAME_OVER    3
#define MUSIC_WELL_DONE    4
#define MUSIC_LOSE      5

//palettes data
//all the palettes are designed in NES Screen Tool, then copy/pasted here
//with Palettes/Put C data to clipboard function
/*{pal:"nes",layout:"nes"}*/
const unsigned char palGame1[16]={ 0x0f,0x04,0x14,0x30,0x0f,0x11,0x21,0x30,0x0f,0x05,0x25,0x30,0x0f,0x19,0x29,0x30 };
/*{pal:"nes",layout:"nes"}*/
const unsigned char palGame2[16]={ 0x0f,0x03,0x13,0x30,0x0f,0x12,0x22,0x30,0x0f,0x14,0x24,0x30,0x0f,0x19,0x29,0x30 };
/*{pal:"nes",layout:"nes"}*/
const unsigned char palGame3[16]={ 0x0f,0x05,0x15,0x30,0x0f,0x11,0x21,0x30,0x0f,0x04,0x24,0x30,0x0f,0x1c,0x2c,0x30 };
/*{pal:"nes",layout:"nes"}*/
const unsigned char palGame4[16]={ 0x0f,0x02,0x12,0x30,0x0f,0x14,0x24,0x30,0x0f,0x11,0x21,0x30,0x0f,0x06,0x16,0x30 };
/*{pal:"nes",layout:"nes"}*/
const unsigned char palGame5[16]={ 0x0f,0x04,0x24,0x30,0x0f,0x05,0x25,0x30,0x0f,0x11,0x21,0x30,0x0f,0x19,0x29,0x30 };
/*{pal:"nes",layout:"nes"}*/
// v7 art fix: enemy slime is a pointed, classic slime silhouette with a mean black face.
// Both sprite CHR banks use the same enemy art; movement comes from a tiny Y bob in code.
// v6 art fix: enemy slime, Code Chips, and Frozen Disk are written identically into both sprite CHR banks.
// The game swaps sprite banks for animation, so both banks must match or pickups/enemies visibly change shape.
// v5 sprite art fix: stable slime silhouette, visible black face, no white pixels.
// Sprite palette fix: enemy palettes use visible black for face details.
// Enemy CHR tiles 0x4d-0x50 were also edited so transparent face holes
// become real black pixels and the old white forehead shine is color 2.
const unsigned char palGameSpr[16]={ 0x0f,0x14,0x2c,0x30,0x0f,0x0f,0x25,0x15,0x0f,0x0f,0x21,0x11,0x0f,0x0f,0x29,0x19 }; // v97 palette 3 restored to green; 4th slime uses palette 0

// v40b pause palettes: make the playfield black and make pause text bright.
// Sprite palette 0 is forced white for the tile-font text; slime palettes stay colored.
const unsigned char palPauseBg[16]={ 0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f };
const unsigned char palPauseSpr[16]={ 0x0f,0x30,0x30,0x30,0x0f,0x05,0x25,0x30,0x0f,0x11,0x21,0x30,0x0f,0x19,0x29,0x30 };
/*{pal:"nes",layout:"nes"}*/
//Retro Replay: Pixel Panic title palette - dark, purple, cyan, white
const unsigned char palTitle[16]={ 0x0f,0x04,0x14,0x30,0x0f,0x11,0x21,0x30,0x0f,0x05,0x25,0x30,0x0f,0x13,0x23,0x30 };


//metasprites

const unsigned char sprPlayer[]={
  0,-1,0x49,0,
  8,-1,0x4a,0,
  0, 7,0x4b,0,
  8, 7,0x4c,0,
  128
};

const unsigned char sprEnemy1[]={
  0, 0,0x4d,1,
  8, 0,0x4e,1,
  0, 8,0x4f,1,
  8, 8,0x50,1,
  128
};

const unsigned char sprEnemy2[]={
  0, 0,0x4d,2,
  8, 0,0x4e,2,
  0, 8,0x4f,2,
  8, 8,0x50,2,
  128
};

const unsigned char sprEnemy3[]={
  0, 0,0x4d,3,
  8, 0,0x4e,3,
  0, 8,0x4f,3,
  8, 8,0x50,3,
  128
};

// v78: late-game Nightmare Slime reuses safe slime tiles.
// The v77 custom tiles at 0x62-0x65 collided with big text/font graphics.
// v97: use sprite palette 0 for an icy/cyan special slime.
// This keeps regular slimes on pink, blue, and green palettes.
const unsigned char sprEnemy4[]={
  0, 0,0x4d,0,
  8, 0,0x4e,0,
  0, 8,0x4f,0,
  8, 8,0x50,0,
  128
};

//Sprite-based dropped item. It uses cleaned sprite copies of the
//map collectible art (tiles 0x51-0x54). The floor/background pixels
//are transparent, so the dropped item no longer discolors the maze.
const unsigned char sprCodeChip[]={
  0, 0,0x51,3,
  8, 0,0x52,3,
  0, 8,0x53,3,
  8, 8,0x54,3,
  128
};

// sprCodeChip was identical to sprCodeChip; reuse the same metasprite to save ROM.

//Sprite-based Frozen Disk pickup.
const unsigned char sprOrb[]={
  0, 0,0x55,0,
  8, 0,0x56,0,
  0, 8,0x57,0,
  8, 8,0x58,0,
  128
};

//Sprite-based Lightning Disk pickup. Same floppy idea, but with a cyan bolt.
const unsigned char sprLightningOrb[]={
  0, 0,0x5a,0,
  8, 0,0x5b,0,
  0, 8,0x5c,0,
  8, 8,0x5d,0,
  128
};

//Sprite-based Skull Clear pickup. Grabbing it stores a skull power;
//press A to banish all slimes from the board for a few seconds.
const unsigned char sprSkullOrb[]={
  0, 0,0x5e,0,
  8, 0,0x5f,0,
  0, 8,0x60,0,
  8, 8,0x61,0,
  128
};

// v107: improved 16x16 Glitch Bomb sprite in safe high tile slots.
// Normal and red blink use the same shape, just different CHR color indices.
const unsigned char sprBomb[]={
  0, 0,BOMB_TILE_A,1,
  8, 0,BOMB_TILE_B,1,
  0, 8,BOMB_TILE_C,1,
  8, 8,BOMB_TILE_D,1,
  128
};

const unsigned char sprBombRed[]={
  0, 0,BOMB_RED_TILE_A,1,
  8, 0,BOMB_RED_TILE_B,1,
  0, 8,BOMB_RED_TILE_C,1,
  8, 8,BOMB_RED_TILE_D,1,
  128
};

// v167 ROM save: removed unused sprBreakGate metasprite. Gate markers are background tiles now.

//list of metasprites

const unsigned char* const sprListPlayer[]={ sprPlayer,sprEnemy1,sprEnemy2,sprEnemy3,sprEnemy4 };


//list of the levels, include pointer to the packed nametable of the level,
//and pointer to the associated palette

const unsigned char* const levelList[LEVEL_LAYOUTS*2]={
level1_nam,palGame1,
level2_nam,palGame2,
level3_nam,palGame3,
level4_nam,palGame3,
level5_nam,palGame3,
level6_nam,palGame1,
level7_nam,palGame2,
level8_nam,palGame3,
level9_nam,palGame4,
level10_nam,palGame5,
level1_nam,palGame3,
level1_nam,palGame1,
level1_nam,palGame2,
level1_nam,palGame4
};


//preinitialized update list used during the gameplay

const unsigned char updateListData[]={
0x28,0x00,TILE_EMPTY,  //these four entries are used to clear
0x28,0x00,TILE_EMPTY,  //the level tile after an item/orb/chip is collected
0x28,0x00,TILE_EMPTY,
0x28,0x00,TILE_EMPTY,
0x20,0x4d,0x10,      //SCORE digits on HUD line 1
0x20,0x4e,0x10,
0x20,0x4f,0x10,
0x20,0x50,0x10,
0x20,0x68,0x10,      //ITEMS collected digits on HUD line 2
0x20,0x69,0x10,
0x20,0x6a,0x10,
0x20,0x76,0x00,      //POWER letter: F or L, blank when inactive
0x20,0x77,0x00,      //POWER timer digit
NT_UPD_EOF
};


//a nametable string with the game stats, created in NES Screen Tool
//and copy/pasted here with Shift+C

//HUD line 1: "LV:0 SCORE:0000 LI:0       "
const unsigned char statsStr[27]={
  0x2c,0x36,0x1a,0x10,0x00,0x33,0x23,0x2f,0x32,
  0x25,0x1a,0x10,0x10,0x10,0x10,0x00,0x2c,0x29,
  0x1a,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

//HUD line 2: "ITEMS:000/000 POWER:--     "
const unsigned char statsStr2[27]={
  0x29,0x34,0x25,0x2d,0x33,0x1a,0x10,0x10,0x10,
  0x0f,0x10,0x10,0x10,0x00,0x30,0x2f,0x37,0x25,
  0x32,0x1a,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

//list of screens such as level number, game over, and well done
//contains pointers to the packed nametables

const unsigned char* const screenList[3]={ level_nam,gameover_nam,welldone_nam };

//list of sub songs for corresponding screens

const unsigned char screenMusicList[3]={MUSIC_LEVEL,MUSIC_GAME_OVER,MUSIC_WELL_DONE};

//Level clear splash text, using the same built-in font tile mapping as the HUD.
//0x00 is space, 0x21 is A, 0x22 is B, etc.
const unsigned char levelClearStr[5]={0x23,0x2c,0x25,0x21,0x32}; //CLEAR
const unsigned char nextLevelStr[6]={0x32,0x25,0x21,0x24,0x39,0x01}; //READY! v166: shorter to save PRG
const unsigned char clearScoreStr[6]={0x33,0x23,0x2f,0x32,0x25,0x00}; //SCORE 
// v161: level clear URL reuses gameOverUrlStr to save 16 PRG bytes.

//Game over score text. 0x00 is space, A=0x21, 0x0d is dash, 0x0e is dot.
// v161: game over SCORE label reuses clearScoreStr first 5 bytes to save 5 PRG bytes.
const unsigned char gameOverHighStr[2]={0x28,0x29}; //HI v166: shorter to save PRG
const unsigned char gameOverUrlStr[16]={0x32,0x25,0x34,0x32,0x2f,0x0d,0x32,0x25,0x30,0x2c,0x21,0x39,0x0e,0x23,0x2f,0x2d}; //RETRO-REPLAY.COM

//forward declaration used by game over / level clear score displays
void put_num(unsigned int adr,unsigned int num,unsigned char len);
void put_score_num(unsigned int adr,unsigned int num);

// Pause overlay text, drawn with sprites so gameplay nametable stays untouched.
// Uses the built-in tile font mapping: A=0x21, space=0x00, '-'=0x0d, '.'=0x0e.
const unsigned char pauseStr[6]={0x30,0x21,0x35,0x33,0x25,0x24}; //PAUSED
// v161: removed unused pauseResumeStr to save 14 PRG bytes.
// v160: pause URL reuses gameOverUrlStr to save 16 ROM bytes.

// v80 pause power legend. Text is sprite-drawn; no CHR changes.
// Tile font mapping: A=0x21, B=0x22 ... Z=0x3a, space=0x00.
const unsigned char pauseLegendStr[5]={0x30,0x2f,0x37,0x25,0x32}; //POWER v166: shorter to save PRG
const unsigned char pauseFreezeStr[3]={0x29,0x23,0x25}; //ICE v166: shorter to save PRG
const unsigned char pauseZapStr[3]={0x3a,0x21,0x30}; //ZAP
// v160: pause CLEAR text reuses levelClearStr to save 5 ROM bytes.
const unsigned char pauseChipStr[4]={0x23,0x28,0x29,0x30}; //CHIP
const unsigned char pauseUseDashStr[7]={0x21,0x00,0x35,0x33,0x25,0x00,0x22}; //A USE B v166: shorter to save PRG

// v167 ROM save: removed title STG label; tiny digits remain.

//large 1-5 numbers nametable definitons, 2x3 tiles each
//numbers were drawn one next to the other in NES Screen Tool,
//then that part of nametable was copy/pasted here with Shift+C
//here they are orderded as
//   top row of 11 22 33 44 55
//middle row of 11 22 33 44 55
//bottom row of 11 22 33 44 55

const unsigned char largeNums[10*3]={
  0x7a,0x7b,0x7c,0x7d,0x7c,0x7d,0x7e,0x7f,0x80,0x81,
  0x86,0x7b,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x7d,
  0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x7f,0x94,0x95
};


// v58: 3x5 block-digit font for true big level numbers 01-50.
// Uses an existing blue LEVEL-screen tile so we do not need new CHR art.
#define BIG_LEVEL_NUM_TILE 0x67
const unsigned char bigLevelDigitBits[50]={
  0x07,0x05,0x05,0x05,0x07, //0
  0x02,0x06,0x02,0x02,0x07, //1
  0x07,0x01,0x07,0x04,0x07, //2
  0x07,0x01,0x07,0x01,0x07, //3
  0x05,0x05,0x07,0x01,0x01, //4
  0x07,0x04,0x07,0x01,0x07, //5
  0x07,0x04,0x07,0x05,0x07, //6
  0x07,0x01,0x01,0x01,0x01, //7
  0x07,0x05,0x07,0x05,0x07, //8
  0x07,0x05,0x07,0x01,0x07  //9
};

void put_big_level_num(unsigned int adr,unsigned char num)
{
  unsigned char tens,ones,row,col,bits,digit;

  tens=num/10;
  ones=num%10;

  for(row=0;row<5;++row)
  {
    vram_adr(adr+((unsigned int)row<<5));

    digit=tens;
    bits=bigLevelDigitBits[digit*5+row];
    for(col=0;col<3;++col)
    {
      vram_put(bits&(0x04>>col)?BIG_LEVEL_NUM_TILE:0x00);
    }

    vram_put(0x00);

    digit=ones;
    bits=bigLevelDigitBits[digit*5+row];
    for(col=0;col<3;++col)
    {
      vram_put(bits&(0x04>>col)?BIG_LEVEL_NUM_TILE:0x00);
    }
  }
}

// v82: The big level number sits at tile x20-y12 and is 7x5 tiles.
// These attribute bytes switch only that right-side number area to BG palette 2.
// Palette 2 is set to purple below. This avoids touching CHR/sprite art.
void put_big_level_num_purple_attrs(void)
{
  // Attribute row for y12-y15, columns covering x20-x27.
  vram_adr(NAMETABLE_A+0x03c0+3*8+5);
  vram_put(0xaa); // all quadrants palette 2
  vram_put(0xaa); // all quadrants palette 2

  // Attribute row for y16-y19, same columns.
  // Only top quadrants need palette 2 because the number is 5 tiles tall.
  vram_adr(NAMETABLE_A+0x03c0+4*8+5);
  vram_put(0x0a); // top half palette 2
  vram_put(0x0a); // top half palette 2
}


//array for game map, contains walls, empty spaces, and items

static unsigned char map[MAP_WDT*MAP_HGT];


//put all the subsequent global vars into zeropage, to make code faster and shorter

#pragma bss-name(push,"ZEROPAGE")
#pragma data-name(push,"ZEROPAGE")

//set of general purpose global vars that are used everywhere in the program
//this makes code faster and shorter, although not very convinent and readable

static unsigned char i,j;
static unsigned char ptr,spr;
static unsigned char px,py;
static unsigned char wait;
static unsigned int i16;
static int iy,dy;

//this array is used to determine movement directions for enemies

static unsigned char dir[4];

//this array is used to convert nametable into game map, row by row.
// It is only needed while loading a stage, so keep it out of ZEROPAGE.
#pragma bss-name(push,"BSS")
#pragma data-name(push,"DATA")
static unsigned char nameRow[32];
#pragma data-name(pop)
#pragma bss-name(pop)

//number of moving characters on current level

static unsigned char player_all;

//character variables

static unsigned int  player_x    [PLAYER_MAX];
static unsigned int  player_y    [PLAYER_MAX];
static unsigned char player_dir  [PLAYER_MAX];
static int           player_cnt  [PLAYER_MAX];
static unsigned int  player_speed[PLAYER_MAX];
static unsigned char player_wait [PLAYER_MAX];
// v77: sprite/behavior kind. 0=player, 1-3=normal slimes, 4=Nightmare Slime.
static unsigned char player_kind [PLAYER_MAX];

//number of items on current level, total and collected

static unsigned char items_count;
static unsigned char items_collected;
static unsigned char carts_count;
static unsigned char chips_required;
static unsigned char chips_spawned;
static unsigned char chip_active[CHIP_MAX];
static unsigned char chip_x[CHIP_MAX];
static unsigned char chip_y[CHIP_MAX];
static unsigned char orb_spawned;
static unsigned char lightning_spawned;
static unsigned char skull_spawned;
static unsigned char orb_active;
static unsigned char orb_type;
static unsigned char orb_x;
static unsigned char orb_y;
static unsigned int  freeze_timer;
static byte          freeze_chime_timer;
static unsigned int  lightning_timer;
static byte          lightning_shot_timer;

// Keep the lightning shot arrays out of ZEROPAGE.
// v16 overflowed the NES ZP segment by a few bytes; these do not need
// zero-page speed, so moving them back to normal BSS fixes the build.
#pragma bss-name(push,"BSS")
#pragma data-name(push,"DATA")
static unsigned char shot_active[SHOT_MAX];
static unsigned char shot_x[SHOT_MAX];
static unsigned char shot_y[SHOT_MAX];
static unsigned char shot_dir[SHOT_MAX];
#pragma data-name(pop)
#pragma bss-name(pop)

static unsigned int  game_score;

// Keep the high score in normal RAM so it does not push ZEROPAGE over.
#pragma bss-name(push,"BSS")
#pragma data-name(push,"DATA")
static unsigned int  high_score;
#pragma data-name(pop)
#pragma bss-name(pop)

// v54: title-screen level select. Kept in normal RAM so we do not risk
// another ZEROPAGE overflow. This is the stage the next game starts on.
#pragma bss-name(push,"BSS")
#pragma data-name(push,"DATA")
static unsigned char start_level; // v103 testing build: selected title-screen start stage
#pragma data-name(pop)
#pragma bss-name(pop)

// v59/v60: held power-up and dash boost timer are kept in normal RAM to avoid ZP overflow.
#pragma bss-name(push,"BSS")
#pragma data-name(push,"DATA")
static unsigned char held_power;
static unsigned char dash_boost_timer;
static unsigned char dash_boost_tiles;
static unsigned int  skull_clear_timer;
static unsigned char skull_flash_timer;
static unsigned char bomb_state;
static unsigned char bomb_x;
static unsigned char bomb_y;
static unsigned int  bomb_timer;
static unsigned char bomb_blast_timer;
static unsigned char bomb_blast_x;
static unsigned char bomb_blast_y;
static unsigned char level_patch_save_i;
#pragma data-name(pop)
#pragma bss-name(pop)

// Spawn positions are mostly used on deaths/respawns, not every hot movement step.
// Keep them in normal RAM to free 20 bytes of scarce zero page.
#pragma bss-name(push,"BSS")
#pragma data-name(push,"DATA")
static unsigned int  enemy_spawn_x[PLAYER_MAX];
static unsigned int  enemy_spawn_y[PLAYER_MAX];
#pragma data-name(pop)
#pragma bss-name(pop)

//game state variables

static unsigned char game_level;
static unsigned char game_lives;

//game state flags, they are 0 or 1

static unsigned char game_done;
static unsigned char game_paused;
static unsigned char game_clear;

//system vars used everywhere as well

static unsigned char frame_cnt;
static unsigned char bright;

//update list
// Keep this out of ZEROPAGE. The v17 HUD made the update list larger,
// which overflowed the NES zero-page segment. Normal RAM is fine here.
#pragma bss-name(push,"BSS")
#pragma data-name(push,"DATA")
static unsigned char update_list[13*3+1];
#pragma data-name(pop)
#pragma bss-name(pop)



//smoothly fade current bright to the given value
//in case when to=0 it also stops the music,
//turns the display off, reset vram update and scroll

void pal_fade_to(unsigned to)
{
  if(!to) music_stop();

  while(bright!=to)
  {
    delay(4);
    if(bright<to) ++bright; else --bright;
    pal_bright(bright);
  }

  if(!bright)
  {
    ppu_off();
    set_vram_update(NULL);
    scroll(0,0);
  }
}



//draw a small sprite text string using the built-in font tiles

unsigned char oam_text(unsigned char x,unsigned char y,const unsigned char* str,unsigned char len,unsigned char sprid,unsigned char attr)
{
  while(len)
  {
    if(*str) sprid=oam_spr(x,y,*str,attr,sprid);
    x+=8;
    ++str;
    --len;
  }

  return sprid;
}



//show title screen

void title_screen(void)
{
  // v124: clear leftover gameplay VRAM updates before title/menu drawing.
  set_vram_update(NULL);
  for(i=0;i<32;++i) update_list[i]=NT_UPD_EOF;

  // v24: title art now fills the screen, so do not shift it right.
  // This fixes the slightly clipped O in RETRO.
  scroll(0,240);

  vram_adr(NAMETABLE_A);
  vram_unrle(title_nam);

  vram_adr(NAMETABLE_C);//clear second nametable, as it is visible in the jumping effect
  vram_fill(0,1024);

  pal_bg(palTitle);
  pal_spr(palGameSpr);
  pal_bright(4);
  oam_clear();
  ppu_on_all();

  // v98: restored title intro music.
  famitone_init(&music_data_untitled);
  sfx_init(&sound_data);
  music_play(0);

  delay(20);//delay just to make it look better

  iy=240<<FP_BITS;
  dy=-8<<FP_BITS;
  frame_cnt=0;
  wait=160;
  bright=4;

  while(1)
  {
    ppu_wait_frame();

    scroll(0,iy>>FP_BITS);

    // Little title-screen parade: the horn guy runs across with slimes chasing.
    // v27 safe title fix: keep the known-working title tiles, but move the parade lower
    // so the sprites do not walk through the words.
    spr=0;
    i=frame_cnt<<1;
    py=198+((frame_cnt&8)>>3);
    spr=oam_meta_spr(i,py,spr,sprPlayer);
    spr=oam_meta_spr(i-44,py+1,spr,sprEnemy1);
    spr=oam_meta_spr(i-78,py+2,spr,sprEnemy2);
    spr=oam_meta_spr(i-112,py+1,spr,sprEnemy3);

    // v167 ROM save: keep tiny test level select, but remove label/up/down/select code.
    // LEFT/RIGHT changes one stage. START begins on displayed stage.
    spr=oam_spr(120,216,0x10+(start_level+1)/10,0,spr);
    spr=oam_spr(128,216,0x10+(start_level+1)%10,0,spr);
    oam_hide_rest(spr);

    ptr=pad_trigger(0);

    if(ptr&PAD_RIGHT)
    {
      if(start_level+1<LEVELS_ALL) ++start_level; else start_level=0;
      sfx_play(SFX_ITEM,1);
    }

    if(ptr&PAD_LEFT)
    {
      if(start_level) --start_level; else start_level=LEVELS_ALL-1;
      sfx_play(SFX_ITEM,1);
    }

    if(ptr&PAD_START) break;

    ++frame_cnt;
    iy+=dy;

    if(iy<0)
    {
      iy=0;
      dy=-dy>>1;
    }

    if(dy>(-8<<FP_BITS)) dy-=2;

    if(wait)
    {
      --wait;
    }
    else
    {
      pal_col(2,(frame_cnt&32)?0x0f:0x20);//blinking press start text
    }
  }

  scroll(0,0);//if start is pressed, show the title at whole
  sfx_play(SFX_START,0);

  for(i=0;i<16;++i)//and blink the text faster
  {
    pal_col(2,i&1?0x0f:0x20);
    delay(4);
  }

  pal_fade_to(0);
}



//show level intro, game over, or well done screen

void show_screen(unsigned char num)
{
  scroll(-4,0); //all the screens are misaligneg horizontally by half of a tile

  if(num<LEVELS_ALL) spr=0; else spr=num-LEVELS_ALL+1;//get offset in the screens list

  vram_adr(NAMETABLE_A);
  vram_unrle(screenList[spr]);

  if(!spr)//if it is the level screen, print the real level number big
  {
    // v58: remove the little STAGE line, but keep the level number big.
    // This draws 01-50 as chunky 3x5 block digits beside LEVEL.
    put_big_level_num(NAMETABLE_A+0x0194,num+1);
    put_big_level_num_purple_attrs();
  }

  //Retro Replay: add final score/high score to the Game Over screen.
  //This keeps high score session-based, like an arcade/NES test build.
  if(num==SCREEN_GAMEOVER)
  {
    if(game_score>high_score) high_score=game_score;

    vram_adr(NAMETABLE_A+0x0228);
    vram_write((unsigned char*)clearScoreStr,5);
    put_score_num(NAMETABLE_A+0x022f,game_score);

    vram_adr(NAMETABLE_A+0x0269);
    vram_write((unsigned char*)gameOverHighStr,2);
    put_score_num(NAMETABLE_A+0x026f,high_score);

    vram_adr(NAMETABLE_A+0x02ca);
    vram_write((unsigned char*)gameOverUrlStr,16);
  }

  i16=(num==SCREEN_GAMEOVER)?0x1525:0x1121;//two colors for flashing text in LSB and MSB

  frame_cnt=0;

  pal_col(2,i16&0xff);//this palette entry is used for flashing text
  pal_col(3,0x30);
  pal_col(6,0x30);
  // v82: BG palette 2 used only for the big level number.
  pal_col(9,0x15);
  pal_col(10,0x25);
  pal_col(11,0x30);
  ppu_on_bg();

  pal_fade_to(4);
  music_play(screenMusicList[spr]);

  if(!spr)//if it is the level screen, just wait one second
  {
    delay(50);
  }
  else//otherwise wait for Start button and display flashing text
  {
    while(1)
    {
      ppu_wait_frame();

      pal_col(2,frame_cnt&2?i16&0xff:i16>>8);

      if(pad_trigger(0)&PAD_START) break;

      ++frame_cnt;
    }
  }

  pal_fade_to(0);
}



//show a quick level clear splash before the next level

void show_level_clear(unsigned char num)
{
  scroll(-4,0); //match the level intro screen alignment
  ppu_off();
  oam_clear();

  vram_adr(NAMETABLE_A);
  vram_unrle(level_nam);

  // v58: show the true level number big on the CLEAR screen.
  put_big_level_num(NAMETABLE_A+0x0194,num+1);
  put_big_level_num_purple_attrs();

  //Move CLEAR down so it no longer sits inside the big LEVEL letters.
  vram_adr(NAMETABLE_A+0x022d);
  vram_write((unsigned char*)levelClearStr,5);

  //Show current score on the splash.
  vram_adr(NAMETABLE_A+0x026a);
  vram_write((unsigned char*)clearScoreStr,6);
  put_score_num(NAMETABLE_A+0x0270,game_score);

  if(num+1<LEVELS_ALL)
  {
    vram_adr(NAMETABLE_A+0x02ab);
    vram_write((unsigned char*)nextLevelStr,6);
  }

  //Small Retro Replay mark near the bottom, high enough to avoid clipping.
  vram_adr(NAMETABLE_A+0x0328);
  vram_write((unsigned char*)gameOverUrlStr,16);

  pal_col(2,0x21);
  pal_col(3,0x30);
  pal_col(6,0x30);
  pal_bg(palGame1);
  // v82: BG palette 2 used only for the big level number.
  pal_col(9,0x15);
  pal_col(10,0x25);
  pal_col(11,0x30);
  pal_spr(palGameSpr);
  ppu_on_all();

  pal_fade_to(4);
  music_play(MUSIC_CLEAR);

  //Quick parade during the same short wait: horn guy and slimes sprint by.
  frame_cnt=0;
  while(frame_cnt<96)
  {
    ppu_wait_frame();

    //Flash the blue text color a little so the score/splash feels alive.
    pal_col(2,(frame_cnt&8)?0x21:0x11);

    spr=0;
    i=(frame_cnt<<2);
    py=196+((frame_cnt&4)>>2);
    spr=oam_meta_spr(i,py,spr,sprPlayer);
    spr=oam_meta_spr(i-34,py+1,spr,sprEnemy1);
    spr=oam_meta_spr(i-60,py+2,spr,sprEnemy2);
    spr=oam_meta_spr(i-86,py+1,spr,sprEnemy3);
    oam_hide_rest(spr);

    ++frame_cnt;
  }

  pal_fade_to(0);
}



//show a short GET READY 3-2-1 countdown before gameplay starts
//Uses the same large number tile set as the level intro screen.
void show_ready_countdown(void)
{
  scroll(-4,0); //match the level intro alignment

  //Play a small count-in on a clean black screen so the message is readable.
  for(wait=3;wait>0;--wait)
  {
    ppu_off();
    oam_clear();

    vram_adr(NAMETABLE_A);
    vram_fill(0,1024);

    //GET READY!
    vram_adr(NAMETABLE_A+0x00ea);
    vram_write((unsigned char*)nextLevelStr,6);

    //large countdown number centered below it
    j=(wait-1)<<1;
    i16=NAMETABLE_A+0x014f;
    for(i=0;i<3;++i)
    {
      vram_adr(i16);
      vram_put(largeNums[j]);
      vram_put(largeNums[j+1]);
      j+=10;
      i16+=32;
    }

    //small brand mark at the bottom
    vram_adr(NAMETABLE_A+0x0328);
    vram_write((unsigned char*)gameOverUrlStr,16);

    pal_bg(palGame1);
    pal_spr(palGameSpr);
    pal_col(2,0x21);
    pal_col(3,0x30);
    ppu_on_all();
    pal_bright(4);

    sfx_play(SFX_START,0);
    delay(45);
  }

  //Tiny GO flash so the transition feels responsive, but keeps the countdown short.
  ppu_off();
  vram_adr(NAMETABLE_A);
  vram_fill(0,1024);
  vram_adr(NAMETABLE_A+0x01cd);
  vram_put(0x27); //G
  vram_put(0x2f); //O
  vram_put(0x01); //!
  vram_adr(NAMETABLE_A+0x0328);
  vram_write((unsigned char*)gameOverUrlStr,16);
  ppu_on_all();
  sfx_play(SFX_START,0);
  delay(20);

  pal_fade_to(0);
}


//set up a move in the specified direction if there is no wall

void player_move(unsigned char id,unsigned char dir)
{
  px=player_x[id]>>(TILE_SIZE_BIT+FP_BITS);
  py=player_y[id]>>(TILE_SIZE_BIT+FP_BITS);

  switch(dir)
  {
  case DIR_LEFT:  --px; break;
  case DIR_RIGHT: ++px; break;
  case DIR_UP:    --py; break;
  case DIR_DOWN:  ++py; break;
  }

  //v20 safety clamp: never allow the player/enemies to step outside
  //the 16 x 13 gameplay map, even if a level border tile is wrong.
  if(px>=MAP_WDT || py<2 || py>=MAP_HGT+2) return;

  if(map[MAP_ADR(px,py)]==TILE_WALL) return;

  player_cnt[id]=TILE_SIZE<<FP_BITS;
  player_dir[id]=dir;
}



//print a 1-3 digit decimal number into VRAM

void put_num(unsigned int adr,unsigned int num,unsigned char len)
{
  vram_adr(adr);

  if(len>2) vram_put(0x10+num/100);
  if(len>1) vram_put(0x10+num/10%10);
  vram_put(0x10+num%10);
}

//print a 4 digit score directly into VRAM during setup
void put_score_num(unsigned int adr,unsigned int num)
{
  vram_adr(adr);

  vram_put(0x10+num/1000);
  vram_put(0x10+num/100%10);
  vram_put(0x10+num/10%10);
  vram_put(0x10+num%10);
}

//refresh SCORE digits in the always-active VRAM update list
void refresh_score_hud(void)
{
  update_list[14]=0x10+game_score/1000;
  update_list[17]=0x10+game_score/100%10;
  update_list[20]=0x10+game_score/10%10;
  update_list[23]=0x10+game_score%10;
}

//refresh collected item count in the HUD
void refresh_items_hud(void)
{
  update_list[26]=0x10+items_collected/100;
  update_list[29]=0x10+items_collected/10%10;
  update_list[32]=0x10+items_collected%10;
}

//show active power-up and seconds remaining on the HUD
void refresh_power_hud(void)
{
  if(freeze_timer)
  {
    update_list[35]=0x26; //F
    update_list[38]=0x10+((freeze_timer+59)/60);
  }
  else if(lightning_timer)
  {
    update_list[35]=0x2c; //L
    update_list[38]=0x10+((lightning_timer+59)/60);
  }
  else if(skull_clear_timer)
  {
    update_list[35]=0x33; //S for Skull Clear
    update_list[38]=0x10+((skull_clear_timer+59)/60);
  }
  else if(held_power)
  {
    if(held_power==POWER_LIGHTNING) update_list[35]=0x2c; //L
    else if(held_power==POWER_SKULL_CLEAR) update_list[35]=0x33; //S
    else update_list[35]=0x26; //F
    update_list[38]=0x00;
  }
  else
  {
    update_list[35]=0x00;
    update_list[38]=0x00;
  }
}


//queue a 16x16 background tile update at a map position
void queue_meta_tile(unsigned int adr,unsigned char tile)
{
  update_list[0]=adr>>8;
  update_list[1]=adr&255;
  update_list[3]=update_list[0];
  update_list[4]=update_list[1]+1;
  adr+=32;
  update_list[6]=adr>>8;
  update_list[7]=adr&255;
  update_list[9]=update_list[6];
  update_list[10]=update_list[7]+1;

  if(tile==TILE_EMPTY)
  {
    update_list[2]=TILE_EMPTY;
    update_list[5]=TILE_EMPTY;
    update_list[8]=TILE_EMPTY;
    update_list[11]=TILE_EMPTY;
  }
  else
  {
    update_list[2]=tile;
    update_list[5]=tile+1;
    update_list[8]=tile+2;
    update_list[11]=tile+3;
  }
}

//convert a map index back into a nametable address
unsigned int map_index_to_nt(unsigned int idx)
{
  return NAMETABLE_A+0x0080+((idx>>MAP_WDT_BIT)<<6)+((idx&(MAP_WDT-1))<<1);
}

//true if a sprite Code Chip is already sitting on this map tile
unsigned char chip_at_index(unsigned int idx)
{
  px=(idx&(MAP_WDT-1))<<TILE_SIZE_BIT;
  py=((idx>>MAP_WDT_BIT)+2)<<TILE_SIZE_BIT;

  for(ptr=0;ptr<CHIP_MAX;++ptr)
  {
    if(chip_active[ptr]&&chip_x[ptr]==px&&chip_y[ptr]==py) return TRUE;
  }

  return FALSE;
}

//spawn a sprite Code Chip at a map tile. The map/background tile is forced
//back to empty so only the chip sprite changes color or glows.
void spawn_code_chip(unsigned int idx)
{
  // v146b: do not leave extra chip sprites once the objective is already satisfied.
  if(items_collected>=items_count) return;
  if(chip_at_index(idx)) return;

  for(ptr=0;ptr<CHIP_MAX;++ptr)
  {
    if(!chip_active[ptr])
    {
      map[idx]=TILE_EMPTY;
      queue_meta_tile(map_index_to_nt(idx),TILE_EMPTY);
      chip_active[ptr]=TRUE;
      chip_x[ptr]=(idx&(MAP_WDT-1))<<TILE_SIZE_BIT;
      chip_y[ptr]=((idx>>MAP_WDT_BIT)+2)<<TILE_SIZE_BIT;
      ++chips_spawned;
      return;
    }
  }
}


//true if the sprite Frozen Disk power-up is sitting on this map tile
unsigned char orb_at_index(unsigned int idx)
{
  px=(idx&(MAP_WDT-1))<<TILE_SIZE_BIT;
  py=((idx>>MAP_WDT_BIT)+2)<<TILE_SIZE_BIT;

  if(orb_active&&orb_x==px&&orb_y==py) return TRUE;

  return FALSE;
}

//spawn the Frozen Disk as a sprite pickup instead of a background tile.
//This prevents ugly solid black square backgrounds.
void spawn_orb(unsigned int idx)
{
  if(orb_active) return;

  orb_active=TRUE;
  orb_type=POWER_FREEZE;
  orb_x=(idx&(MAP_WDT-1))<<TILE_SIZE_BIT;
  orb_y=((idx>>MAP_WDT_BIT)+2)<<TILE_SIZE_BIT;
}

void spawn_lightning_orb(unsigned int idx)
{
  if(orb_active) return;

  orb_active=TRUE;
  orb_type=POWER_LIGHTNING;
  orb_x=(idx&(MAP_WDT-1))<<TILE_SIZE_BIT;
  orb_y=((idx>>MAP_WDT_BIT)+2)<<TILE_SIZE_BIT;
}

void spawn_skull_orb(unsigned int idx)
{
  if(orb_active) return;

  orb_active=TRUE;
  orb_type=POWER_SKULL_CLEAR;
  orb_x=(idx&(MAP_WDT-1))<<TILE_SIZE_BIT;
  orb_y=((idx>>MAP_WDT_BIT)+2)<<TILE_SIZE_BIT;
}


void add_score(unsigned int points);

unsigned char dist_close(unsigned char ax,unsigned char ay,unsigned char bx,unsigned char by,unsigned char range)
{
  if(ax>bx) px=ax-bx; else px=bx-ax;
  if(ay>by) py=ay-by; else py=by-ay;

  if(px<=range && py<=range) return TRUE;

  return FALSE;
}

unsigned char bomb_at_index(unsigned int idx)
{
  if(bomb_state!=BOMB_FLOOR) return FALSE;

  px=(idx&(MAP_WDT-1))<<TILE_SIZE_BIT;
  py=((idx>>MAP_WDT_BIT)+2)<<TILE_SIZE_BIT;

  if(bomb_x==px && bomb_y==py) return TRUE;

  return FALSE;
}

unsigned char bomb_near_player(void)
{
  if(bomb_state!=BOMB_FLOOR) return FALSE;

  if(dist_close((player_x[0]>>FP_BITS),(player_y[0]>>FP_BITS),
                bomb_x,bomb_y,BOMB_PICKUP_RANGE)) return TRUE;

  return FALSE;
}

void spawn_glitch_bomb(unsigned int idx)
{
  if(bomb_state||bomb_blast_timer) return;

  bomb_state=BOMB_FLOOR;
  bomb_x=(idx&(MAP_WDT-1))<<TILE_SIZE_BIT;
  bomb_y=((idx>>MAP_WDT_BIT)+2)<<TILE_SIZE_BIT;
  bomb_timer=BOMB_TIME;
}

void drop_carried_bomb(void)
{
  if(bomb_state!=BOMB_HELD) return;

  bomb_state=BOMB_FLOOR;
  bomb_x=player_x[0]>>FP_BITS;
  bomb_y=player_y[0]>>FP_BITS;
  sfx_play(SFX_ITEM,1);
}



void bomb_break_single_wall(unsigned char tx,unsigned char ty)
{
  // v148: break only exact bomb-gate cells.
  // Stage 11: one gate at x9,y5.
  // Stage 12: two gates at x5,y6 and x10,y8.
  // Stage 13: one dead-end gate at x9,y6.
  // Stage 14: two side-room gates at x3,y6 and x12,y6.
  if((game_level%LEVEL_LAYOUTS)==10)
  {
    if(tx!=9) return;
    if(ty!=5) return;
  }
  else if((game_level%LEVEL_LAYOUTS)==11)
  {
    if(!((tx==5 && ty==6) || (tx==10 && ty==8))) return;
  }
  else if((game_level%LEVEL_LAYOUTS)==12)
  {
    if(!(tx==9 && ty==6)) return;
  }
  else if((game_level%LEVEL_LAYOUTS)==13)
  {
    if(!((tx==4 && ty==6) || (tx==11 && ty==6))) return;
  }
  else return;

  ptr=ty*MAP_WDT+tx;
  if(map[ptr]!=TILE_WALL) return;

  map[ptr]=TILE_EMPTY;

  // Clear one 2x2 wall metatile through the normal update list.
  i=(ty+2)<<1;
  j=tx<<1;

  update_list[ 0]=MSB(NAMETABLE_A+(i<<5)+j);
  update_list[ 1]=LSB(NAMETABLE_A+(i<<5)+j);
  update_list[ 2]=TILE_EMPTY;

  update_list[ 3]=MSB(NAMETABLE_A+(i<<5)+j+1);
  update_list[ 4]=LSB(NAMETABLE_A+(i<<5)+j+1);
  update_list[ 5]=TILE_EMPTY;

  update_list[ 6]=MSB(NAMETABLE_A+((i+1)<<5)+j);
  update_list[ 7]=LSB(NAMETABLE_A+((i+1)<<5)+j);
  update_list[ 8]=TILE_EMPTY;

  update_list[ 9]=MSB(NAMETABLE_A+((i+1)<<5)+j+1);
  update_list[10]=LSB(NAMETABLE_A+((i+1)<<5)+j+1);
  update_list[11]=TILE_EMPTY;
}

void bomb_break_nearby_walls(void)
{
  // v148: no real radius wall destruction.
  // If the bomb explodes near a gate, open only that exact gate cell.
  px=bomb_blast_x>>TILE_SIZE_BIT;
  py=(bomb_blast_y>>TILE_SIZE_BIT)-2;

  if((game_level%LEVEL_LAYOUTS)==10)
  {
    if(px>=8 && px<=10 && py>=4 && py<=6) bomb_break_single_wall(9,5);
  }
  else if((game_level%LEVEL_LAYOUTS)==11)
  {
    if(px>=4 && px<=6 && py>=5 && py<=7) bomb_break_single_wall(5,6);
    if(px>=9 && px<=11 && py>=7 && py<=9) bomb_break_single_wall(10,8);
  }
  else if((game_level%LEVEL_LAYOUTS)==12)
  {
    if(px>=7 && px<=10 && py>=4 && py<=8) bomb_break_single_wall(9,6);
  }
  else if((game_level%LEVEL_LAYOUTS)==13)
  {
    if(px>=3 && px<=5 && py>=5 && py<=7) bomb_break_single_wall(4,6);
    if(px>=10 && px<=12 && py>=5 && py<=7) bomb_break_single_wall(11,6);
  }
}



void explode_glitch_bomb(void)
{
  bomb_blast_x=bomb_x;
  bomb_blast_y=bomb_y;
  bomb_blast_timer=BOMB_BLAST_TIME;
  bomb_state=BOMB_NONE;
  bomb_timer=0;

  sfx_play(SFX_RESPAWN2,1);

  // v120: no radius wall breaking. Only the single Stage 11 gate can open.
  bomb_break_nearby_walls();

  //Clear nearby slimes. They are sent back to spawn with a short respawn delay.
  for(ptr=1;ptr<player_all;++ptr)
  {
    if(!player_wait[ptr])
    {
      if(dist_close((player_x[ptr]>>FP_BITS),(player_y[ptr]>>FP_BITS),
                    bomb_blast_x,bomb_blast_y,BOMB_RADIUS))
      {
        add_score(SCORE_ENEMY);
        player_x[ptr]=enemy_spawn_x[ptr];
        player_y[ptr]=enemy_spawn_y[ptr];
        player_cnt[ptr]=0;
        player_dir[ptr]=DIR_NONE;
        player_wait[ptr]=90;
      }
    }
  }
}

void update_glitch_bomb(void)
{
  if(bomb_state==BOMB_HELD)
  {
    bomb_x=player_x[0]>>FP_BITS;
    bomb_y=player_y[0]>>FP_BITS;
  }

  if(bomb_state)
  {
    if(bomb_timer) --bomb_timer;
    if(!bomb_timer) explode_glitch_bomb();
  }

  if(bomb_blast_timer) --bomb_blast_timer;
}

void activate_skull_clear(void)
{
  skull_clear_timer=SKULL_CLEAR_TIME;
  skull_flash_timer=24;

  //Banish every slime off-screen for the whole clear window, then let
  //the normal spawn animation bring them back. This makes the board
  //visibly empty for about 3 seconds without touching the map logic.
  for(ptr=1;ptr<player_all;++ptr)
  {
    player_x[ptr]=enemy_spawn_x[ptr];
    player_y[ptr]=enemy_spawn_y[ptr];
    player_cnt[ptr]=0;
    player_dir[ptr]=DIR_NONE;
    player_wait[ptr]=SKULL_CLEAR_TIME;
  }

  //Layer two existing effects together for a stronger laser/blast hit.
  sfx_play(SFX_START,0);
  sfx_play(SFX_RESPAWN2,1);
}

//start one small Frozen Disk shot in the player's current direction.
//Returns TRUE only when a shot slot was actually created.
unsigned char spawn_shot(void)
{
  j=player_dir[0];
  if(!j) j=DIR_RIGHT;

  for(ptr=0;ptr<SHOT_MAX;++ptr)
  {
    if(!shot_active[ptr])
    {
      shot_active[ptr]=TRUE;
      shot_x[ptr]=(player_x[0]>>FP_BITS)+4;
      shot_y[ptr]=(player_y[0]>>FP_BITS)+4;
      shot_dir[ptr]=j;
      return TRUE;
    }
  }

  return FALSE;
}

//move Frozen Disk shots and remove them when they hit a wall or leave the map
void update_shots(void)
{
  for(ptr=0;ptr<SHOT_MAX;++ptr)
  {
    if(shot_active[ptr])
    {
      //Do not allow unsigned wraparound. When a shot leaves the playfield,
      //it disappears instead of reappearing on the opposite side.
      switch(shot_dir[ptr])
      {
      case DIR_RIGHT:
        if(shot_x[ptr]>=(MAP_WDT*TILE_SIZE-8)) { shot_active[ptr]=FALSE; break; }
        shot_x[ptr]+=SHOT_SPEED;
        break;
      case DIR_LEFT:
        if(shot_x[ptr]<SHOT_SPEED) { shot_active[ptr]=FALSE; break; }
        shot_x[ptr]-=SHOT_SPEED;
        break;
      case DIR_DOWN:
        if(shot_y[ptr]>=((MAP_HGT+2)*TILE_SIZE-8)) { shot_active[ptr]=FALSE; break; }
        shot_y[ptr]+=SHOT_SPEED;
        break;
      case DIR_UP:
        if(shot_y[ptr]<(2*TILE_SIZE+SHOT_SPEED)) { shot_active[ptr]=FALSE; break; }
        shot_y[ptr]-=SHOT_SPEED;
        break;
      }

      if(shot_active[ptr])
      {
        px=shot_x[ptr]>>TILE_SIZE_BIT;
        py=(shot_y[ptr]>>TILE_SIZE_BIT)-2;

        if(px>=MAP_WDT||py>=MAP_HGT||map[MAP_ADR(px,py)]==TILE_WALL)
        {
          shot_active[ptr]=FALSE;
        }
      }
    }
  }
}

//add score and refresh SCORE digits in the VRAM update list
void add_score(unsigned int points)
{
  game_score+=points;
  if(game_score>9999) game_score=9999;
  if(game_score>high_score) high_score=game_score;
  refresh_score_hud();
}


//Draw a short tile-font string as sprites.
//This lets the pause screen appear over the frozen game without touching VRAM.
unsigned char draw_oam_text(unsigned char x,unsigned char y,const unsigned char* text,unsigned char len,unsigned char attr,unsigned char sprid)
{
  for(ptr=0;ptr<len;++ptr)
  {
    if(text[ptr]) sprid=oam_spr(x,y,text[ptr],attr,sprid);
    x+=8;
  }
  return sprid;
}




// v117: Stage 11 patching is done in code after loading a known-good Level 1 clone.
// This avoids corrupt custom nametable exports while still giving Stage 11 a unique layout.
void level11_put_meta(unsigned char mx,unsigned char my,unsigned char kind)
{
  // Convert collision-map cell to nametable address.
  // Map row 0 begins at nametable row 4, and each map cell is 2x2 tiles.
  i16=NAMETABLE_A+((my+2)<<6)+(mx<<1);

  // v156: keep the collision/object map in sync while runtime-patching.
  // Without this, Stage 13 could start with zero counted chips on first load.
  ptr=my*MAP_WDT+mx;
  if(kind==1 || kind==7) map[ptr]=TILE_WALL;
  else if(kind==2) map[ptr]=TILE_ITEM;
  else if(kind==3) map[ptr]=TILE_PLAYER;
  else if(kind==4) map[ptr]=TILE_ENEMY1;
  else if(kind==5) map[ptr]=TILE_ENEMY2;
  else if(kind==6) map[ptr]=TILE_ENEMY3;
  else map[ptr]=TILE_EMPTY;

  vram_adr(i16);

  switch(kind)
  {
  case 8: // v156 black void outside shaped maps
    vram_put(0x00);
    vram_put(0x00);
    vram_adr(i16+32);
    vram_put(0x00);
    vram_put(0x00);
    break;

  case 1: // wall
    vram_put(TILE_WALL);
    vram_put(TILE_WALL+1);
    vram_adr(i16+32);
    vram_put(TILE_WALL+2);
    vram_put(TILE_WALL+3);
    break;

  case 7: // v132 grey bomb gate
    // Same exact wall block art as the normal walls.
    // It is identified by a grey/white background palette quadrant.
    vram_put(TILE_WALL);
    vram_put(TILE_WALL+1);
    vram_adr(i16+32);
    vram_put(TILE_WALL+2);
    vram_put(TILE_WALL+3);
    break;

  case 2: // chip/item
    vram_put(TILE_ITEM);
    vram_put(TILE_ITEM+1);
    vram_adr(i16+32);
    vram_put(TILE_ITEM+2);
    vram_put(TILE_ITEM+3);
    break;

  case 3: // player
    vram_put(TILE_PLAYER);
    vram_put(TILE_EMPTY);
    vram_adr(i16+32);
    vram_put(TILE_EMPTY);
    vram_put(TILE_EMPTY);
    break;

  case 4: // enemy 1
    vram_put(TILE_ENEMY1);
    vram_put(TILE_EMPTY);
    vram_adr(i16+32);
    vram_put(TILE_EMPTY);
    vram_put(TILE_EMPTY);
    break;

  case 5: // enemy 2
    vram_put(TILE_ENEMY2);
    vram_put(TILE_EMPTY);
    vram_adr(i16+32);
    vram_put(TILE_EMPTY);
    vram_put(TILE_EMPTY);
    break;

  case 6: // enemy 3
    vram_put(TILE_ENEMY3);
    vram_put(TILE_EMPTY);
    vram_adr(i16+32);
    vram_put(TILE_EMPTY);
    vram_put(TILE_EMPTY);
    break;

  default: // empty floor
    vram_put(TILE_EMPTY);
    vram_put(TILE_EMPTY);
    vram_adr(i16+32);
    vram_put(TILE_EMPTY);
    vram_put(TILE_EMPTY);
    break;
  }
}


void level11_fix_attrs_v124(void)
{
  // v158: compact attr cleanup. One loop replaces the old repeated vram_put block.
  vram_adr(NAMETABLE_A+0x03c0);
  for(i=0;i<64;++i) vram_put(0x55);

  // v126: only the gate quadrant uses palette 3; surrounding area stays palette 1.
  vram_adr(NAMETABLE_A+0x03c0+3*8+4);
  vram_put(0xd5);
}


void patch_level11_bomb_gate(void)
{
  // Clear only the same small footprint used by the old small levels.
  // Do not touch the HUD/top rows.
  for(py=2;py<9;++py)
  {
    for(px=0;px<16;++px)
    {
      level11_put_meta(px,py,0);
    }
  }

  // Outer wall of the small level.
  for(px=0;px<16;++px)
  {
    level11_put_meta(px,2,1);
    level11_put_meta(px,8,1);
  }

  for(py=2;py<9;++py)
  {
    level11_put_meta(0,py,1);
    level11_put_meta(15,py,1);
  }

  // Main divider. The red/discolored gate is the one block the bomb should open.
  for(py=3;py<8;++py)
  {
    level11_put_meta(9,py,1);
  }

  level11_put_meta(9,5,7); // special bomb gate

  // v142: fill Stage 11 with normal collectible chips/items like the regular maze stages.
  // Leave only spawn spots, enemies, walls, gate, and the starter bomb spot empty.
  for(py=3;py<8;++py)
  {
    for(px=1;px<15;++px)
    {
      if(px==9) continue;          // divider wall / grey bomb gate column
      if(px==1 && py==3) continue; // player spawn
      if(px==5 && py==7) continue; // slime 1 spawn
      if(px==6 && py==7) continue; // slime 2 spawn
      if(px==4 && py==6) continue; // starter bomb spawn
      level11_put_meta(px,py,2);
    }
  }

  // Player and enemies.
  // v122: Stage 11 only uses 2 slimes while testing the bomb gate.
  level11_put_meta(1,3,3);
  level11_put_meta(5,7,4);
  level11_put_meta(6,7,5);

  // v118: gate marker is now a sprite overlay, not a background attribute.
  level11_fix_attrs_v124();
}


// v147c: one safe big runtime-patched layout for Stage 12.
// This is intentionally small-code and tested-style, unlike the broken v147/v147b 12-20 patcher.
void patch_level12_big_safe(void)
{
  // v148: cleaner Stage 12 layout with two bomb-gated chip rooms.
  // Starts lower than the HUD and avoids the full-screen color-band weirdness.

  // Clear gameplay map area first.
  for(py=0;py<13;++py)
  {
    for(px=0;px<16;++px) level11_put_meta(px,py,0);
  }

  // Outer border, slightly lower/safe: rows 1-11.
  for(px=0;px<16;++px)
  {
    level11_put_meta(px,1,1);
    level11_put_meta(px,11,1);
  }

  for(py=1;py<12;++py)
  {
    level11_put_meta(0,py,1);
    level11_put_meta(15,py,1);
  }

  // Two vertical walls that create left, center, and right zones.
  // Each wall has one bomb gate.
  // v150: gates moved down one cell so the grey marker matches the actual break cell.
  for(py=2;py<11;++py)
  {
    if(py!=6) level11_put_meta(5,py,1);
    if(py!=8) level11_put_meta(10,py,1);
  }

  // v153: Level-11-style bomb gates: normal wall art, grey/white BG palette quadrant.
  level11_put_meta(5,6,7);
  level11_put_meta(10,8,7);

  // Fill accessible and gated areas with normal collectible chips.
  // Leave player, enemy, and bomb spawn cells empty.
  for(py=2;py<11;++py)
  {
    for(px=1;px<15;++px)
    {
      if(px==5 || px==10) continue;   // walls/gates
      if(px==1 && py==2) continue;    // player
      if(px==3 && py==8) continue;    // starter bomb
      if(px==8 && py==5) continue;    // future bomb/drop space
      if(px==13 && py==2) continue;   // enemy 1
      if(px==13 && py==10) continue;  // enemy 2
      if(px==3 && py==9) continue;    // enemy 3 in first bay
      level11_put_meta(px,py,2);
    }
  }

  // Re-draw walls/gates after chip fill.
  for(py=2;py<11;++py)
  {
    if(py!=6) level11_put_meta(5,py,1);
    if(py!=8) level11_put_meta(10,py,1);
  }
  level11_put_meta(5,6,7);
  level11_put_meta(10,8,7);

  // Player and enemies.
  level11_put_meta(1,2,3);
  level11_put_meta(13,2,4);
  level11_put_meta(13,10,5);
  level11_put_meta(3,9,6); // v149: slime inside first bay so bombs can happen there

  // Stronger full attribute cleanup: all visible gameplay attributes use palette 1.
  // Then only gate quadrants use palette 3 for grey/white gate blocks.
  vram_adr(NAMETABLE_A+0x03c0);
  for(i=0;i<64;++i) vram_put(0x55);

  // v153: Level-11-style grey gate attributes.
  // Normal playfield uses palette 1 (0x55). Only the exact 16x16 gate quadrant is switched to palette 3.
  // Gate 1 at map x5,y6 -> nametable tiles x10-11,y16-17 -> attr row 4,col 2, top-right quadrant.
  vram_adr(NAMETABLE_A+0x03c0+4*8+2);
  vram_put(0x5d);

  // Gate 2 at map x10,y8 -> nametable tiles x20-21,y20-21 -> attr row 5,col 5, top-left quadrant.
  vram_adr(NAMETABLE_A+0x03c0+5*8+5);
  vram_put(0x57);
}



// v154b: compact Stage 13. Dead-end passage with one bomb-gated wall.
void patch_level13_dead_end_gate(void)
{
  // v156: black void outside, stable map sync, exact grey gate.
  // Clear whole gameplay patch area to black/blank first.
  for(py=0;py<13;++py)
  {
    for(px=0;px<16;++px) level11_put_meta(px,py,8);
  }

  // Build a 3/4-screen arena, leaving outside as black void.
  for(py=2;py<12;++py)
  {
    for(px=1;px<15;++px) level11_put_meta(px,py,0);
  }

  // Arena border.
  for(px=1;px<15;++px)
  {
    level11_put_meta(px,2,1);
    level11_put_meta(px,11,1);
  }
  for(py=2;py<12;++py)
  {
    level11_put_meta(1,py,1);
    level11_put_meta(14,py,1);
  }

  // Dead-end wall and gate into the other side.
  for(py=3;py<11;++py) level11_put_meta(9,py,1);
  level11_put_meta(9,6,7); // visible grey gate, exact break target

  // Small interior blocks.
  for(px=3;px<9;++px) level11_put_meta(px,5,1);
  level11_put_meta(5,5,0);
  level11_put_meta(7,5,0);
  level11_put_meta(4,9,1);
  level11_put_meta(5,9,1);
  level11_put_meta(12,4,1);
  level11_put_meta(12,9,1);

  // Fill chips after map has been correctly updated by level11_put_meta().
  for(py=3;py<11;++py)
  {
    for(px=2;px<14;++px)
    {
      if(px==2 && py==3) continue;   // player
      if(px==3 && py==8) continue;   // slime
      if(px==4 && py==8) continue;   // starter bomb
      if(px==12 && py==5) continue;  // slime
      if(px==12 && py==10) continue; // slime
      ptr=py*MAP_WDT+px;
      if(map[ptr]==TILE_EMPTY) level11_put_meta(px,py,2);
    }
  }

  // Spawns.
  level11_put_meta(2,3,3);
  level11_put_meta(3,8,4);
  level11_put_meta(12,5,5);
  level11_put_meta(12,10,6);

  // Clean attributes: mostly one stable palette, with only the exact gate quadrant grey/white.
  vram_adr(NAMETABLE_A+0x03c0);
  for(i=0;i<64;++i) vram_put(0x55);

  // Gate at map x9,y6 -> nametable x18-19,y16-17 -> attr row 4, col 4, top-right quadrant.
  vram_adr(NAMETABLE_A+0x03c0+4*8+4);
  vram_put(0x5d);
}


// v163: Stage 14 side-room bomb test, lean.
// Player must be the first spawn found during VRAM scan, so he stays index 0.
void patch_level14_side_rooms(void)
{
  for(py=0;py<13;++py)
  {
    for(px=0;px<16;++px) level11_put_meta(px,py,0);
  }

  for(px=0;px<16;++px)
  {
    level11_put_meta(px,1,1);
    level11_put_meta(px,11,1);
  }

  for(py=1;py<12;++py)
  {
    level11_put_meta(0,py,1);
    level11_put_meta(15,py,1);
    if(py!=6)
    {
      level11_put_meta(4,py,1);
      level11_put_meta(11,py,1);
    }
  }

  level11_put_meta(4,6,7);
  level11_put_meta(11,6,7);

  level11_put_meta(7,4,1);
  level11_put_meta(8,4,1);
  level11_put_meta(7,8,1);
  level11_put_meta(8,8,1);
  level11_put_meta(6,6,1);
  level11_put_meta(9,6,1);

  // Main arena chips.
  for(px=5;px<11;++px)
  {
    if(px!=7 && px!=8)
    {
      level11_put_meta(px,3,2);
      level11_put_meta(px,9,2);
    }
  }
  for(py=5;py<8;++py)
  {
    level11_put_meta(5,py,2);
    level11_put_meta(10,py,2);
    level11_put_meta(13,py,2);
  }

  // v164: pack the whole starting chamber with chips, but leave player/slime/bomb cells open.
  for(py=2;py<11;++py)
  {
    for(px=1;px<4;++px)
    {
      if((px==2 && py==3)||(px==3 && py==6)||(px==3 && py==8)) continue;
      level11_put_meta(px,py,2);
    }
  }

  level11_put_meta(2,3,3);   // player: first spawn in scan order
  level11_put_meta(3,8,4);   // slime in same starting chamber
  level11_put_meta(13,9,5);
  level11_put_meta(8,10,6);

  vram_adr(NAMETABLE_A+0x03c0);
  for(i=0;i<64;++i) vram_put(0x55);
  vram_adr(NAMETABLE_A+0x03c0+4*8+2);
  vram_put(0x57);
  vram_adr(NAMETABLE_A+0x03c0+4*8+5);
  vram_put(0x5d);
}

//the main gameplay code

void game_loop(void)
{
  oam_clear();

  // v158: 50 stages use 14 layouts in rotation.
  // Stage 14 is runtime-patched from the safe bomb-gate base to avoid PRG overflow.
  i=(game_level%LEVEL_LAYOUTS)<<1;

  vram_adr(NAMETABLE_A);
  vram_unrle(levelList[i]);          //unpack level nametable

  // v121: Stage 11 patch uses temp variables internally, so preserve the levelList index.
  // Losing this value corrupts the palette selection and causes the weird colors after retries.
  level_patch_save_i=i;
  j=game_level%LEVEL_LAYOUTS;
  if(j==10) patch_level11_bomb_gate();
  else if(j==11) patch_level12_big_safe();
  else if(j==12) patch_level13_dead_end_gate();
  else if(j==13) patch_level14_side_rooms(); // v159: real compact Stage 14 side rooms
  i=level_patch_save_i;

  vram_adr(NAMETABLE_A+0x0042);
  vram_write((unsigned char*)statsStr,27);   //add HUD line 1
  vram_adr(NAMETABLE_A+0x0062);
  vram_write((unsigned char*)statsStr2,27);  //add HUD line 2

  pal_bg(levelList[i+1]);             //set up background palette
  // v126: Stage 11 uses BG palette 3 as a grey/white marker for the bomb gate.
  j=game_level%LEVEL_LAYOUTS;if(j>=10&&j<=13)
  {
    pal_col(13,0x00);
    pal_col(14,0x10);
    pal_col(15,0x30);
  }
  pal_spr(palGameSpr);               //set up sprites palette

  player_all=0;
  items_count=0;
  items_collected=0;
  carts_count=0;
  chips_required=0;
  chips_spawned=0;
  for(i=0;i<CHIP_MAX;++i) chip_active[i]=FALSE;
  for(i=0;i<SHOT_MAX;++i) shot_active[i]=FALSE;
  orb_spawned=0;
  lightning_spawned=0;
  skull_spawned=0;
  orb_active=FALSE;
  orb_type=0;
  freeze_timer=0;
  freeze_chime_timer=0;
  lightning_timer=0;
  lightning_shot_timer=0;
  skull_clear_timer=0;
  skull_flash_timer=0;
  held_power=0;
  dash_boost_timer=0;
  dash_boost_tiles=0;
  bomb_state=BOMB_NONE;
  bomb_timer=0;
  bomb_blast_timer=0;
  bomb_x=0;
  bomb_y=0;
  bomb_blast_x=0;
  bomb_blast_y=0;

  //this loop reads the level nametable back from VRAM, row by row,
  //constructs game map, removes spawn points from the nametable,
  //and writes back to the VRAM

  i16=NAMETABLE_A+0x0080;
  ptr=0;
  wait=0;

  for(i=2;i<MAP_HGT+2;++i)
  {
    vram_adr(i16);
    vram_read(nameRow,32);
    vram_adr(i16);

    for(j=0;j<MAP_WDT<<1;j+=2)
    {
      spr=nameRow[j];

      switch(spr)
      {
      case TILE_PLAYER://player
      case TILE_ENEMY1://enemies
      case TILE_ENEMY2:
      case TILE_ENEMY3:
      case TILE_ENEMY4:
        player_dir  [player_all]=DIR_NONE;
        player_x    [player_all]=(j<<3)<<FP_BITS;
        player_y    [player_all]=(i<<4)<<FP_BITS;
        enemy_spawn_x[player_all]=player_x[player_all];
        enemy_spawn_y[player_all]=player_y[player_all];
        player_cnt  [player_all]=0;
        player_wait [player_all]=16+((spr-TILE_PLAYER)<<4);
        player_kind [player_all]=spr-TILE_PLAYER;
        player_speed[player_all]=(spr==TILE_PLAYER)?2<<FP_BITS:10+((spr-TILE_ENEMY1)<<1);
        // v78: gentler enemy speed ramp.
        // Stages 01-10: normal
        // Stages 11-20: +1
        // Stages 21-30: +2
        // Stages 31-40: +3
        // Stages 41-50: +4
        if(spr!=TILE_PLAYER) player_speed[player_all]+=(game_level/10);
        if(spr==TILE_ENEMY4) player_speed[player_all]+=2; // Nightmare is tougher, but no longer insane.
        ++player_all;
        wait+=16;
        spr=TILE_EMPTY;
        break;

      case TILE_ITEM:
        ++items_count;
        break;
      }

      map[ptr++]=spr;

      vram_put(spr);
      vram_put(nameRow[j+1]);
    }

    i16+=64;
  }

  //Pixel Panic: after all starting cartridges are recovered, enemies
  //drop required Code Chips. These are added to the objective total now,
  //but they are spawned during play.
  carts_count=items_count;
  // v43: cap chip requirements so the 50-stage loop stays possible.
  // It cycles 3-8 required Code Chips instead of growing past CHIP_MAX.
  chips_required=CODE_CHIP_BASE+(game_level%6);
  items_count+=chips_required;

  // v117: start Stage 11 with one bomb so the bomb-gate chip path can be tested.
  if((game_level%LEVEL_LAYOUTS)==10) spawn_glitch_bomb(MAP_ADR(4,6));
  if((game_level%LEVEL_LAYOUTS)==11) spawn_glitch_bomb(MAP_ADR(3,8));
  if((game_level%LEVEL_LAYOUTS)==12) spawn_glitch_bomb(MAP_ADR(4,8));
  if((game_level%LEVEL_LAYOUTS)==13) spawn_glitch_bomb(MAP_ADR(3,6));

  // v78: Add a 4th late-game Nightmare Slime, but not too early.
  // It now starts from Stage 31 onward instead of Stage 21, and it is slower.
  if(game_level>=30 && player_all<PLAYER_MAX && player_all>1)
  {
    player_dir  [player_all]=DIR_NONE;
    player_x    [player_all]=enemy_spawn_x[1];
    player_y    [player_all]=enemy_spawn_y[1];
    enemy_spawn_x[player_all]=player_x[player_all];
    enemy_spawn_y[player_all]=player_y[player_all];
    player_cnt  [player_all]=0;
    player_wait [player_all]=96;
    player_kind [player_all]=4;
    player_speed[player_all]=14+(game_level/10);
    ++player_all;
  }

  //setup update list

  memcpy(update_list,updateListData,sizeof(updateListData));

  set_vram_update(update_list);

  //put constant game stats numbers, that aren't updated during level

  put_num(NAMETABLE_A+0x0045,game_level+1,2);
  put_score_num(NAMETABLE_A+0x004d,game_score);
  put_num(NAMETABLE_A+0x0055,game_lives-1,1);
  put_num(NAMETABLE_A+0x0068,items_collected,3);
  put_num(NAMETABLE_A+0x006c,items_count,3);
  refresh_score_hud();
  refresh_items_hud();
  refresh_power_hud();
  //enable display

  ppu_on_all();

  game_done=FALSE;
  game_paused=FALSE;
  game_clear=FALSE;

  bright=0;
  frame_cnt=0;

  while(!game_done)
  {
    //construct OAM from object parameters

    if(game_paused)
    {
      // Pause overlay: hide gameplay sprites to keep the message clean and avoid OAM overflow.
      spr=0;

      // v80: Power-up legend. Uses existing metasprites only;
      // no new tiles and no CHR movement.
      // OAM budget: 4 icons = 16 sprites, text = 46 sprites, total = 62.
      spr=draw_oam_text(104,32,pauseStr,6,0,spr);
      spr=draw_oam_text(88,56,pauseLegendStr,5,0,spr);

      spr=oam_meta_spr(64,80,spr,sprOrb);
      spr=draw_oam_text(88,84,pauseFreezeStr,3,0,spr);

      spr=oam_meta_spr(64,104,spr,sprLightningOrb);
      spr=draw_oam_text(88,108,pauseZapStr,3,0,spr);

      spr=oam_meta_spr(64,128,spr,sprSkullOrb);
      spr=draw_oam_text(88,132,levelClearStr,5,0,spr);

      spr=oam_meta_spr(64,152,spr,sprCodeChip);
      spr=draw_oam_text(88,156,pauseChipStr,4,0,spr);

      spr=draw_oam_text(80,188,pauseUseDashStr,7,0,spr);

      oam_hide_rest(spr);
    }
    else
    {
      // v145: draw bomb first in OAM. Bottom-shadow bomb with red fuse flash; both frames mirrored in both sprite banks.
      spr=0;
      if(bomb_state)
      {
        px=bomb_x;
        py=bomb_y;
        if(bomb_state==BOMB_HELD)
        {
          px=player_x[0]>>FP_BITS;
          py=player_y[0]>>FP_BITS;
          if(py>12) py-=12;
        }

        if(bomb_timer&16) spr=oam_meta_spr(px,py,spr,sprBombRed);
        else              spr=oam_meta_spr(px,py,spr,sprBomb);
      }

      // Draw player/enemies after the bomb. Reserve 16 bytes when bomb is visible.
      spr=(player_all-1)<<4;
      if(bomb_state) spr+=16;

      for(i=0;i<player_all;++i)
      {
        py=player_y[i]>>FP_BITS;

        if(player_wait[i])
        {
          if(player_wait[i]>=16||player_wait[i]&2) py=240;
        }
        else if(!i&&((frame_cnt&24)==24))
        {
          --py;
        }
        else if(i&&((frame_cnt&16)==0))
        {
          py-=2;
        }

        oam_meta_spr(player_x[i]>>FP_BITS,py,spr,sprListPlayer[player_kind[i]]);
        spr-=16;
      }

      spr=player_all<<4;
      if(bomb_state) spr+=16;

      for(ptr=0;ptr<SHOT_MAX;++ptr)
      {
        if(shot_active[ptr]) spr=oam_spr(shot_x[ptr],shot_y[ptr],SHOT_TILE,0,spr);
      }

      for(ptr=0;ptr<CHIP_MAX;++ptr)
      {
        if(chip_active[ptr])
        {
          spr=oam_meta_spr(chip_x[ptr],chip_y[ptr]-((frame_cnt>>3)&1),spr,sprCodeChip);
        }
      }

      if(orb_active)
      {
        if(orb_type==POWER_LIGHTNING)        spr=oam_meta_spr(orb_x,orb_y,spr,sprLightningOrb);
        else if(orb_type==POWER_SKULL_CLEAR) spr=oam_meta_spr(orb_x,orb_y,spr,sprSkullOrb);
        else                                 spr=oam_meta_spr(orb_x,orb_y,spr,sprOrb);
      }

      if(bomb_blast_timer)
      {
        px=bomb_blast_x+4;
        py=bomb_blast_y+4;
        spr=oam_spr(px,py,SHOT_TILE,0,spr);
        spr=oam_spr(px+8,py,SHOT_TILE,0,spr);
        spr=oam_spr(px-8,py,SHOT_TILE,0,spr);
        spr=oam_spr(px,py+8,SHOT_TILE,0,spr);
        spr=oam_spr(px,py-8,SHOT_TILE,0,spr);
      }

      oam_hide_rest(spr);
    }

    //wait for next frame
    //it is here and not at beginning of the loop because you need
    //to update OAM for the very first frame, and you also need to do that
    //right after object parameters were changed, so either OAM update should
    //be in a function that called before the loop and at the end of the loop,
    //or wait for NMI should be placed there
    //otherwise you would have situation update-wait-display, i.e.
    //one frame delay between action and display of its result

    ppu_wait_frame();

    ++frame_cnt;

    //Keep pause bright/readable. v40 dimmed the whole screen, which made
    //the pause text hard to read over the maze. v40b instead blacks out
    //the background palette while paused and leaves sprite text bright.

    if(!(frame_cnt&3))
    {
      if(bright<4)
      {
        ++bright;
        pal_bright(bright);
      }
    }

    //poll the gamepad in the trigger mode

    i=pad_trigger(0);

    //it start was released and then pressed, toggle pause mode

    if(i&PAD_START)
    {
      game_paused^=TRUE;
      music_pause(game_paused);

      if(game_paused)
      {
        //Black background + bright sprite text = readable pause screen.
        pal_bg(palPauseBg);
        pal_spr(palPauseSpr);
        bright=4;
        pal_bright(4);
      }
      else
      {
        //Restore the current level palettes when returning to play.
        pal_bg(levelList[((game_level%LEVEL_LAYOUTS)<<1)+1]);
        j=game_level%LEVEL_LAYOUTS;if(j>=10&&j<=13)
        {
          pal_col(13,0x00);
          pal_col(14,0x10);
          pal_col(15,0x30);
        }
        pal_spr(palGameSpr);
        bright=4;
        pal_bright(4);
      }
    }

    //don't process anything in pause mode, just display latest game state

    if(game_paused) continue;

    // v59 controls:
    // A activates a held disk power-up. Disks are collected first, then used when needed.
    if((i&PAD_A)&&held_power&&!freeze_timer&&!lightning_timer&&!skull_clear_timer)
    {
      if(held_power==POWER_LIGHTNING)
      {
        lightning_timer=LIGHTNING_TIME;
        lightning_shot_timer=1;
      }
      else if(held_power==POWER_SKULL_CLEAR)
      {
        activate_skull_clear();
      }
      else
      {
        freeze_timer=FREEZE_DISK_TIME;
        freeze_chime_timer=1;
      }

      held_power=0;
      refresh_power_hud();
      if(!skull_clear_timer) sfx_play(SFX_START,0);
    }

    // B is now context-sensitive in this testing build.
    // Near a Glitch Bomb: pick it up. Carrying one: drop it.
    // Otherwise, B keeps the normal short dash.
    if(i&PAD_B)
    {
      if(bomb_state==BOMB_HELD)
      {
        drop_carried_bomb();
      }
      else if(bomb_near_player())
      {
        bomb_state=BOMB_HELD;
        sfx_play(SFX_ITEM,1);
      }
      else
      {
        dash_boost_timer=DASH_BOOST_TIME;
        dash_boost_tiles=DASH_BOOST_MAX_TILES;
        player_speed[0]=DASH_BOOST_SPEED;
        sfx_play(SFX_ITEM,1);
      }
    }

    if(dash_boost_timer && dash_boost_tiles)
    {
      --dash_boost_timer;
      player_speed[0]=DASH_BOOST_SPEED;
    }
    else
    {
      dash_boost_timer=0;
      dash_boost_tiles=0;
      player_speed[0]=2<<FP_BITS;
    }

    //CHR bank switching animation with different speed for background and sprites

    bank_bg((frame_cnt>>4)&1);
    bank_spr((frame_cnt>>3)&1); // bomb normal/red frames are mirrored in both sprite banks in v139

    //Lightning Disk auto-shoots in the horn guy's current direction.
    if(lightning_timer)
    {
      if(lightning_shot_timer) --lightning_shot_timer;
      if(!lightning_shot_timer)
      {
        if(spawn_shot()) sfx_play(SFX_ITEM,3);
        lightning_shot_timer=LIGHTNING_SHOT_RATE;
      }
      --lightning_timer;
    }

    update_shots();
    update_glitch_bomb();

    //Frozen Disk: enemies freeze in place, music keeps playing, slimes flash icy blue/white,
    //and a chime plays once per second until the timer runs out.
    if(freeze_timer)
    {
      if(freeze_chime_timer) --freeze_chime_timer;
      if(!freeze_chime_timer)
      {
        sfx_play(SFX_ITEM,3);
        freeze_chime_timer=FREEZE_CHIME_PERIOD;
      }

      if(frame_cnt&8)
      {
        pal_col(22,0x2c); pal_col(23,0x30);
        pal_col(26,0x2c); pal_col(27,0x30);
        pal_col(30,0x2c); pal_col(31,0x30);
      }
      else
      {
        pal_col(22,0x1c); pal_col(23,0x2c);
        pal_col(26,0x1c); pal_col(27,0x2c);
        pal_col(30,0x1c); pal_col(31,0x2c);
      }

      --freeze_timer;
      if(!freeze_timer)
      {
        pal_spr(palGameSpr);
        freeze_chime_timer=0;
        // v94: freeze ends without touching music.
      }
    }



    if(skull_clear_timer)
    {
      --skull_clear_timer;
    }

    //Skull Clear polish: fast white flash right after activation.
    //The flash is intentionally short so it feels like a laser blast
    //without hiding the game for the full 3-second clear window.
    if(skull_flash_timer)
    {
      --skull_flash_timer;
      if(skull_flash_timer&2) pal_bright(8);
      else pal_bright(4);

      if(!skull_flash_timer)
      {
        pal_bright(4);
        pal_bg(levelList[((game_level%LEVEL_LAYOUTS)<<1)+1]);
        pal_spr(palGameSpr);
      }
    }

    refresh_power_hud();

    //a counter that does not allow objects to move while spawn animation plays

    if(wait)
    {
      --wait;

      if(!wait)
      {
        // v88: switch to uploaded gameplay music data.
        famitone_init(&music_data_main);
        sfx_init(&sound_data);
        music_play(0);
      }//start the music when all the objects spawned
    }

    //check for level completion condition

    if(items_collected==items_count)
    {
      // v146b: remove any harmless leftover sprite chips before the clear delay.
      for(ptr=0;ptr<CHIP_MAX;++ptr) chip_active[ptr]=FALSE;
      oam_clear();

      add_score(SCORE_LEVEL);
      // v88: restore original music data for clear jingle/screen music.
      famitone_init(&music_data);
      sfx_init(&sound_data);
      music_play(MUSIC_CLEAR);
      game_done=TRUE;
      game_clear=TRUE;
    }

    //process all the objects
    //player and enemies are the same type of object in this game,
    //to make code simpler and shorter, but generally they need to be
    //different kind of objects

    for(i=0;i<player_all;++i)
    {
      //per-object spawn animation counter, it counts fron N to 16 to 0
      //needed because objects spawn in sequence, not all at once

      if(player_wait[i])
      {
        if(player_wait[i]==16) sfx_play(i?SFX_RESPAWN2:SFX_RESPAWN1,i);
        --player_wait[i];
        continue;
      }

      if(wait) continue; //don't process object movements if spawn animation is running

      //Frozen Disk: slimes are paused and harmless while the timer is active.
      if(i&&freeze_timer) continue;

      //check collision of an enemy object with player object
      //NOT logic is used here, check http://gendev.spritesmind.net/page-collide.html

      if(i)
      {
        //Frozen Disk shots destroy enemies and send them back to spawn.
        for(ptr=0;ptr<SHOT_MAX;++ptr)
        {
          if(shot_active[ptr])
          {
            if(!((shot_x[ptr]+2)>=((player_x[i]>>FP_BITS)+14)||
                 (shot_x[ptr]+6)< ((player_x[i]>>FP_BITS)+2)||
                 (shot_y[ptr]+2)>=((player_y[i]>>FP_BITS)+14)||
                 (shot_y[ptr]+6)< ((player_y[i]>>FP_BITS)+2)))
            {
              shot_active[ptr]=FALSE;
              add_score(SCORE_ENEMY);
              sfx_play(SFX_RESPAWN2,i);
              player_x[i]=enemy_spawn_x[i];
              player_y[i]=enemy_spawn_y[i];
              player_cnt[i]=0;
              player_dir[i]=DIR_NONE;
              player_wait[i]=90;
              break;
            }
          }
        }

        if(player_wait[i]) continue;

        if(!((player_x[i]+(4 <<FP_BITS))>=(player_x[0]+(12<<FP_BITS))||
             (player_x[i]+(12<<FP_BITS))< (player_x[0]+(4 <<FP_BITS))||
           (player_y[i]+(4 <<FP_BITS))>=(player_y[0]+(12<<FP_BITS))||
           (player_y[i]+(12<<FP_BITS))< (player_y[0]+(4 <<FP_BITS))))
        {
          if(!game_clear)
          {
            // v88: restore original music data for lose jingle/screen music.
            famitone_init(&music_data);
            sfx_init(&sound_data);
            music_play(MUSIC_LOSE);
            game_done=TRUE;
            break;
          }
        }
      }

      //if movement counter is not zero, process the movement

      if(player_cnt[i])
      {
        switch(player_dir[i])
        {
        case DIR_RIGHT: player_x[i]+=player_speed[i]; break;
        case DIR_LEFT:  player_x[i]-=player_speed[i]; break;
        case DIR_DOWN:  player_y[i]+=player_speed[i]; break;
        case DIR_UP:    player_y[i]-=player_speed[i]; break;
        }

        player_cnt[i]-=player_speed[i];

        //if move from one tile to another is over, realign the object to tile grid
        //it is needed because when it moves with non-integer speed, it could
        //overrun the destination tile a little bit, and thus can't take a turn properly

        if(player_cnt[i]<=0)
        {
          if(player_cnt[i]<0) //overrun
          {
            player_cnt[i]=0;

            //0xff is a coordinate mask that leaves only integer tile offeset
            //it is 8:4:4 here, where 8 is integer tile coordinate,
            //first 4 is offset in the tile, which is 16 pixels wide,
            //and second 4 is fixed point resolution

            player_x[i]=(player_x[i]&0xff00)+(player_dir[i]==DIR_LEFT?0x100:0);
            player_y[i]=(player_y[i]&0xff00)+(player_dir[i]==DIR_UP  ?0x100:0);
          }

          //it is is the player object, check if there is an item in the new tile
          if(!i)
          {
            // v61: if B boost is active, count completed tile moves and stop
            // after two tiles even if the 1-second timer has time left.
            if(dash_boost_timer && dash_boost_tiles)
            {
              --dash_boost_tiles;
              if(!dash_boost_tiles)
              {
                dash_boost_timer=0;
                player_speed[0]=2<<FP_BITS;
              }
            }

            i16=MAP_ADR((player_x[i]>>(TILE_SIZE_BIT+FP_BITS)),
                        (player_y[i]>>(TILE_SIZE_BIT+FP_BITS)));

            if(map[i16]==TILE_ITEM)
            {
              map[i16]=TILE_EMPTY; //mark as collected in the game map

              sfx_play(SFX_ITEM,2);
              ++items_collected;
              add_score(SCORE_CART);

              //replace it with empty tile through the update list

              queue_meta_tile(map_index_to_nt(i16),TILE_EMPTY);

              //update number of collected carts/chips in the game stats

              refresh_items_hud();
            }

            if(orb_at_index(i16))
            {
              orb_active=FALSE;

              // v59: collect the disk into a held slot instead of activating immediately.
              // Press A to use it when the timing is right.
              held_power=orb_type;
              refresh_power_hud();

              add_score(SCORE_ORB);
              sfx_play(SFX_ITEM,2);
              orb_type=0;
            }

            //Code Chips are now sprites, not background tiles. Collect them
            //when the player reaches the same 16x16 map cell.
            for(ptr=0;ptr<CHIP_MAX;++ptr)
            {
              // v146b: bounding-box pickup prevents rare stranded/rogue chip sprites.
              if(chip_active[ptr]&&
                 !(((player_x[0]>>FP_BITS)+12)<chip_x[ptr]||
                   (player_x[0]>>FP_BITS)>(chip_x[ptr]+12)||
                   ((player_y[0]>>FP_BITS)+12)<chip_y[ptr]||
                   (player_y[0]>>FP_BITS)>(chip_y[ptr]+12)))
              {
                chip_active[ptr]=FALSE;
                sfx_play(SFX_ITEM,2);
                ++items_collected;
                add_score(SCORE_CHIP);
                refresh_items_hud();
              }
            }
          }
        }
      }

      if(!player_cnt[i]) //movement to the next tile is done, set up new movement
      {
        if(!i) //this is the player, process controls
        {
          //get gamepad state, it was previously polled with pad_trigger

          j=pad_state(0);

          //this is a tricky part to make controls more predictable
          //when you press two directions at once, sliding by a wall
          //to take turn into a passage on the side
          //this piece of code gives current direction lower priority
          //through testing it first
          //bits in player_dir var are matching to the buttons bits

          if(j&player_dir[0])
          {
            j&=~player_dir[0]; //remove the direction from further check
            player_move(i,player_dir[0]); //change the direction
          }

          //now continue control processing as usual

          if(j&PAD_LEFT)  player_move(i,DIR_LEFT);
          if(j&PAD_RIGHT) player_move(i,DIR_RIGHT);
          if(j&PAD_UP)    player_move(i,DIR_UP);
          if(j&PAD_DOWN)  player_move(i,DIR_DOWN);
        }
        else //this is an enemy, run AI
        {
          //the AI is very simple
          //first we create list of all directions that are possible to take
          //excluding the direction that is opposite to previous one

          i16=MAP_ADR((player_x[i]>>8),(player_y[i]>>8));

          //Pixel Panic v2.7: make the enemy-drop feature visible again.
          //Enemies can start dropping the Frozen Disk and required purple
          //Code Chips after the level has been active briefly. This keeps the
          //feature obvious instead of hiding it until every green cart is gone.
          if(frame_cnt>90&&map[i16]==TILE_EMPTY&&!chip_at_index(i16)&&!orb_at_index(i16)&&!bomb_at_index(i16))
          {
            //First power drops are Frozen Disk, Lightning Bolt, then Skull Clear.
            if(!orb_spawned)
            {
              orb_spawned=TRUE;
              spawn_orb(i16);
            }
            else if(!lightning_spawned&&chips_spawned>=1)
            {
              lightning_spawned=TRUE;
              spawn_lightning_orb(i16);
            }
            else if(!skull_spawned&&chips_spawned>=2)
            {
              skull_spawned=TRUE;
              spawn_skull_orb(i16);
            }
            //After that, enemies drop the required purple Code Chips often.
            else if(chips_spawned<chips_required&&!(rand8()%CODE_CHIP_DROP_CHANCE))
            {
              spawn_code_chip(i16);
            }
            // v104 testing: after the required drops, slimes can drop one blinking Glitch Bomb.
            else if(!bomb_state&&!bomb_blast_timer&&!(rand8()%BOMB_DROP_CHANCE))
            {
              spawn_glitch_bomb(i16);
            }
          }

          ptr=player_dir[i];

          // v78: later levels are more aggressive, but less impossible.
          // Slimes turn around more in later stages, but not constantly.
          spr=game_level/10;
          if(player_kind[i]==4&&!(rand8()&1)) ptr=DIR_NONE; // Nightmare can reverse often, not always.
          if(spr>=3&&!(rand8()&3)) ptr=DIR_NONE; // stages 31+
          if(spr>=4&&!(rand8()&1)) ptr=DIR_NONE; // stages 41+

          j=0;

          if(ptr!=DIR_RIGHT&&map[i16-1]!=TILE_WALL) dir[j++]=DIR_LEFT;
          if(ptr!=DIR_LEFT &&map[i16+1]!=TILE_WALL) dir[j++]=DIR_RIGHT;
          if(ptr!=DIR_DOWN &&map[i16-MAP_WDT]!=TILE_WALL) dir[j++]=DIR_UP;
          if(ptr!=DIR_UP   &&map[i16+MAP_WDT]!=TILE_WALL) dir[j++]=DIR_DOWN;

          //randomly select a possible direction

          player_move(i,dir[rand8()%j]);

          //if there was more than one possible direction,
          //i.e. it is a branch and not a corridor,
          //attempt to move towards the player

          if(j>1)
          {
            // v76: chase chance improves as levels climb.
            // Early stages stay readable, later stages feel meaner.
            if((player_kind[i]==4 && !(rand8()&1)) || spr==0 || (spr>=2 && (rand8()&3)<spr))
            {
              if(ptr!=DIR_DOWN &&player_y[0]<player_y[i]) player_move(i,DIR_UP);
              if(ptr!=DIR_UP   &&player_y[0]>player_y[i]) player_move(i,DIR_DOWN);
              if(ptr!=DIR_RIGHT&&player_x[0]<player_x[i]) player_move(i,DIR_LEFT);
              if(ptr!=DIR_LEFT &&player_x[0]>player_x[i]) player_move(i,DIR_RIGHT);
            }
          }
        }
      }
    }
  }

  delay(100);
  pal_fade_to(0);
}



//this is where the program starts
extern const void sound_data[];
extern const void music_data[];

void main(void)
{
  famitone_init(&music_data);
  sfx_init(&sound_data);
  nmi_set_callback(famitone_update);
  
  while(1)//infinite loop, title-gameplay
  {
    title_screen();

    // v98: restore original multi-song data after title intro.
    famitone_init(&music_data);
    sfx_init(&sound_data);

    game_level=start_level;
    game_lives=4;
    game_score=0;

    while(game_lives&&game_level<LEVELS_ALL)//loop for gameplay
    {
      show_screen(game_level);
      show_ready_countdown();

      game_loop();

      // v88: make sure screen music uses original multi-song music data.
      famitone_init(&music_data);
      sfx_init(&sound_data);

      if(game_clear)
      {
        show_level_clear(game_level);
        ++game_level;
      }
      else
      {
        --game_lives;
      }
    }

    show_screen(!game_lives?SCREEN_GAMEOVER:SCREEN_WELLDONE);//show game results
  }
}
