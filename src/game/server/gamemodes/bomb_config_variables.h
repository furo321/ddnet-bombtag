// def, min, max
MACRO_CONFIG_INT(BombtagBombsPerPlayer, bombtag_bombs_per_player, 1, 1, 64, CFGFLAG_SERVER, "The amount of bombs that should spawn per X alive players.")
MACRO_CONFIG_INT(BombtagScoreForSuriving, bombtag_score_for_surviving, 0, 0, 100, CFGFLAG_SERVER, "The amount of score given for players surviving a round.")
MACRO_CONFIG_INT(BombtagScoreForWinning, bombtag_score_for_winning, 1, 0, 100, CFGFLAG_SERVER, "The amount of score given for the player who wins the round.")

MACRO_CONFIG_INT(BombtagSecondsToExplosion, bombtag_seconds_to_explosion, 15, 0, 100, CFGFLAG_SERVER, "The amount of seconds till the bomb explodes.")
MACRO_CONFIG_INT(BombtagBombDamage, bombtag_bomb_damage, 1, 0, 100, CFGFLAG_SERVER, "The amount of seconds removed from a bombs timer when hit by someone.")
MACRO_CONFIG_INT(BombtagHammerFreeze, bombtag_hammer_freeze, 1, 0, 100, CFGFLAG_SERVER, "The amount of seconds of freeze inflicted when hitting an alive player. (0 to disable)")
