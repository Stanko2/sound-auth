#include<iostream>
#include "audio-control.h"

#define PI 3.14159265

void AudioControl::play(float* data) {

}

static void registry_event_global(void *data, uint32_t id,
                uint32_t permissions, const char *type, uint32_t version,
                const struct spa_dict *props)
{
        printf("object: id:%u type:%s/%d\n", id, type, version);
}

static const struct pw_registry_events registry_events = {
        PW_VERSION_REGISTRY_EVENTS,
        .global = registry_event_global,
};

AudioControl::AudioControl(int argc, char *argv[])
{
  pw_init(&argc, &argv);
  std::cout << pw_get_headers_version() << '\n' << pw_get_library_version() << std::endl;

  loop = pw_main_loop_new(NULL);
  context = pw_context_new(pw_main_loop_get_loop(loop), NULL, 0);
  core = pw_context_connect(context, NULL, 0);
  registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
  spa_zero(listener);
  
  pw_registry_add_listener(registry, listener, &registry_events, NULL);

  pw_main_loop_run(loop);

}


AudioControl::~AudioControl() {
  pw_proxy_destroy((pw_proxy*)registry);
  pw_core_disconnect(core);
  pw_context_destroy(context);
  pw_main_loop_destroy(loop);
}