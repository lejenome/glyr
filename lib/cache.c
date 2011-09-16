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
#include "core.h"
#include "glyr.h"
#include "register_plugins.h"
#include "cache.h"

#include <glib.h>


/* ------------------------------------------------------------------ */
static void create_table_defs(GlyrDatabase * db);
static void execute(GlyrDatabase * db, const gchar * sql_statement);
static GLYR_FIELD_REQUIREMENT get_req(GlyrQuery * q);
/* ------------------------------------------------------------------ */


GlyrDatabase * glyr_db_init(char * root_path)
{
	if(sqlite3_threadsafe() == 0)
	{
		g_printerr("WARNING: Your SQLite version seems not to be threadsafe!\n");
	}

	GlyrDatabase * to_return = NULL;
	if(root_path != NULL && g_file_test(root_path,G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) == TRUE)
	{
		sqlite3 * db_connection = NULL;

		gchar * db_file_path = g_strdup_printf("file://%s%s%s",root_path,(g_str_has_suffix(root_path,"/") ? "" : "/"),GLYR_DB_FILENAME);
		gint db_err = sqlite3_open_v2(db_file_path,&db_connection, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI, NULL);
		if(db_err == SQLITE_OK)
		{
			to_return = g_malloc0(sizeof(GlyrDatabase));
			to_return->root_path = g_strdup(root_path);
			to_return->db_handle = db_connection;
			create_table_defs(to_return);
		}
		else
		{
			glyr_message(-1,NULL,"Connecting to database failed: %s\n",sqlite3_errmsg(db_connection));
			sqlite3_close(db_connection);
		}
		g_free(db_file_path);
	}
	return to_return;
}

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/


void glyr_db_destroy(GlyrDatabase * db_object)
{
	if(db_object != NULL)
	{
		int db_err = sqlite3_close(db_object->db_handle);
		if(db_err == SQLITE_OK)
		{
			g_free((gchar*)db_object->root_path);
			g_free(db_object);
		}
		else
		{
			glyr_message(-1,NULL,"Disconnecting database failed: %s\n",sqlite3_errmsg(db_object->db_handle));
		}
	}
}

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/


static void execute(GlyrDatabase * db, const gchar * sql_statement)
{
	if(db && sql_statement)
	{
		//puts(sql_statement);
		char * err_msg = NULL;
		sqlite3_exec(db->db_handle,sql_statement,NULL,NULL,&err_msg);
		if(err_msg != NULL)
		{
			fprintf(stderr, "SQL error: %s\n", err_msg);
			sqlite3_free(err_msg);
		}
	}
}

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/


static void create_table_defs(GlyrDatabase * db)
{
	execute(db,
	"BEGIN;\n"
	"-- Provider\n"
	"CREATE TABLE IF NOT EXISTS providers (provider_name VARCHAR(20) UNIQUE);\n"
	"\n"
	"-- Artist\n"
	"CREATE TABLE IF NOT EXISTS artists (artist_name VARCHAR(128) UNIQUE);\n"
	"CREATE TABLE IF NOT EXISTS albums  (album_name  VARCHAR(128) UNIQUE);\n"
	"CREATE TABLE IF NOT EXISTS titles  (title_name  VARCHAR(128) UNIQUE);\n"
	"\n"
	"-- Enum\n"
	"CREATE TABLE IF NOT EXISTS image_types(image_type_name VARCHAR(16) UNIQUE);\n"
	"CREATE TABLE IF NOT EXISTS db_version(version INTEGER UNIQUE);\n"
	"\n"
	"-- MetaData\n"
	"CREATE TABLE IF NOT EXISTS metadata(\n"
	"			          artist_id INTEGER,\n"
	"			          album_id  INTEGER,\n"
	"			          title_id  INTEGER,\n"
	"			          provider_id INTEGER,\n"
	"			          source_url  VARCHAR(512),\n"
	"			          image_type_id INTEGER,\n"
	"                                 track_duration INTEGER,\n"
	"			          get_type INTEGER,\n"
	"			          data_type INTEGER,\n"
	"			          data_size INTEGER,\n"
	"			          data_checksum BLOB,\n"
	"			          data BLOB\n"
	");\n"
	"CREATE INDEX IF NOT EXISTS index_artist_id   ON metadata(artist_id);\n"
	"CREATE INDEX IF NOT EXISTS index_album_id    ON metadata(album_id);\n"
	"CREATE INDEX IF NOT EXISTS index_title_id    ON metadata(title_id);\n"
	"CREATE INDEX IF NOT EXISTS index_provider_id ON metadata(provider_id);\n"
	"CREATE UNIQUE INDEX IF NOT EXISTS index_unique ON metadata(data_type,data_checksum,source_url);\n"
	"-- Insert imageformats\n"
	"INSERT OR IGNORE INTO image_types VALUES('jpeg');\n"
	"INSERT OR IGNORE INTO image_types VALUES('jpg');\n"
	"INSERT OR IGNORE INTO image_types VALUES('png');\n"
	"INSERT OR IGNORE INTO image_types VALUES('gif');\n"
	"INSERT OR IGNORE INTO image_types VALUES('tiff');\n"
	"INSERT OR IGNORE INTO db_version VALUES(0);\n"
	"COMMIT;\n"
	);
}

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/


