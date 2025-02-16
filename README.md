# Bombtag!
Bombtag is a Teeworlds mod where one or more players are bombs. The goal is to be the last man standing. It's basically a combination of survival, hammer party and a little bit of suicidal tee spirit.

## Features
This mod is pretty much an enchanced version of the original mod from [SomeRunce](https://www.teeworlds.com/forum/viewtopic.php?id=3965).

- Based on DDNet!
- 64 players support
- Ability to have multiple bombs
- Ability to use a different weapon than hammer for bomb
- 0.6 ~~and 0.7~~ support
- Configurable gameplay

## Configuration
Bombtag has a few options you can use to configure and tweak the gameplay. They're the following,

- `bombtag_bombs_per_player`, The amount of bombs that should spawn per X alive players, 1 for everyone except one. Default: 6
- `bombtag_score_for_surviving`, The amount of score given for players surviving a round. Default: 0
- `bombtag_score_for_winning`, The amount of score given for the player who wins the round. Default: 1
- `bombtag_seconds_to_explosion`, The amount of seconds till the bomb explodes. Default: 15
- `bombtag_minimum_seconds_to_explosion`, The minimum amount of seconds a tee's bomb timer will have after getting bomb. Default: 1
- `bombtag_bomb_damage`, The amount of seconds removed from a bombs timer when hit by someone. Default: 1
- `bombtag_hammer_freeze`, The amount of ticks of freeze inflicted when hitting an alive player. (0 to disable). Default: 50
- `bombtag_bomb_weapon`, Which weapon should the bomb be given? 0 - Hammer, 1 - Gun, 2 - Shotgun, 3 - Grenade, 4 - Laser, 5 - Ninja. Default: 0
- `bombtag_seconds_to_explosion`, The minimum amount of seconds a tee's bomb timer will have after getting bomb. Default: 1
- `bombtag_collateral_damage`, Enable collateral damage, exploding bombs will eliminate any tees in a 3 tile radius. Default: 0
- `bombtag_mystery_chance`, The percentage of a mystery round happening! A random line from sv_mysteryrounds_filename will be executed. Default: 0
- `sv_mysteryrounds_filename`, File which contains mystery round commands, one round per line. Default: ""
- `sv_mysteryrounds_reset_filename`, File which contains the commands to execute after a mystery round. Default: ""

## Commands

- `enqueue_map r[map]`, Adds a map to queue after the round has finished.
- `reload_mysteryrounds`, Reloads the configuration file for mystery rounds.

## Antibot
Bombtag has a simple antibot which can be enabled using `-DANTIBOT=ON` in CMake. The antibot kicks people using autoclickers and permanently bans anyone who sends advertisements of a specific popular cheat client.

## Building
This mod is based on DDRaceNetwork. You can use their instructions to learn how to build the project, [ddnet/ddnet](https://github.com/ddnet/ddnet).
