/* Copyright 2021 Kyle Farrell
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

#ifndef VIEW_GAMEOVER_H
#define VIEW_GAMEOVER_H

#include "tempest.h"
#include "model.h"


struct display_strategy* create_gameover_display_strategy(struct model *model);
void free_gameover_display_strategy(struct display_strategy *display_strategy); 

#endif
