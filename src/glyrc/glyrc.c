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

#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <glib.h>

#ifndef WIN32
  /* Backtrace*/
  #include <execinfo.h>
#endif

/* Somehow needed for g_vasprintf */
#include <glib/gprintf.h>

/* All you need from libglyr */
#include "../../lib/glyr.h"

/* Compile information, you do not have this header */
#include "../../lib/config.h"

/* Silly autohelp feature */
#include "autohelp.h"

/* Globals */
const gchar * exec_on_call = NULL;
const gchar * from_string  = NULL;
gchar * write_to = NULL;

//* ------------------------------------------------------- */

static gint message(gint verbosity, GlyrQuery * s, FILE * stream, const gchar * fmt, ...)
{
    int written_bytes = -1;
    if(s || verbosity == -1)
    {
        if(verbosity == -1 || verbosity <= s->verbosity)
        {
            va_list param;
            char * tmp = NULL;
            if(stream && fmt)
            {
                va_start(param,fmt);
                written_bytes = g_vasprintf (&tmp,fmt,param);
                va_end(param);

                if(written_bytes != -1 && tmp)
                {
                    written_bytes = fprintf(stream,"%s%s",(verbosity>2)?"DEBUG: ":"",tmp);
                    g_free(tmp);
                    tmp = NULL;
                }
            }
        }
    }
    return written_bytes;
}

//* ------------------------------------------------------- */

#ifndef WIN32
#define STACK_FRAME_SIZE 20

/* Obtain a backtrace and print it to stdout. */
static void print_trace(void)
{
    void * array[STACK_FRAME_SIZE];
    gchar ** bt_info_list;
    gsize size, it = 0;

    size = backtrace (array, STACK_FRAME_SIZE);
    bt_info_list = backtrace_symbols(array, size);

    for (it = 0; it < size; it++)
    {
        g_printerr("    [#%02u] %s\n",(gint)it+1, bt_info_list[it]);
    }

    g_printerr("\n%zd calls in total are shown.\n", size);
    g_free(bt_info_list);
}
#endif

//* ------------------------------------------------------- */

static void sig_handler(int signal)
{
    switch(signal)
    {
    case SIGABRT :
    case SIGFPE  :
    case SIGSEGV : /* sigh */
        message(-1,NULL,stderr,"\nFATAL: libglyr stopped/crashed due to a %s of death.\n",g_strsignal(signal));
        message(-1,NULL,stderr,"       This is entirely the fault of the libglyr developers. Yes, we failed. Sorry. Now what to do:\n");
        message(-1,NULL,stderr,"       It would be just natural to blame us now, so just visit <https://github.com/sahib/glyr/issues>\n");
        message(-1,NULL,stderr,"       and throw hard words like 'backtrace', 'bug report' or even the '$(command I issued' at them).\n");
        message(-1,NULL,stderr,"       The libglyr developers will try to fix it as soon as possible so you stop pulling their hair.\n");
#ifndef WIN32
        message(-1,NULL,stderr,"\nA list of the last called functions follows, please add this to your report:\n");
        print_trace();
#endif
        message(-1,NULL,stderr,"\n(Thanks, and Sorry for any bad feelings.)\n\n");
        break;
    }
    exit(EXIT_FAILURE);
}


//* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static void print_version(GlyrQuery * s)
{
    message(-1,s,stdout, "%s\n\n",glyr_version());
    message(-1,s,stderr, "This is still beta software, expect quite a lot bugs.\n");
    message(-1,s,stderr, "Email bugs to <sahib@online.de> or use the bugtracker\n"
            "at https://github.com/sahib/glyr/issues - Thank you! \n");

    exit(0);
}


//* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

