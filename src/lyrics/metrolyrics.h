#ifndef L_METROLYRICS_H
#define L_METROLYRICS_H

#include "../types.h"

const char * lyrics_metrolyrics_url(void);
memCache_t * lyrics_metrolyrics_parse(cb_object *capo);

#endif