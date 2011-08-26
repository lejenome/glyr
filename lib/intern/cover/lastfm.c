/***********************************************************
* This file is part of glyr
* + a commnadline tool and library to download various sort of musicrelated metadata.
* + Copyright (C) [2011]  [Christopher Pahl]
* + Hosted at: https://github.com/sahib/glyr
*
* glyr is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* glyr is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with glyr. If not, see <http://www.gnu.org/licenses/>.
**************************************************************/

#include "lastfm.h"
#include "../../utils.h"
#include "../../stringlib.h"
#include "../../core.h"

/* ----------------------------------------------- */

const char * cover_lastfm_url(GlyrQuery * sets)
{
    return "http://ws.audioscrobbler.com/2.0/?method=album.search&album=%artist%+%album%&api_key="API_KEY_LASTFM;
}

/* ----------------------------------------------- */

#define BAD_DEFAULT_IMAGE "http://cdn.last.fm/flatness/catalogue/noimage/2/default_album_medium.png"

GList * cover_lastfm_parse(cb_object *capo)
{
    /* Handle size requirements (Default to large) */
    const char *tag_ssize = NULL ;
    const char *tag_esize = "</image>";

    /* find desired size */
    if( size_is_okay(300,capo->s->img_min_size,capo->s->img_max_size) )
        tag_ssize = "<image size=\"extralarge\">";
    else if( size_is_okay(125,capo->s->img_min_size,capo->s->img_max_size) )
        tag_ssize = "<image size=\"large\">";
    else if( size_is_okay(64, capo->s->img_min_size,capo->s->img_max_size) )
        tag_ssize = "<image size=\"middle\">";
    else if( size_is_okay(34, capo->s->img_min_size,capo->s->img_max_size) )
        tag_ssize = "<image size=\"small\">";
    else if ( true || false )
        tag_ssize = "<image size=\"extralarge\">";

    /* The result (perhaps) */
    GlyrMemCache * result = NULL;
    GList * result_list = NULL;
    gchar * find  = capo->cache->data;
	gsize tag_len = strlen(tag_ssize);

    while(continue_search(g_list_length(result_list),capo->s) && (find = strstr(find+1, tag_ssize)) != NULL)
    {
        gchar * end_tag;
        if((end_tag = strstr(find,tag_esize)) != NULL)
        {
            gchar * url;
			find += tag_len;
            if((url = copy_value(find,end_tag)) != NULL)
            {
                if(strcmp(url,BAD_DEFAULT_IMAGE) != 0)
                {
                    result = DL_init();
                    result->data = url;
                    result->size = end_tag - find;
                    result_list = g_list_prepend(result_list,result);
                }
                else
                {
                    g_free(url);
                }
            }
        }
    }
    return result_list;
}

/* ----------------------------------------------- */

MetaDataSource cover_lastfm_src =
{
    .name      = "lastfm",
    .key       = 'l',
    .parser    = cover_lastfm_parse,
    .get_url   = cover_lastfm_url,
    .type      = GET_COVERART,
    .quality   = 90,
    .speed     = 75,
    .endmarker = NULL,
    .free_url  = false
};