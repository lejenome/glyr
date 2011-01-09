#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core.h"
#include "types.h"
#include "stringop.h"

//Include plugins:
#include "cover/last_fm.h"
#include "cover/discogs.h"
#include "cover/amazon.h"
#include "cover/lyricswiki.h"
#include "cover/google.h"

char * get_cover(const char *artist, const char *album, const char *dir,  char update, char max_parallel, const char * order)
{
    if (artist == NULL ||  album == NULL)
    {
        fprintf(stderr,"(!) %s == NULL",artist?"album":"artist");
        return NULL;
    }

    char * escaped_album  = escape_slashes(album);
    char * escaped_artist = escape_slashes(artist);

    char * filename = NULL;
    if (escaped_artist && escaped_album)
    {
        filename = strdup_printf("%s/%s-%s.jpg",(dir) ? dir : "./",escaped_artist,escaped_album);
        free(escaped_album);
        free(escaped_artist);
    }

    // Check if cover is already in dir
    if (update == 0 && access(filename,R_OK) == 0) {
        return filename;
    }

    // else: do what this thingie is supposed to do:

    // Assume a max. of 10 plugins, just set it higher if you need to.
    cb_object *plugin_list = malloc(sizeof(cb_object) * 10);

    size_t it = 0;

    int max = -1;
    int min = 150;

    // Sanitize input
    if (max != -1 && min > max) min=-1;
    if (min != -1 && max < min) max=-1;

    // Default order
    if (order == NULL)
    {
        order = "wldag";
    }

    // Register plugins
    // You only have to set the url and the callback to your needs..
    int i = 0;
    for (i = 0; i < strlen(order); i++)
    {
        switch (order[i])
        {
        case 'w':
            if (max == -1 || max >= 400)
            {
                plugin_init( &plugin_list[it++], cover_lyricswiki_url(), cover_lyricswiki_parse, min, max, C_C"<W>"C_);
            }
            break;
        case 'l':
            if (min == -1 || min <= 125)
            {
                plugin_init( &plugin_list[it++], cover_lastfm_url(), cover_lastfm_parse, min,max,C_B"<L>"C_);
            }
            break;
        case 'd':
            if (max == -1 || max >= 400)
            {
                plugin_init( &plugin_list[it++], cover_discogs_url(), cover_discogs_parse,min,max,C_G"<D>"C_);
            }
            break;
        case 'a':
            plugin_init( &plugin_list[it++], cover_amazon_url_lng(-1), cover_amazon_parse,min,max,C_Y"<A>"C_);
            break;
        case 'g':
            plugin_init( &plugin_list[it++], cover_google_url(min,max),  cover_google_parse,min,max,C_R"<G>"C_);
            break;
        }
    }

    fprintf(stderr,C_R"["C_"%s"C_R"]-["C_"%s"C_R"]=["C_,artist,album);

    // Now do the actual work
    memCache_t *result = invoke(plugin_list, it, 10,artist,album,"NULL");

    if (result != NULL && result->data != NULL && filename)
    {
        memCache_t * image = download_single(result->data,1L);
        fprintf(stderr,C_G"*"C_R"] "C_);

        if (image == NULL)
        {
            fprintf(stderr,"?? Found an apparently correct url, but unable to download <%s>\n",result->data);
            fprintf(stderr,"?? Please drop me a note, this might be a bug.\n");
        }

        write_file(filename,image);
        fprintf(stderr,C_G">> ("C_"'%s'"C_G")\n"C_,filename);

        DL_free(image);
        DL_free(result);
        result = NULL;
    }
    else
    {
        fprintf(stderr,C_"\\<end>"C_R"] ");
        if (filename)
        {
            free(filename);
        }

        fprintf(stderr,C_R":(\n"C_);
        filename = NULL;
    }

    if (plugin_list)
    {
        free(plugin_list);
        plugin_list = NULL;
    }

    return filename;
}

