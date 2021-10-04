/* hex invaders defs
 * Copyright 2021 Kyle Farrell
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef HEX_INV_ADER_H
#define HEX_INV_ADER_H

#include "ut3k_pulseaudio.h"

#define GAMES_LOCAL_ENV_VAR "GAMES_LOCAL"
#define DEFAULT_GAMES_BASEDIR "/usr/local/games/ultratroninator_3000"
#define CONFIG_DIRNAME "/etc/"
#define CONFIG_FILENAME "hex_invaders.config"
#define SAMPLE_DIRNAME "/samples/"


// config keys for various sounds
extern const char *laser_toggled_soundkey;
extern const char *showhex_soundkey;
extern const char *hidehex_soundkey;
extern const char *playerfire_soundkey;
extern const char *gameover_soundkey;
extern const char *attract_soundkey;
extern const char *shieldhit_soundkey;
extern const char *shieldlow_soundkey;
extern const char *moveinvaders1_soundkey;
extern const char *moveinvaders2_soundkey;
extern const char *invader_forming_soundkey;
extern const char *laser_hit_invader_soundkey;
extern const char *laser_hit_shielded_invader_soundkey;
extern const char *laser_hit_invader_shield_destroyed_soundkey;
extern const char *levelup_soundkey;
extern const char *start_game_soundkey;


#endif
