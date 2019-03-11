#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <unistd.h>

//#define dbg_printf printf
#define dbg_printf(msg, args...)

int main (int argc, char *argv[]) {
  int err;
  snd_seq_t *seq_handle;
  
  if ((err = snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0)) < 0) {
    fprintf (stderr, "cannot open audio device %s (%s)\n", 
	     argv[1],
	     snd_strerror (err));
    exit(1);
  }
  
  if ((err = snd_seq_create_simple_port(seq_handle, "port",
					SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE
					| SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
					SND_SEQ_PORT_TYPE_MIDI_GENERIC)) < 0)  {
    fprintf (stderr, "cannot open audio device %s (%s) to create input port\n", 
	     argv[1],
	     snd_strerror (err));
    exit(1);
  }

  int port_id = err;

  if ((err = snd_seq_connect_from(seq_handle, port_id, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE)) < 0) {
    fprintf(stderr, "snd_seq_subscribe_from() failed: %s\n", snd_strerror(err));
    exit(1);
  }

  if ((err = snd_seq_alloc_queue(seq_handle)) < 0) {
    fprintf(stderr, "snd_seq_create_queue() failed: %s\n", snd_strerror(err));
    exit(1);
  }

  int queue_id = err;

  if ((err = snd_seq_start_queue(seq_handle, queue_id, NULL)) < 0) {
    fprintf(stderr, "snd_seq_start_queue() failed: %s\n", snd_strerror(err));
    exit(1);
  }

  unsigned long curr_delay_ms = 10;
  printf("Current delay: %lu ms\n", curr_delay_ms);
  while (1) {
    snd_seq_event_t *ev = NULL;
    unsigned long delay_ms = 0;
    if ((err = snd_seq_event_input(seq_handle, &ev)) < 0) {
      fprintf(stderr, "snd_seq_event_input() failed: %s\n", snd_strerror(err));
      continue;
    }
    if ((ev->type == SND_SEQ_EVENT_NOTEON)||(ev->type == SND_SEQ_EVENT_NOTEOFF)) {
      if (ev->type == SND_SEQ_EVENT_NOTEON && ev->data.note.velocity > 0 && ev->data.note.note == 85) {
	if (curr_delay_ms > 5)
	  curr_delay_ms /= 2;
	else
	  curr_delay_ms = 0;
	printf("Current delay: %lu ms\n", curr_delay_ms);
      } else if (ev->type == SND_SEQ_EVENT_NOTEON && ev->data.note.velocity > 0 && ev->data.note.note == 87) {
	if (curr_delay_ms == 0)
	  curr_delay_ms = 5;
	else if (curr_delay_ms < 1000)
	  curr_delay_ms *= 2;
	printf("Current delay: %lu ms\n", curr_delay_ms);
      }
      if (ev->data.note.note >= 60) {
	delay_ms = curr_delay_ms;
      }
      const char *type = (ev->type == SND_SEQ_EVENT_NOTEON) ? "on " : "off";
      dbg_printf("[%d] Note %s: %2x vel(%2x)\n", ev->time.tick, type,
	     ev->data.note.note,
	     ev->data.note.velocity);
    } else if(ev->type == SND_SEQ_EVENT_CONTROLLER) {
      dbg_printf("[%d] Control:  %2x val(%2x)\n", ev->time.tick,
	     ev->data.control.param,
	     ev->data.control.value);
    } else if(ev->type == SND_SEQ_EVENT_PGMCHANGE) {
      dbg_printf("[%d] Program change:  %2x val(%2x)\n", ev->time.tick,
	     ev->data.control.param,
	     ev->data.control.value);
    } else if(ev->type == SND_SEQ_EVENT_PITCHBEND) {
      dbg_printf("[%d] Pith bend:  %2x val(%2x)\n", ev->time.tick,
	     ev->data.control.param,
	     ev->data.control.value);
    } else if (ev->type == SND_SEQ_EVENT_CLIENT_START || ev->type == SND_SEQ_EVENT_CLIENT_EXIT || ev->type == SND_SEQ_EVENT_CLIENT_CHANGE) {
      dbg_printf("[%d] Client event: client=%d\n", ev->time.tick, ev->data.addr.client);
    } else if (ev->type == SND_SEQ_EVENT_PORT_START || ev->type == SND_SEQ_EVENT_PORT_EXIT || ev->type == SND_SEQ_EVENT_PORT_CHANGE) {
      dbg_printf("[%d] Port event: client=%d, port=%d\n", ev->time.tick, ev->data.addr.client, ev->data.addr.port);
    } else {
      dbg_printf("[%d] Unknown:  Unhandled Event Received\n", ev->time.tick);
    }

    snd_seq_ev_set_source(ev, port_id);
    snd_seq_ev_set_subs(ev);
    struct snd_seq_real_time delta = (struct snd_seq_real_time) {
      .tv_sec = delay_ms / 1000,
      .tv_nsec = (delay_ms % 1000L) * 1000L * 1000L
    };
    snd_seq_ev_schedule_real(ev, queue_id, 1, &delta);
    //snd_seq_ev_set_direct(ev);
    if ((err = snd_seq_event_output(seq_handle, ev)) < 0) {
      fprintf(stderr, "snd_seq_event_output() failed: %s\n", snd_strerror(err));
      exit(1);
    }
    snd_seq_drain_output(seq_handle);
  }

  snd_seq_close(seq_handle);

  exit (0);
}
