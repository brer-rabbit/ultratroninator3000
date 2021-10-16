#ifndef UT3K_PULSEAUDIO_H
#define UT3K_PULSEAUDIO_H

#include <stdint.h>

void ut3k_new_audio_context();
void ut3k_pa_mainloop_iterate();
void ut3k_disconnect_audio_context();

char* ut3k_upload_wavfile(char *filename, char *sample_name);
void ut3k_play_sample(const char *sample_name);
void ut3k_play_sample_at_volume(const char *sample_name, int32_t volume);
int32_t ut3k_get_default_volume();
void ut3k_remove_sample(char *sample_name);
void ut3k_remove_all_samples();
void ut3k_wait_last_operation();
void ut3k_list_samples();

// returns, via first arg, pointer to a list of sinks.
// second arg is number of sinks.
// caller is responsible for freeing memory for the list.
void ut3k_get_sink_list(uint32_t sink_list[], int *num); // pa_context_get_sink_info_list
void ut3k_set_default_sink(uint32_t index);
void ut3k_set_sink_volume(uint32_t index);

#endif