void help_short(GlyrQuery * s)
{
    message(-1,s,stderr,"Usage: glyrc [GETTER] (options)\n\nwhere [GETTER] must be one of:\n");
    GlyrFetcherInfo * info  = glyr_get_plugin_info();
    GlyrFetcherInfo * track = info;
    while(info != NULL)
    {
        g_print(" - %s\n", info->name);
        info = info->next;
    }
    glyr_free_plugin_info(track);

#define IN "    "
    message(-1,s,stderr,"\nGENERAL OPTIONS:\n"
            IN"-f --from             String: Providers from where to get metadata. Refer to glyrc --list for a full list\n"
            IN"-w --write            Path: Write metadata to dir <d>, special values stdout, stderr and null are supported\n"
            IN"-n --number           Integer: Download max. <n> items. Amount of actual downloaded items may be less.\n"
            IN"-t --lang             String: Language settings. Used by a few getters to deliever localized data. Given in ISO 639-1 codes like 'de'\n"
            IN"-f --fuzzyness        Integer: Set treshold for level of Levenshtein algorithm.\n"
            IN"-q --qsratio          Float: How to weight quality/speed; 1.0 = full quality, 0.0 = full speed.\n"
            IN"-x --plugmax          Integer. Maximum number od download a plugin may deliever. Use to make results more vary.\n"
            IN"-v --verbosity        Integer. Set verbosity from 0 to 4. See --usage for details.\n"
	    "\nNETWORK OPTIONS\n"
            IN"-p --parallel         Integer: Define the number of downloads that may be performed in parallel.\n"
            IN"-u --useragent        String: The useragent to use during HTTP requests\n"
            IN"-r --redirects        Integer. Define the number of redirects that are allowed.\n"
            IN"-m --timeout          Integer. Define the maximum number in seconds after which a download is cancelled.\n"
            IN"-k --proxy            String: Set the proxy to use in the form of [protocol://][user:pass@]yourproxy.domain[:port]\n"
	    "\nPROVIDER SPECIFIC OPTIONS\n"
            IN"-d --download         Download Images.\n"
            IN"-D --skip-download    Don't download images, but return the URLs to them (act like a search engine)\n"
            IN"-a --artist           String: Artist name to search for\n"
            IN"-b --album            String: Album name to search for\n"
            IN"-t --title            String: Songname to search for\n"
            IN"-e --maxsize          Integer: (images only) The maximum size a cover may have.\n"
            IN"-i --minsize          Integer: (images only) The minimum size a cover may have.\n"
            IN"-F --formats          String: A semicolon seperated list of imageformats that are allowed. e.g.: \"png;jpeg\"\n"
            IN"-8 --force-utf8       Forces utf8 encoding for text items, invalid encodings get sorted out\n"
	    "\nMISC OPTIONS\n"
            IN"-L --list             List all fetchers and source providers for each and exit.\n"
            IN"-h --help             This text you unlucky wanderer are viewing.\n"
            IN"-V --version          Print the version string.\n"
            IN"-j --callback         Command: Set a bash command to be executed when a item is finished downloading;\n"
            IN"                      The special string <path> is expanded with the actual path to the data.\n"
	    IN"\n\n"
	    "With each item received you get a link to the original source, please refer to the individual terms of use,\n"
	    "copying and distributing of this data might be not allowed.\n"
	    "A more detailed version of this help can be found online: https://github.com/sahib/glyr/wiki/Commandline-arguments\n"
           );

    message(-1,s,stdout,"\nAUTHOR: (C) Christopher Pahl - 2011, <sahib@online.de>\n%s\n\n",glyr_version());
    exit(EXIT_FAILURE);
#undef IN
}

/* --------------------------------------------------------- */

static void visualize_from_options(void)
{
    GlyrFetcherInfo * info = glyr_get_plugin_info();
    if(info != NULL)
    {
        for(GlyrFetcherInfo * elem0 = info; elem0; elem0 = elem0->next)
        {
            g_print("%s\n",elem0->name);
            for(GlyrSourceInfo * elem1 = elem0->head; elem1; elem1 = elem1->next)
            {
                g_print("  [%c] %s\n",elem1->key,elem1->name);
            }
            g_print("\n");
        }
    }
    glyr_free_plugin_info(info);
}

/* --------------------------------------------------------- */

