#include "EventListener.h"
#include "../../include/UsbInterface.h"
#include "../../interfaces/MutexInterface.h"
#include "../../interfaces/SystemInterface.h"
#include "../../utility/Utils.h"

static void register_event_callback(struct EventListener* event_listener, event_callback callback, void* buffer);
static void deregister_event_callback(struct EventListener* event_listener, event_callback callback);

void new_super_event_listener(EventListener *listener) {
  __INIT_EVENT_LISTENER__(listener);
  listener->head.l_e = 0;
  listener->l_e.l_e = 0;
  listener->listener_id = -1;

  listener->listener_mutex =
      (Mutex_C *)interface_allocateMemory(sizeof(Mutex_C), 0);
  __STRUCT_INIT__(listener->listener_mutex, new_mutex, new_mutex);
}

static void register_event_callback(EventListener *event_listener,
                             event_callback callback, void* buffer) {
  list_element *l_e = event_listener->head.l_e;
  Event_Callback *event_c =
      (Event_Callback *)interface_allocateMemory(sizeof(Event_Callback), 0);
  event_c->callback = callback;
  event_c->buffer = buffer;
  event_c->l_e.l_e = 0;

  // event_listener->listener_mutex->acquire_c(event_listener->listener_mutex);

  if (l_e == (void *)0) {
    event_listener->head.l_e = &event_c->l_e;
    event_listener->listener_mutex->release_c(event_listener->listener_mutex);
    return;
  }

  while (l_e->l_e != (void *)0) {
    l_e = l_e->l_e;
  }

  l_e->l_e = &event_c->l_e;

  // event_listener->listener_mutex->release_c(event_listener->listener_mutex);
}

static void deregister_event_callback(EventListener *event_listener,
                               event_callback callback) {
  list_element *l_e = event_listener->head.l_e;
  list_element *prev = 0;

  // event_listener->listener_mutex->acquire_c(event_listener->listener_mutex);

  while (l_e != (void *)0) {
    Event_Callback *event_c =
        (Event_Callback *)container_of(l_e, Event_Callback, l_e);
    if (event_c->callback == callback) {
      if (prev == (void *)0) {
        event_listener->head.l_e = l_e->l_e;
      } else {
        prev->l_e = l_e->l_e;
      }
      interface_freeMemory(event_c, 0);
      // event_listener->listener_mutex->release_c(event_listener->listener_mutex);
      return;
    }
    prev = l_e;
    l_e = l_e->l_e;
  }

  // event_listener->listener_mutex->release_c(event_listener->listener_mutex);
}