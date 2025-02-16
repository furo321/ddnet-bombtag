// ddnet-bombtag config variables
#ifndef ENGINE_SHARED_BOMB_CONFIG_VARIABLES_H
#define ENGINE_SHARED_BOMB_CONFIG_VARIABLES_H
#undef ENGINE_SHARED_BOMB_CONFIG_VARIABLES_H // this file will be included several times

#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Save, Desc) ;
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Save, Desc) ;
#endif

// def, min, max
MACRO_CONFIG_INT(BombtagBombsPerPlayer, bombtag_bombs_per_player, 6, 1, 64, CFGFLAG_SERVER, "The amount of bombs that should spawn per X alive players, 1 for everyone except one.")
MACRO_CONFIG_INT(BombtagScoreForSuriving, bombtag_score_for_surviving, 0, 0, 100, CFGFLAG_SERVER, "The amount of score given for players surviving a round.")
MACRO_CONFIG_INT(BombtagScoreForWinning, bombtag_score_for_winning, 1, 0, 100, CFGFLAG_SERVER, "The amount of score given for the player who wins the round.")

MACRO_CONFIG_INT(BombtagSecondsToExplosion, bombtag_seconds_to_explosion, 15, 0, 100, CFGFLAG_SERVER, "The amount of seconds till the bomb explodes.")
MACRO_CONFIG_INT(BombtagMinSecondsToExplosion, bombtag_minimum_seconds_to_explosion, 1, 0, 10, CFGFLAG_SERVER, "The minimum amount of seconds a tee's bomb timer will have after getting bomb.")
MACRO_CONFIG_INT(BombtagBombDamage, bombtag_bomb_damage, 1, 0, 100, CFGFLAG_SERVER, "The amount of seconds removed from a bombs timer when hit by someone.")
MACRO_CONFIG_INT(BombtagHammerFreeze, bombtag_hammer_freeze, 50, 0, 100000, CFGFLAG_SERVER, "The amount of ticks of freeze inflicted when hitting an alive player. (0 to disable)")
MACRO_CONFIG_INT(BombtagBombWeapon, bombtag_bomb_weapon, 0, 0, 5, CFGFLAG_SERVER, "Which weapon should the bomb be given? 0 - Hammer, 1 - Gun, 2 - Shotgun, 3 - Grenade, 4 - Laser, 5 - Ninja")
MACRO_CONFIG_INT(BombtagCollateralDamage, bombtag_collateral_damage, 0, 0, 1, CFGFLAG_SERVER, "Enable collateral damage, exploding bombs will eliminate any tees in a 3 tile radius.")

MACRO_CONFIG_INT(BombtagMysteryChance, bombtag_mystery_chance, 0, 0, 100, CFGFLAG_SERVER, "The percentage of a mystery round happening! A random line from sv_mysteryrounds_filename will be executed.")
MACRO_CONFIG_STR(SvMysteryRoundsFileName, sv_mysteryrounds_filename, IO_MAX_PATH_LENGTH, "", CFGFLAG_SERVER, "File which contains mystery round commands, one round per line.")
MACRO_CONFIG_STR(SvMysteryRoundsResetFileName, sv_mysteryrounds_reset_filename, IO_MAX_PATH_LENGTH, "", CFGFLAG_SERVER, "File which contains the commands to execute after a mystery round.")

#endif