static void insert_cache_data(GlyrDatabase * db, GlyrQuery * query, GlyrMemCache * cache)
{
	char * sql= sqlite3_mprintf("INSERT OR IGNORE INTO metadata VALUES(\n"
			"(select rowid from artists where artist_name = '%q'),\n"
			"(select rowid from albums  where album_name = '%q'),\n"
			"(select rowid from titles  where title_name = '%q'),\n"
			"(select rowid from providers where provider_name = '%q'),\n"
			"?,"
			"(select rowid from image_types where image_type_name = '%q'),"
			"?,?,?,?,?,?);\n",
			query->artist,
			query->album,
			query->title,
			cache->prov,
			cache->img_format
			);

	sqlite3_stmt *stmt = NULL;
	sqlite3_prepare_v2(db->db_handle, sql, strlen(sql) + 1, &stmt, NULL);

	sqlite3_bind_text(stmt, 1, cache->dsrc,strlen(cache->dsrc) + 1, SQLITE_TRANSIENT);
	sqlite3_bind_int (stmt, 2, cache->duration);
	sqlite3_bind_int (stmt, 3, query->type);
	sqlite3_bind_int( stmt, 4, cache->type);
	sqlite3_bind_int( stmt, 5, cache->size);
	sqlite3_bind_blob(stmt, 6, cache->md5sum, sizeof cache->md5sum, SQLITE_TRANSIENT);
	sqlite3_bind_blob(stmt, 7, cache->data, cache->size, SQLITE_TRANSIENT);

	if(sqlite3_step(stmt) != SQLITE_DONE) 
	{
		fprintf(stderr,"Error message: %s\n", sqlite3_errmsg(db->db_handle));
	}
	sqlite3_finalize(stmt);

	sqlite3_free(sql);
}

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/

#define INSERT_STRING(SQL,ARG) { if(ARG) {gchar * sql = sqlite3_mprintf(SQL,ARG); execute(db,sql); sqlite3_free(sql);}}

void glyr_db_insert(GlyrDatabase * db, GlyrQuery * q, GlyrMemCache * cache)
{
	if(db && q && cache)
	{
		GLYR_FIELD_REQUIREMENT reqs = get_req(q);
		execute(db,"BEGIN;");
		if((reqs & GLYR_REQUIRES_ARTIST) || (reqs & GLYR_OPTIONAL_ARTIST)) {
		  INSERT_STRING("INSERT OR IGNORE INTO artists VALUES('%q');",q->artist); 
		}
		if((reqs & GLYR_REQUIRES_ALBUM) || (reqs & GLYR_OPTIONAL_ALBUM)) {
		  INSERT_STRING("INSERT OR IGNORE INTO albums  VALUES('%q');",q->album);
		}
		if((reqs & GLYR_REQUIRES_TITLE) || (reqs & GLYR_OPTIONAL_TITLE)) {
	 	  INSERT_STRING("INSERT OR IGNORE INTO titles  VALUES('%q');",q->title);
		}

		INSERT_STRING("INSERT OR IGNORE INTO providers VALUES('%q');",cache->prov);
		insert_cache_data(db,q,cache);
		execute(db,"COMMIT;");
	}
}

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/


static void add_to_cache_list(GlyrMemCache * head, GlyrMemCache * to_add)
{
	while(head && head->next)
	{
		head = head->next;
	}
	if(head != NULL && to_add != NULL)
	{
		head->next = to_add;
		to_add->prev = head;
	}
}

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/


