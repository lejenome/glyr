#include <stdlib.h>
#include <string.h>

#include "amazon.h"

#include "../types.h"
#include "../core.h"


// Example snippet of what we parse:
/***

<SmallImage>
<URL>
http://ecx.images-amazon.com/images/I/51rnlRwtsiL._SL75_.jpg
</URL>
<Height Units="pixels">75</Height>
<Width Units="pixels">75</Width>
</SmallImage>

<MediumImage>
<URL>
http://ecx.images-amazon.com/images/I/51rnlRwtsiL._SL160_.jpg
</URL>
<Height Units="pixels">160</Height>
<Width Units="pixels">160</Width>
</MediumImage>
<LargeImage>
<URL>
http://ecx.images-amazon.com/images/I/51rnlRwtsiL.jpg
</URL>
<Height Units="pixels">455</Height>
<Width Units="pixels">455</Width>
</LargeImage>

***/


/***

Country settings:

0) US
1) Canada
2) UK
3) France
4) Germany
5) Japan

***/

#define ACCESS_KEY "AKIAJ6NEA642OU3FM24Q"

const char * cover_amazon_url_lng(char LANG_ID)
{
	switch(LANG_ID)
	{	
		case  0: return "http://free.apisigning.com/onca/xml?Service=AWSECommerceService&AWSAccessKeyId="ACCESS_KEY"&Operation=ItemSearch&SearchIndex=Music&ResponseGroup=Images&Keywords=%artist%+%album%\0";
		case  1: return "http://ca.free.apisigning.com/onca/xml?Service=AWSECommerceService&AWSAccessKeyId="ACCESS_KEY"&Operation=ItemSearch&SearchIndex=Music&ResponseGroup=Images&Keywords=%artist%+%album%\0"; 
		case  2: return "http://co.uk.free.apisigning.com/onca/xml?Service=AWSECommerceService&AWSAccessKeyId="ACCESS_KEY"&Operation=ItemSearch&SearchIndex=Music&ResponseGroup=Images&Keywords=%artist%+%album%\0"; 
		case  3: return "http://fr.free.apisigning.com/onca/xml?Service=AWSECommerceService&AWSAccessKeyId="ACCESS_KEY"&Operation=ItemSearch&SearchIndex=Music&ResponseGroup=Images&Keywords=%artist%+%album%\0";
		case  4: return "http://de.free.apisigning.com/onca/xml?Service=AWSECommerceService&AWSAccessKeyId="ACCESS_KEY"&Operation=ItemSearch&SearchIndex=Music&ResponseGroup=Images&Keywords=%artist%+%album%\0";
		case  5: return "http://co.jp.free.apisigning.com/onca/xml?Service=AWSECommerceService&AWSAccessKeyId="ACCESS_KEY"&Operation=ItemSearch&SearchIndex=Music&ResponseGroup=Images&Keywords=%artist%+%album%\0";
		default: return "http://free.apisigning.com/onca/xml?Service=AWSECommerceService&AWSAccessKeyId="ACCESS_KEY"&Operation=ItemSearch&SearchIndex=Music&ResponseGroup=Images&Keywords=%artist%+%album%\0";
	}
}

memCache_t * cover_amazon_parse(cb_object *capo)
{       
         char * find, * endTag; 
         char *tag_ssize = (capo->max == -1 && capo->min == -1) ? "<LargeImage>"   : 
                           (capo->max <  30 && capo->min >= -1) ? "<SwatchImage>"  : 
                           (capo->max <  70 && capo->min >= -1) ? "<SmallImage>"   : 
                           (capo->max < 150 && capo->min >= -1) ? "<MediumImage>"  : 
                                                                  "<LargeImage>"   ; 

         if( (find = strstr(capo->cache->data, tag_ssize)) == NULL) 
         {
                 return NULL;
         }
 
         /* Next two XML tags not relevant */
         nextTag(find); 
         nextTag(find);

         if( (endTag = strstr(find, "</URL>")) == NULL) 
         {
                 return NULL; 
         } 
  
         size_t endTagLen = (size_t)(endTag - find);     
         char *result_url = calloc(endTagLen+1, sizeof(char));
         strncpy(result_url, find, endTagLen); 
         result_url[endTagLen] = '\0';
         
	 memCache_t *result_cache = DL_init();
	 result_cache->data = result_url;
	 result_cache->size = endTagLen;
     return result_cache; 
}
