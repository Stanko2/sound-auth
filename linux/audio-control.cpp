#include<iostream>
#include "audio-control.h"

#define PI 3.14159265

void AudioControl::play(uint8_t* data) {

}

void AudioControl::process(void* data)
{
  std::cout << "Update" << std::endl;
}

AudioControl::AudioControl(int argc, char *argv[])
{
  const struct spa_pod *params[1];
  struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
 
  pw_init(&argc, &argv);
  std::cout << pw_get_headers_version() << '\n' << pw_get_library_version() << std::endl;

  loop = pw_main_loop_new(NULL);
  // context = pw_context_new(pw_main_loop_get_loop(loop), NULL, 0);
  // core = pw_context_connect(context, NULL, 0);
  output_stream = pw_stream_new_simple(
    pw_main_loop_get_loop(loop),
    "sound-auth",
    pw_properties_new(
      PW_KEY_MEDIA_TYPE, "Audio",
      PW_KEY_MEDIA_CATEGORY, "Others",
      PW_KEY_MEDIA_ROLE, "auth",
      NULL
    ),
    &stream_events,
    NULL
  );
  spa_audio_info_raw info = SPA_AUDIO_INFO_RAW_INIT(
    .format = SPA_AUDIO_FORMAT_S16,
    .rate = DEFAULT_RATE,
    .channels = DEFAULT_CHANNELS
  );
  params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
    &info);

  pw_stream_connect(output_stream, 
    PW_DIRECTION_OUTPUT,
    PW_ID_ANY,
    (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
    PW_STREAM_FLAG_MAP_BUFFERS |
    PW_STREAM_FLAG_RT_PROCESS),
    params, 1
  );

  pw_main_loop_run(loop);
}


AudioControl::~AudioControl() {
  pw_core_disconnect(core);
  pw_context_destroy(context);
  pw_main_loop_destroy(loop);
  pw_stream_destroy(output_stream);
}