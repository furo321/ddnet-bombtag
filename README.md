# Bombtag!
Bombtag is a Teeworlds mod where one or more players are bombs. The goal is to be the last man standing. It's basically a combination of survival, hammer party and a little bit of suicidal tee spirit.

## Features
This mod is pretty much an enchanced version of the original mod from [SomeRunce](https://www.teeworlds.com/forum/viewtopic.php?id=3965).

- Based on DDNet!
- 64 players support
- Ability to have multiple bombs
- 0.6 and 0.7 support
- Configurable gameplay

## Configuration
Bombtag has a few options you can use to configure and tweak the gameplay. They're the following,

- `bombtag_bombs_per_player`, The amount of bombs that should spawn per X alive players. Default: 6
- `bombtag_score_for_surviving`, The amount of score given for players surviving a round. Default: 0
- `bombtag_score_for_winning`, The amount of score given for the player who wins the round. Default: 1
- `bombtag_seconds_to_explosion`, The amount of seconds till the bomb explodes. Default: 15
- `bombtag_bomb_damage`, The amount of seconds removed from a bombs timer when hit by someone. Default: 1
- `bombtag_hammer_freeze`, The amount of ticks of freeze inflicted when hitting an alive player. (0 to disable). Default: 50

## Building
This mod is based on DDRaceNetwork. You can use their instructions to learn how to build the project, [ddnet/ddnet](https://github.com/ddnet/ddnet).