static int select_callback(void * result, int argc, char ** argv, char ** azColName)
{
   GlyrMemCache** list = (GlyrMemCache **)result;
   if(list != NULL && argc >= 12)
   {
	   GlyrMemCache * head  = *list;
	   GlyrMemCache * cache = DL_init();
	   if(cache != NULL)
	   {
		   cache->prov = g_strdup(argv[3]);
		   cache->dsrc = g_strdup(argv[4]);
		   cache->img_format = g_strdup(argv[5]);

		   cache->duration = (argv[6] ? strtol(argv[6],NULL,10) : 0);
		   cache->type =     (argv[8] ? strtol(argv[8],NULL,10) : 0);
		   cache->size =     (argv[9] ? strtol(argv[9],NULL,10) : 0);

		   if(argv[10] != NULL)
		   {
			   memcpy(cache->md5sum,argv[10],16);
		   }

		   cache->is_image = !(cache->img_format == NULL);

		   if(argv[11] != NULL && cache->size > 0)
		   {
			   cache->data = g_malloc0(cache->size + 1);
			   memcpy(cache->data,argv[11],cache->size);
			   cache->data[cache->size] = 0;
		   }


		   if(head == NULL)
		   {
			   *list = cache;
		   }
		   else
		   { 
			   add_to_cache_list(head,cache);
		   }
	   }
   }

   return 0;
}

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/

static gchar * convert_from_option_to_sql(GlyrQuery * q)
{
	gchar * result = g_strdup("");

	for(GList * elem = r_getSList(); elem; elem = elem->next)
	{
		MetaDataSource * item = elem->data;
		if(item && q->type == item->type)
		{
			if(provider_is_enabled(q->from,item) == TRUE)
			{
				gchar * old_mem = result;
				result = g_strdup_printf("%s%s'%s'",result,(*result) ? "," : "",item->name);
				g_free(old_mem);
			}
		}
	}
	return result;
}

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/

static GLYR_FIELD_REQUIREMENT get_req(GlyrQuery * q)
{
	GLYR_FIELD_REQUIREMENT result = 0;
	GlyrFetcherInfo * info = glyr_info_get();
	GlyrFetcherInfo * head = info;
	while(head != NULL)
	{
		if(q && q->type == head->type)
		{
			result = head->reqs;
		}
		head = head->next;
	}
	glyr_info_free(info);
	return result;
}


/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/

GlyrMemCache * glyr_db_lookup(GlyrDatabase * db, GlyrQuery * query)
{
	GlyrMemCache * result = NULL;
	if(db != NULL && query != NULL)
	{

		GLYR_FIELD_REQUIREMENT reqs = get_req(query);
		gchar * artist_constr = ((reqs & GLYR_REQUIRES_ARTIST) == 0) ? "OR artist_name is null" : "";
		gchar * album_constr  = ((reqs & GLYR_REQUIRES_ALBUM ) == 0) ? "OR album_name  is null" : "";
		gchar * title_constr  = ((reqs & GLYR_REQUIRES_TITLE ) == 0) ? "OR title_name  is null" : "";

		gchar * from_argument_list = convert_from_option_to_sql(query);

		gchar * img_url_constr = "";
		if(TYPE_IS_IMAGE(query->type))
		{
			if(query->download == FALSE)
			{
				img_url_constr = sqlite3_mprintf("AND data_type = %d ", GLYR_TYPE_IMG_URL);
			}
			else
			{
				img_url_constr = sqlite3_mprintf("AND NOT data_type = %d ", GLYR_TYPE_IMG_URL);
			}
		}

		gchar * sql = sqlite3_mprintf(
			"SELECT artist_name,album_name,title_name,provider_name,source_url, "
			"image_type_name,track_duration,get_type,data_type,data_size,data_checksum,data "
			"FROM metadata as m "
			"LEFT JOIN artists as a on m.artist_id = a.rowid "
			"LEFT JOIN albums as b on m.album_id = b.rowid "
			"LEFT JOIN titles as t on m.title_id = t.rowid "
			"INNER JOIN providers as p on m.provider_id = p.rowid "
			"LEFT JOIN image_types as i on m.image_type_id = i.rowid "
			"WHERE m.get_type = %d AND provider_name IN(%s) %s"
			"AND (artist_name = '%q' %s) "
			"AND (album_name = '%q'  %s) "
			"AND (title_name = '%q'  %s);",
			query->type, from_argument_list, 
			img_url_constr,
			query->artist, artist_constr,
			query->album,  album_constr,
			query->title,  title_constr);

		if(sql != NULL)
		{
			gchar * err_msg = NULL;
			sqlite3_exec(db->db_handle,sql,select_callback,&result,&err_msg);
			if(err_msg != NULL)
			{
				g_printerr("SQL Select error: %s\n",err_msg);
				sqlite3_free(err_msg);
			}
			sqlite3_free(sql);
		}
		g_free(from_argument_list);

		if(*img_url_constr != '\0')
		{
			sqlite3_free(img_url_constr);
		}
	}
	return result;
}

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/