static void parse_commandline_general(int argc, char * const * argv, GlyrQuery * glyrs, gchar ** write_to)
{
    static struct option long_options[] =
    {
        {"from",          required_argument, 0, 'f'},
        {"write",         required_argument, 0, 'w'},
        {"parallel",      required_argument, 0, 'p'},
        {"redirects",     required_argument, 0, 'r'},
        {"timeout",       required_argument, 0, 'm'},
        {"proxy",	  required_argument, 0, 'k'},
        {"plugmax",       required_argument, 0, 'x'},
        {"useragent",     required_argument, 0, 'u'},
        {"verbosity",     required_argument, 0, 'v'},
        {"qsratio",       required_argument, 0, 'q'},
        {"formats",       required_argument, 0, 'F'},
        {"help",          no_argument,       0, 'h'},
        {"version",       no_argument,       0, 'V'},
        {"download",      no_argument,       0, 'd'},
        {"no-download",   no_argument,       0, 'D'},
        {"list",          no_argument,       0, 'L'},
        {"force-utf8",    no_argument,       0, '8'},
        // ---------- plugin specific ------------ //
        {"artist",        required_argument, 0, 'a'},
        {"album",         required_argument, 0, 'b'},
        {"title",         required_argument, 0, 't'},
        {"minsize",       required_argument, 0, 'i'},
        {"maxsize",       required_argument, 0, 'e'},
        {"number",        required_argument, 0, 'n'},
        {"lang",          required_argument, 0, 'l'},
        {"fuzzyness",     required_argument, 0, 'z'},
        {"prefer",        required_argument, 0, 'o'},
        {"callback",	  required_argument, 0, 'j'},
        {0,               0,                 0, '0'}
    };

    while(TRUE)
    {
        gint c;
        gint option_index = 0;
        if((c = getopt_long_only(argc, argv, "f:w:p:r:m:x:u:v:q:F:hVdDLa:b:t:i:e:n:l:z:o:j:k:8",long_options, &option_index)) == -1)
        {
            break;
        }

        switch (c)
        {
        case 'w':
        {
            gsize opt_len = strlen(optarg);
            if(g_ascii_strncasecmp(optarg,"stdout",opt_len) == 0 ||
               g_ascii_strncasecmp(optarg,"stderr",opt_len) == 0 ||
               g_ascii_strncasecmp(optarg,"null",  opt_len) == 0 ||
               g_file_test(optarg,G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) == TRUE)
            {
                *write_to = optarg;
            }
            else
            {
                g_printerr("'%s' does not seem to be an valid directory!\n",optarg);
                exit(-1);
            }
            break;
        }
        case 'f':
            glyr_opt_from(glyrs,optarg);
				from_string = optarg;
            break;
        case 'v':
            glyr_opt_verbosity(glyrs,atoi(optarg));
            break;
        case 'p':
            glyr_opt_parallel(glyrs,atoi(optarg));
            break;
        case 'r':
            glyr_opt_redirects(glyrs,atoi(optarg));
            break;
        case 'm':
            glyr_opt_timeout(glyrs,atoi(optarg));
            break;
        case 'u':
            glyr_opt_useragent(glyrs,optarg);
            break;
        case 'x':
            glyr_opt_plugmax(glyrs,atoi(optarg));
            break;
        case 'V':
            print_version(glyrs);
            break;
        case 'h':
            help_short(glyrs);
            break;
        case 'a':
            glyr_opt_artist(glyrs,optarg);
            break;
        case 'b':
            glyr_opt_album(glyrs,optarg);
            break;
        case 't':
            glyr_opt_title(glyrs,optarg);
            break;
        case 'i':
            glyr_opt_img_minsize(glyrs,atoi(optarg));
            break;
        case 'e':
            glyr_opt_img_maxsize(glyrs,atoi(optarg));
            break;
        case 'n':
            glyr_opt_number(glyrs,atoi(optarg));
            break;
        case 'd':
            glyr_opt_download(glyrs,true);
            break;
        case 'D':
            glyr_opt_download(glyrs,false);
            break;
        case 'l':
            glyr_opt_lang(glyrs,optarg);
            break;
        case 'L':
            visualize_from_options();
            exit(0);
            break;
        case 'z':
            glyr_opt_fuzzyness(glyrs,atoi(optarg));
            break;
        case 'j':
            exec_on_call = optarg;
            break;
        case 'k':
            glyr_opt_proxy(glyrs,optarg);
            break;
        case 'q':
            glyr_opt_qsratio(glyrs,atof(optarg));
            break;
        case 'F':
            glyr_opt_allowed_formats(glyrs,optarg);
            break;
        case '8':
            glyr_opt_force_utf8(glyrs,true);
            break;
        case '?':
            break;
        }
    }
}

//* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static void replace_char(gchar * string, gchar a, gchar b, gsize len)
{
    if(string != NULL)
    {
        gsize str_len = (len == 0) ? strlen(string) : len;
        for(gsize i = 0; i < str_len; i++)
        {
            if(string[i] == a)
            {
                string[i] = b;
            }
        }
    }
}

//* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

char * correct_path(const char * path)
{
    if(path != NULL)
    {
        gchar * no_slash = g_strdup(path);
        replace_char(no_slash,'/','+',0);

        if(no_slash)
        {
            replace_char(no_slash,' ','+',0);
        }
        return no_slash;
    }
    return NULL;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static char * path_covers(GlyrQuery * s, const char * save_dir, int i)
{
    char * good_artist = correct_path(s->artist);
    char * good_album  = correct_path(s->album );
    char * good_path   =  g_strdup_printf("%s/%s_%s_cover_%d.jpg",save_dir, good_artist,good_album,i);

    if(good_album)
        free(good_album);
    if(good_artist)
        free(good_artist);

    return good_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static char * path_lyrics(GlyrQuery * s, const char * save_dir, int i)
{
    char * good_artist = correct_path(s->artist);
    char * good_title  = correct_path(s->title );
    char * good_path   =  g_strdup_printf("%s/%s_%s_lyrics_%d.txt",save_dir,good_artist,good_title,i);

    if(good_title)
        free(good_title);
    if(good_artist)
        free(good_artist);

    return good_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static char * path_photos(GlyrQuery * s, const char * save_dir, int i)
{
    char * good_artist = correct_path(s->artist);
    char * good_path   =  g_strdup_printf("%s/%s_photo_%d.jpg",save_dir,good_artist,i);

    if(good_artist)
        free(good_artist);

    return good_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static char * path_ainfo(GlyrQuery * s, const char * save_dir, int i)
{
    char * good_artist = correct_path(s->artist);
    char * good_path   =  g_strdup_printf("%s/%s_ainfo_%d.txt",save_dir,good_artist,i);

    if(good_artist)
        free(good_artist);

    return good_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static char * path_similiar(GlyrQuery *s, const char * save_dir, int i)
{
    char * good_artist = correct_path(s->artist);
    char * good_path   =  g_strdup_printf("%s/%s_similiar_%d.txt",save_dir, good_artist,i);

    if(good_artist)
        free(good_artist);

    return good_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

static char * path_album_artist(GlyrQuery *s, const char * save_dir, int i, const char * type)
{
    char * good_artist = correct_path(s->artist);
    char * good_album  = correct_path(s->album );
    char * good_path   =  g_strdup_printf("%s/%s_%s_%s_%d.txt",save_dir,good_artist,good_album,type,i);

    if(good_artist)
        free(good_artist);
    if(good_album)
        free(good_album);

    return good_path;
}

/* --------------------------------------------------------- */
// ---------------------- MISC ----------------------------- //
/* --------------------------------------------------------- */

static char * path_review(GlyrQuery *s, const char * save_dir, int i)
{
    return path_album_artist(s,save_dir,i,"review");
}

/* --------------------------------------------------------- */

static char * path_tracklist(GlyrQuery *s, const char * save_dir, int i)
{
    return path_album_artist(s,save_dir,i,"tracktitle");
}

/* --------------------------------------------------------- */

static char * path_albumlist(GlyrQuery *s, const char * save_dir, int i)
{
    char * good_artist = correct_path(s->artist);
    char * good_path   =  g_strdup_printf("%s/%s_albumtitle_%d.txt",save_dir,good_artist,i);
    if(good_artist)
        free(good_artist);

    return good_path;
}

/* --------------------------------------------------------- */

static char * path_tags(GlyrQuery *s, const char * save_dir, int i)
{
    char * good_artist = correct_path(s->artist);
    char * good_path   =  g_strdup_printf("%s/%s_tag_%d.txt",save_dir,s->artist,i);

    if(good_artist)
        free(good_artist);

    return good_path;
}

/* --------------------------------------------------------- */

static char * path_relations(GlyrQuery *s, const char * save_dir, int i)
{
    char * good_artist = correct_path(s->artist);
    char * good_path   =  g_strdup_printf("%s/%s_url_%d.txt",save_dir,s->artist,i);

    if(good_artist)
        free(good_artist);

    return good_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* --------------------------------------------------------- */

char * get_path_by_type(GlyrQuery * s, const char * sd, int iter)
{
    char * m_path = NULL;
    switch(s->type)
    {
    case GLYR_GET_COVERART:
        m_path = path_covers(s,sd,iter);
        break;
    case GLYR_GET_LYRICS:
        m_path = path_lyrics(s,sd,iter);
        break;
    case GLYR_GET_ARTIST_PHOTOS:
        m_path = path_photos(s,sd,iter);
        break;
    case GLYR_GET_ARTISTBIO:
        m_path = path_ainfo(s,sd,iter);
        break;
    case GLYR_GET_SIMILIAR_ARTISTS:
        m_path = path_similiar(s,sd,iter);
        break;
    case GLYR_GET_SIMILIAR_SONGS:
        m_path = path_similiar(s,sd,iter);
        break;
    case GLYR_GET_ALBUM_REVIEW:
        m_path = path_review(s,sd,iter);
        break;
    case GLYR_GET_TRACKLIST:
        m_path = path_tracklist(s,sd,iter);
        break;
    case GLYR_GET_ALBUMLIST:
        m_path = path_albumlist(s,sd,iter);
        break;
    case GLYR_GET_TAGS:
        m_path = path_tags(s,sd,iter);
        break;
    case GLYR_GET_RELATIONS:
        m_path = path_relations(s,sd,iter);
        break;
    case GLYR_GET_UNSURE:
        message(-1,NULL,stderr,"glyrc: getPath(): Unknown type, Problem?\n");
    }
    return m_path;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* -------------------------------------------------------- */

static void print_item(GlyrQuery *s, GlyrMemCache * cacheditem, int num)
{
    message(1,s,stderr,"\n///// ITEM #%d /////\n",num);
    glyr_printitem(cacheditem);
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* -------------------------------------------------------- */

static GLYR_ERROR callback(GlyrMemCache * c, GlyrQuery * s)
{
    // This is just to demonstrate the callback option.
    // Put anything in here that you want to be executed when
    // a cache is 'ready' (i.e. ready for return)
    // See the glyr_set_dl_callback for more info
    // a custom pointer is in s->callback.user_pointer
    int * current = s->callback.user_pointer;

    // Text Represantation of this item
    if(s->verbosity >= 1)
    {
        print_item(s,c,*(current));
    }

    /* write out 'live' */
    if(write_to != NULL)
    {
        gsize write_len = strlen(write_to);
        if(g_ascii_strncasecmp(write_to,"stdout",write_len) == 0||
           g_ascii_strncasecmp(write_to,"stderr",write_len) == 0||
           g_ascii_strncasecmp(write_to,"null",  write_len) == 0)
        {
            glyr_write(c,write_to);
        }
        else
        {
            gchar * path = get_path_by_type(s,write_to,*current);
            if(path != NULL)
            {
                if(s->verbosity > 1)
                {
                    message(1,s,stderr,"\nWRITE to '%s'\n",path);
                    message(1,s,stderr,"////////////////////\n");
                }

                if(glyr_write(c,path) == -1)
                {
                    message(1,s,stderr,"(!!) glyrc: writing data to <%s> failed.\n",path);
                }
            }

            /* call the program if any specified */
            if(exec_on_call != NULL)
            {
                char ** path_splitv = g_strsplit(exec_on_call,"<path>",0);
                char * replace_path = g_strjoinv(path,path_splitv);
                g_strfreev(path_splitv);

                /* Call that command */
                int exitVal = system(replace_path);
                if(exitVal != EXIT_SUCCESS)
                {
                    message(3,s,stderr,"glyrc: cmd returned a value != EXIT_SUCCESS\n");
                }

                g_free(replace_path);
            }
            g_free(path);
        }
    }

    if(current != NULL)
    {
        *current += 1;
    }
    else
    {
        message(-1,NULL,stderr,"warning: Empty counterpointer!\n");
    }
    return GLYRE_OK;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* -------------------------------------------------------- */

GLYR_GET_TYPE get_type_from_string(gchar * string)
{
    GLYR_GET_TYPE result = GLYR_GET_UNSURE;
    GlyrFetcherInfo * info = glyr_get_plugin_info();
    if(info != NULL)
    {
        for(GlyrFetcherInfo * elem = info; elem; elem = elem->next)
        {
            if(g_ascii_strncasecmp(elem->name,string,strlen(elem->name)) == 0)
            {
                result = elem->type;
                break;
            }
        }
    }
    else
    {
        g_printerr("Warning: Can't get type information. Probably a bug.\n");
    }
    glyr_free_plugin_info(info);
    return result;
}

/* --------------------------------------------------------- */
// --------------------------------------------------------- //
/* -------------------------------------------------------- */

int main(int argc, char * argv[])
{
    /* Try to print informative output */
    signal(SIGSEGV, sig_handler);

    /* Assume success */
    gint result = EXIT_SUCCESS;

    /* Init. You _have_ to call this before making any calls */
    glyr_init();

    /* For every init, you should call glyr_cleanup,
     * Note: Both methods are NOT Threadsafe!
     */
    atexit(glyr_cleanup);

    if(argc >= 2)
    {
        /* The struct that control this beast */
        GlyrQuery my_query;

        /* set it on default values */
        glyr_init_query(&my_query);

        /* Default to a bit more verbose mode */
        glyr_opt_verbosity(&my_query,2);

		  GLYR_GET_TYPE type = get_type_from_string(argv[1]);
		  if(type == GLYR_GET_UNSURE)
		  {
				g_print("glyr: \"%s\" is not a know getter. See `glyrc -L` for a list.\n",argv[1]);
				suggest_other_getter(&my_query,argv[1]);
				g_print("\n");
				exit(-1);
        }
		
	/* Set the type */
        my_query.type = type;

        parse_commandline_general(argc-1, argv+1, &my_query,&write_to);

        if(write_to == NULL)
        {
            write_to = ".";
        }

        // Set the callback - it will do all the actual work
        int item_counter = 0;
        glyr_opt_dlcallback(&my_query, callback, &item_counter);

        if(my_query.type != GLYR_GET_UNSURE)
        {
            // Now start searching!
            int length = -1;
            GLYR_ERROR get_error = GLYRE_OK;
            GlyrMemCache * my_list = glyr_get(&my_query, &get_error, &length);

            if(my_list)
            {
                if(get_error == GLYRE_OK)
                {
#if 0
                    /* This is the place where you would work with the cachelist *
                       As the callback is used in glyrc this is just plain empty *
                       Useful if you need to cache the data (e.g. for batch jobs *
                       Left only for the reader's informatiom, no functions here *
                     */

                    GlyrMemCache * elem = my_list;
                    while(elem != NULL)
                    {
                    		g_print("%s\n",elem->dsrc);
                    		elem = elem->next;
                    }
#endif 

                    message(2,&my_query,stderr,"- In total %d item(s) found.\n",length);
                }

                // Free all downloaded buffers
                glyr_free_list(my_list);

            }
				else if(get_error == GLYRE_NO_PROVIDER)
				{
					g_print("glyr: --from \"%s\" does not contain any valid provider.\nSee `glyrc -L` for a list.\n",from_string);
					suggest_other_provider(&my_query,(gchar*)from_string);
				}
            else if(get_error != GLYRE_OK)
            {
                message(1,&my_query,stderr,"E: %s\n",glyr_strerror(get_error));
            }


        }
        // Clean memory alloc'd by settings
        glyr_destroy_query( &my_query);

    }
    else if(argc >= 2 && (!strcmp(argv[1],"-V") || !strcmp(argv[1],"--list")))
    {
        print_version(NULL);
    }
    else if(argc >= 2 && (!strcmp(argv[1],"-L") || !strcmp(argv[1],"--list")))
    {
        visualize_from_options();
    }
    else
    {
        help_short(NULL);
    }

    // byebye
    return result;
}

//* --------------------------------------------------------- */
// ------------------End of program-------------------------- //
/* ---------------------------------------------------------- */