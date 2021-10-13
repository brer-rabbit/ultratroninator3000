#include <assert.h>
#include <libgen.h>
#include <sndfile.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>
#include <unistd.h>

#include <pulse/pulseaudio.h>

#include "ut3k_pulseaudio.h"

char *context_name = "Ultratroninator 3000 audio";

struct sample {
    char *name;
    LIST_ENTRY(sample) nodes;
};

LIST_HEAD(sample_list, sample) sample_list;

static pa_mainloop *mainloop = NULL;
static pa_mainloop_api *mainloop_api = NULL;
static pa_context *context = NULL;
static pa_operation *pa_operation_most_recent = NULL;

static enum pa_sample_format sndfile_format_to_pa_sample_format(int sfinfo);
static char* filename_to_samplename(char *filename);
static int ut3k_write_stream(pa_stream *stream, pa_sample_spec *ss, size_t file_stream_bytes, SNDFILE *f_sndfile);


/** ut3k_new_audio_context
 * create a new audio context connecting to a default pulse audio server
 */
void ut3k_new_audio_context() {
    // Get a mainloop and its context
    mainloop = pa_mainloop_new();
    assert(mainloop);
    mainloop_api = pa_mainloop_get_api(mainloop);

    context = pa_context_new(mainloop_api, context_name);
    assert(context);

    // may need to remove noautospawn later
    //assert(pa_context_connect(context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) == 0);
    if (pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
      printf("pa_context_connect() failed: %s\n", pa_strerror(pa_context_errno(context)));
    }

    // wait for the context to connect to the server
    pa_context_state_t context_state;
    do {
        context_state = pa_context_get_state(context);
        assert(PA_CONTEXT_IS_GOOD(context_state));
        ut3k_pa_mainloop_iterate();
    } while (context_state != PA_CONTEXT_READY);

    // init the linked list
    LIST_INIT(&sample_list);
    printf("pulseaudio connection ready\n");

    return;
}


void ut3k_pa_mainloop_iterate() {
    assert(pa_mainloop_iterate(mainloop, 0, NULL) >= 0);
}


/** ut3k_disconnect_audio_context
 *
 * disconnect from the server.  Set argument remove_samples to iterate
 * over the cached sample list and remove samples and free associated
 * structures.  Set to zero to keep stuff in memory.
 */
void ut3k_disconnect_audio_context(int remove_samples) {

    if (remove_samples) {
        // iterate over the sample list
        
    }

    if (context) {
        pa_context_disconnect(context);
        context = NULL;
    }
    if (mainloop) {
        pa_mainloop_quit(mainloop, 0);
        pa_mainloop_free(mainloop);
        mainloop = NULL;
    }
}


/** ut3k_upload_wavfile
 *
 * given a filename of a wav file, upload that to the PulseAudio server.
 * On success return the sample name that can be used to reference the file.
 * A good citizen will eventually remove the samples via the related
 * ut3k_remove_sample call.
 * The returned string is memory managed by this library and gets
 * free'd as part of the ut3k_remove_sample call.
 */

