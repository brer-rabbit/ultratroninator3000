/* qix.h
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


#ifndef TEMPTEST_H
#define TEMPEST_H

#include "ut3k_pulseaudio.h"

#define GAMES_LOCAL_ENV_VAR "GAMES_LOCAL"
#define DEFAULT_GAMES_BASEDIR "/usr/local/games/ultratroninator_3000"
#define CONFIG_DIRNAME "/etc/"
#define CONFIG_FILENAME "tempest.config"
#define SAMPLE_DIRNAME "/samples/"

extern const char *crash_on_spike_soundkey;
extern const char *enemy_destroyed_soundkey;
extern const char *highscore_soundkey;
extern const char *level_up_soundkey;
extern const char *player_lose_life_soundkey;
extern const char *player_move_soundkey;
extern const char *player_multishot_soundkey;
extern const char *player_shoot_soundkey;
extern const char *superzapper_soundkey;

extern const char *electric1_soundkey;
extern const char *electric2_soundkey;
extern const char *electric3_soundkey;
extern const char *electric4_soundkey;
extern const char *electric5_soundkey;
extern const char *electric6_soundkey;
extern const char *electric7_soundkey;

#endif
