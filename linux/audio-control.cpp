#pragma once
#include<iostream>
#include "audio-control.h"
#include <string>

#define PI 3.14159265

void AudioControl::play(uint8_t* data) {
}

void AudioControl::process_output(void* data)
{
  std::cout << "Update output" << std::endl;
}

void AudioControl::process_input(void* data)
{
  AudioControl* d = instance;
  // std::cout << "Update input" << std::endl;
  pw_buffer* b;
  
  b = pw_stream_dequeue_buffer(d->input_stream);

  if(b == NULL) {
    pw_log_warn("Out of buffer");
    return;
  }

  spa_buffer* buff = b->buffer;
  uint16_t* samples = (uint16_t*)buff->datas[0].data;

  int32_t capture_size = buff->datas[0].chunk->size / sizeof(uint16_t);

  // std::cout << "Captured " << capture_size << "samples" << std::endl;

  instance->comm->samples_received(samples, capture_size);

  pw_stream_queue_buffer(d->input_stream, b);
}

void AudioControl::on_samples_received()
{
}

AudioControl::AudioControl(Communication* comm)
{
  this->comm = comm;
  instance = this;
  const struct spa_pod *output_params[1];
  const struct spa_pod *input_params[1];
  
  spa_pod_builder b_out = SPA_POD_BUILDER_INIT(output_buffer, sizeof(output_buffer));
  spa_pod_builder b_in = SPA_POD_BUILDER_INIT(input_buffer, sizeof(input_buffer));
 
  pw_init(NULL, NULL);
  std::cout << pw_get_headers_version() << '\n' << pw_get_library_version() << std::endl;

  loop = pw_main_loop_new(NULL);
  
  pw_properties* props = pw_properties_new(
    PW_KEY_MEDIA_TYPE, "Audio",
    PW_KEY_MEDIA_CATEGORY, "Others",
    PW_KEY_MEDIA_ROLE, "auth",
    NULL
  );

  output_stream = pw_stream_new_simple(
    pw_main_loop_get_loop(loop),
    "sound-auth-output",
    props,
    &output_stream_events,
    (void*)"output"
  );

  input_stream = pw_stream_new_simple(
    pw_main_loop_get_loop(loop),
    "sound-auth-input",
    props,
    &input_stream_events,
    NULL
  );



  spa_audio_info_raw info = SPA_AUDIO_INFO_RAW_INIT(
    .format = SPA_AUDIO_FORMAT_S16,
    .rate = DEFAULT_RATE,
    .channels = SPA_AUDIO_CHANNEL_MONO
  );
  
  output_params[0] = spa_format_audio_raw_build(&b_out, SPA_PARAM_EnumFormat,
    &info);
  int ret;
  // int ret = pw_stream_connect(output_stream, 
  //   PW_DIRECTION_OUTPUT,
  //   PW_ID_ANY,
  //   (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
  //   PW_STREAM_FLAG_MAP_BUFFERS |
  //   PW_STREAM_FLAG_RT_PROCESS),
  //   output_params, 1
  // );

  if(ret < 0) {
    std::cout << "Failed to create output stream " << ret << std::endl;
  }

  input_params[0] = spa_format_audio_raw_build(&b_in, SPA_PARAM_EnumFormat,
    &info);

  ret = pw_stream_connect(input_stream, 
    PW_DIRECTION_INPUT,
    PW_ID_ANY,
    (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
    PW_STREAM_FLAG_MAP_BUFFERS | 
    PW_STREAM_FLAG_RT_PROCESS),
    input_params, 1
  );

  if (ret < 0) {
    std::cout << "Failed to create input stream " << ret << std::endl;
  }

  pw_main_loop_run(loop);
}


AudioControl::~AudioControl() {
  pw_core_disconnect(core);
  pw_context_destroy(context);
  pw_main_loop_destroy(loop);
  pw_stream_destroy(output_stream);
  pw_stream_destroy(input_stream);

  pw_deinit();
}