char* ut3k_upload_wavfile(char *filename, char *sample_name) {
    SNDFILE *f_sndfile;
    SF_INFO info;
    pa_stream *stream;
    pa_sample_spec ss;
    size_t file_stream_bytes;
    struct sample *new_sample;
    char *pulse_audio_stream_name;

    assert(filename != NULL);
    f_sndfile = sf_open(filename, SFM_READ, &info);

    printf("sndfile opened %s\n", filename);

    if (sf_error(f_sndfile) != 0) {
        printf("File error: %s\n", sf_strerror(f_sndfile));
        return 0;
    }


    ss.format = sndfile_format_to_pa_sample_format(info.format);
    ss.channels = info.channels;
    ss.rate = info.samplerate;

    switch(ss.format) {
    case PA_SAMPLE_S16LE:
        file_stream_bytes = 2 * info.channels * info.frames;
        break;
    case PA_SAMPLE_S24LE: // this isn't likely right for 24 bit...
    case PA_SAMPLE_S32LE:
    case PA_SAMPLE_FLOAT32LE:
        file_stream_bytes = 4 * info.channels * info.frames;
        break;
    case PA_SAMPLE_U8:
    default:
        file_stream_bytes = 1 * info.channels * info.frames;
        break;
    }

    // established the file characteristic, create a stream and write
    // it to the server.  Use the basename of the filename for the
    // uploaded stream.  Track this internally too in the linked list.
    new_sample = (struct sample*)malloc(sizeof(struct sample));
    LIST_INSERT_HEAD(&sample_list, new_sample, nodes);
    
    if (sample_name) { // caller supplied a name to use
      pulse_audio_stream_name = sample_name;
      new_sample->name = strndup(sample_name, 128);  // copy since it's incoming data
    }
    else {
      pulse_audio_stream_name = filename_to_samplename(filename);
      new_sample->name = pulse_audio_stream_name;  // mem already copied
    }

    stream = pa_stream_new(context, pulse_audio_stream_name, &ss, NULL);
    pa_stream_ref(stream);
    assert(pa_stream_connect_upload(stream, file_stream_bytes) == 0);

    // wait for it to be ready
    pa_stream_state_t stream_state;
    do {
        stream_state = pa_stream_get_state(stream);
        assert(PA_STREAM_IS_GOOD(stream_state));
        ut3k_pa_mainloop_iterate();
    } while (stream_state != PA_STREAM_READY);


    // stream is ready to write
    ut3k_write_stream(stream, &ss, file_stream_bytes, f_sndfile);


    // throwing an iterate between finish & disconnect cause an
    // assertion on 12.2.  took out the iterate and things seem to
    // work.  Guess the server doesn't need cycles between these
    // calls.
    assert(pa_stream_finish_upload(stream) == 0);
    assert(pa_stream_disconnect(stream) == 0);
    pa_stream_unref(stream);
    ut3k_pa_mainloop_iterate();

    return sample_name;
}


void ut3k_play_sample(const char *sample_name) {
  ut3k_play_sample_at_volume(sample_name, PA_VOLUME_NORM);
}


void ut3k_play_sample_at_volume(const char *sample_name, int32_t volume) {
    if (pa_operation_most_recent != NULL) {
        pa_operation_unref(pa_operation_most_recent);
    }

    pa_operation_most_recent = pa_context_play_sample(context, sample_name, NULL, volume, NULL, NULL);
    pa_operation_ref(pa_operation_most_recent);
}


int32_t ut3k_get_default_volume() {
  return PA_VOLUME_NORM;
}


/** ut3k_wait_last_operation
 *
 * wait for the most recent playback operation to be complete-
 * note operation complete doesn't mean the file played, but that the request
 * went through
 */
void ut3k_wait_last_operation() {
    if (pa_operation_most_recent != NULL) {
        // use most recent operation
        printf("checking operation state\n");
        while (pa_operation_get_state(pa_operation_most_recent) == PA_OPERATION_RUNNING) {
                ut3k_pa_mainloop_iterate();
        }
        pa_operation_most_recent = NULL;
    }
}


void ut3k_list_samples() {
    struct sample *sample_ptr;
    LIST_FOREACH(sample_ptr, &sample_list, nodes) {
        printf("sample: %s\n", sample_ptr->name);
    }
}



void ut3k_remove_sample(char *sample_name) {
    struct sample *sample_to_remove;
    pa_operation *op;
    op = pa_context_remove_sample(context, sample_name, NULL, NULL);
    LIST_FOREACH(sample_to_remove, &sample_list, nodes) {
        if (strncmp(sample_to_remove->name, sample_name, 128) == 0) {
            break;
        }
    }
    LIST_REMOVE(sample_to_remove, nodes);
    free(sample_to_remove->name);
    free(sample_to_remove);
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
        ut3k_pa_mainloop_iterate();
    }
}


void ut3k_remove_all_samples() {
    struct sample *sample;
    pa_operation *op = NULL;

    while (!LIST_EMPTY(&sample_list)) {
      sample = LIST_FIRST(&sample_list);
      op = pa_context_remove_sample(context, sample->name, NULL, NULL);
      LIST_REMOVE(sample, nodes);
      free(sample->name);
      free(sample);
    }

    // technically this just checks the last operation and we may
    // have queued quite a few up...well, so be it.
    while (op != NULL && pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
        ut3k_pa_mainloop_iterate();
    }
}


void ut3k_get_sink_list(int **sink_list, int *num) {
    // pa_context_get_sink_info_list
    // return a pointer to array of ints
}