typedef struct
{
    GlyrDatabase * con;
    gint deleted;
    gint max_delete;
} delete_callback_data;

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/

static int delete_callback(void * result, int argc, char ** argv, char ** azColName)
{
	delete_callback_data * data = result;
	if(argc >= 4 && result && data->max_delete > data->deleted)
	{
		/* God, this is so silly.. SQL, why you don't like " = null"
		 * I can't think of any easier way to do this, tell me if you found one
                 */
		gchar * sql_delete = sqlite3_mprintf(
			"DELETE FROM metadata WHERE \n"
			"get_type    %s %s AND \n"
			"artist_id   %s %s AND \n"
			"album_id    %s %s AND \n"
			"title_id    %s %s AND \n"
			"provider_id %s %s;\n",
			argv[0] ? "=" : " IS ", argv[0] ? argv[0] : "NULL",
			argv[1] ? "=" : " IS ", argv[1] ? argv[1] : "NULL",
			argv[2] ? "=" : " IS ", argv[2] ? argv[2] : "NULL",
			argv[3] ? "=" : " IS ", argv[3] ? argv[3] : "NULL",
			argv[4] ? "=" : " IS ", argv[4] ? argv[4] : "NULL");

		if(sql_delete != NULL)
		{
			execute(data->con,sql_delete);
			sqlite3_free(sql_delete);
			data->deleted++;
		}
	}
	return 0;
}

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/

gint glyr_db_delete(GlyrDatabase * db, GlyrQuery * query)
{
	gint result = 0;
	if(db && query)
	{
		GLYR_FIELD_REQUIREMENT reqs = get_req(query);
		gchar * artist_constr = ((reqs & GLYR_REQUIRES_ARTIST) == 0) ? "OR artist_name is null" : "";
		gchar * album_constr  = ((reqs & GLYR_REQUIRES_ALBUM ) == 0) ? "OR album_name  is null" : "";
		gchar * title_constr  = ((reqs & GLYR_REQUIRES_TITLE ) == 0) ? "OR title_name  is null" : "";
		gchar * from_argument_list = convert_from_option_to_sql(query);

		gchar * img_url_constr = "";
		if(TYPE_IS_IMAGE(query->type))
		{
			if(query->download == FALSE)
			{
				img_url_constr = sqlite3_mprintf("AND data_type = %d ", GLYR_TYPE_IMG_URL);
			}
			else
			{
				img_url_constr = sqlite3_mprintf("AND NOT data_type = %d ", GLYR_TYPE_IMG_URL);
			}
		}


		gchar * sql = sqlite3_mprintf(
			"SELECT DISTINCT get_type,artist_id,album_id,title_id,provider_id FROM metadata AS m \n"
			"LEFT JOIN (SELECT rowid FROM artists AS a WHERE artist_name = '%q' %s)  \n"
			"LEFT JOIN (SELECT rowid FROM albums  AS b WHERE album_name  = '%q' %s)  \n"
			"LEFT JOIN (SELECT rowid FROM titles  AS t WHERE title_name  = '%q' %s)  \n"
			"INNER JOIN (SELECT rowid FROM providers AS p WHERE provider_name IN(%s))\n"
			"WHERE get_type = %d %s\n"
			";\n",
			query->artist, artist_constr,
			query->album, album_constr,
			query->title, title_constr,
			from_argument_list,
			query->type, img_url_constr
		       );

		if(sql != NULL)
		{
			delete_callback_data cb_data;
			cb_data.con = db;
			cb_data.deleted = 0;
			cb_data.max_delete = query->number;

			gchar * err_msg = NULL;
			sqlite3_exec(db->db_handle,sql,delete_callback,&cb_data,&err_msg);
			if(err_msg != NULL)
			{
				g_printerr("SQL Delete error: %s\n",err_msg);
				sqlite3_free(err_msg);
			}
			sqlite3_free(sql);
			result = cb_data.deleted;
		}
		g_free(from_argument_list);

		if(*img_url_constr)
		{
			sqlite3_free(img_url_constr);
		}
	}
	return result;
}

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
