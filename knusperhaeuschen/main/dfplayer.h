/**
 * This code is public domain.
 */
#ifndef MAIN_DFPLAYER_H
#define MAIN_DFPLAYER_H


void dfplayer_init();


void dfplayer_bell();


void dfplayer_set_volume_p(int vol_per_cent); // 0..100


int dfplayer_get_volume_p();


void dfplayer_set_track(int track);


int dfplayer_get_track();


#endif /* MAIN_DFPLAYER_H */