void ut3k_set_default_sink(int sink) {
}



static enum pa_sample_format sndfile_format_to_pa_sample_format(int sfinfo) {
    switch (sfinfo & SF_FORMAT_SUBMASK) {
    case SF_FORMAT_PCM_16:
        return PA_SAMPLE_S16LE;
    case SF_FORMAT_PCM_24:
        return PA_SAMPLE_S24LE;
    case SF_FORMAT_PCM_32:
        return PA_SAMPLE_S32LE;
    case SF_FORMAT_PCM_U8:
        return PA_SAMPLE_U8;
    case SF_FORMAT_PCM_S8:
        return PA_SAMPLE_U8;
    case SF_FORMAT_FLOAT:
        return PA_SAMPLE_FLOAT32LE;
    default:
        printf("sndfile unsupported\n");
        return PA_SAMPLE_INVALID;
    }

}


/** ut3k_write_stream
 * assumes the stream is ready to write
 * use the sample format provided,
 * write file_stream_bytes from f_sndfile to the stream
 */
static int ut3k_write_stream(pa_stream *stream, pa_sample_spec *ss, size_t file_stream_bytes, SNDFILE *f_sndfile) {
    uint8_t *buffer = NULL;
    size_t file_bytes_remaining = file_stream_bytes, sf_frames_read, pa_bytes_allowed;

    while (file_bytes_remaining > 0) {
        // we've got file_bytes_remaining to write to PA.
        // Request PA to upload this -- PA may respond with a smaller size,
        // tracked in pa_bytes_allowed
        pa_bytes_allowed = file_bytes_remaining;
        assert(pa_stream_begin_write(stream, (void**)&buffer, &pa_bytes_allowed) == 0);

        switch (ss->format) {
        case PA_SAMPLE_S16LE:
            sf_frames_read = sf_read_short(f_sndfile, (short*)buffer, pa_bytes_allowed / 2);
            // compute what sndfile returned as bytes- it ought to match
            // I'd think the assertion should be
            // sf_frames_read * ss->channels * 2 == pa_bytes_allowed
            // but channels doesn't seem to work
            assert(sf_frames_read * 2 == pa_bytes_allowed);
            break;
        case PA_SAMPLE_S32LE:
        case PA_SAMPLE_FLOAT32LE:
        case PA_SAMPLE_S24LE:  // not sure if this is correct for 24 bit...
            sf_frames_read = sf_readf_int(f_sndfile, (int*)buffer, pa_bytes_allowed / 4);
            assert(sf_frames_read * 4 == pa_bytes_allowed);
          break;
        case PA_SAMPLE_U8:
        default:
            sf_frames_read = sf_read_raw(f_sndfile, (void*)buffer, pa_bytes_allowed);
            assert(sf_frames_read == pa_bytes_allowed);

        }

        file_bytes_remaining -= pa_bytes_allowed;

	assert(pa_stream_write(stream, buffer, pa_bytes_allowed, NULL, 0LL, PA_SEEK_RELATIVE) == 0);
        // I don't know how necessary this is... I'm assuming we've got
        // to give PA some cycle
        ut3k_pa_mainloop_iterate();
    }

    //printf("finished stream write with file_bytes_remaining = %d\n", file_bytes_remaining);

    return 0;
}


/** filename_to_samplename
 * use the basename of the passed in filename, minux any suffix.
 * the return char* is caller owned and should be free()'d when done
 */
static char* filename_to_samplename(char *filename) {
    char *full_filename, *file_basename, *suffix_start, *sample_name;
    char *alloced_basename_mem = strndup(filename, 128);
    full_filename = alloced_basename_mem;

    /* basename(3):
     * "These functions may return pointers to statically allocated
     *  memory which may be overwritten by subsequent calls.
     *  Alternatively, they may return a pointer to some part of path,
     *  so that the string referred to by path should not be modified
     *  or freed until the pointer returned by the function is no
     *  longer required."
     */
    file_basename = basename(full_filename);
    suffix_start = rindex(file_basename, '.');

    if (suffix_start == NULL) {
        // no suffix, just use basename
        sample_name = strdup(file_basename);
    }
    else {
        sample_name = strndup(file_basename, suffix_start - file_basename);
    }

    free(alloced_basename_mem);

    return sample_name;
}
