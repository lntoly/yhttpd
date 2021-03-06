#ifndef HTTP_H__
#define HTTP_H__

#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include <sys/stat.h>

#include "common.h"
#include "buffer.h"
#include "connection.h"
#include "http_page.h"
#include "http_file.h"
#include "setting.h"
#include "fastcgi.h"

#define CR     '\r'
#define LF     '\n'
#define CRLF   "\r\n"

#define HTTP_ETAG_LEN               (16)

#define HTTP_GET     0x001  /* HTTP/1.0 */
#define HTTP_POST    0x002  /* HTTP/1.0 */
#define HTTP_HEAD    0x004  /* HTTP/1.0 */
#define HTTP_PUT     0x008  /* HTTP/1.0 */
#define HTTP_DELETE  0x010  /* HTTP/1.0 */
#define HTTP_TRACE   0x020
#define HTTP_OPTIONS 0x040
#define HTTP_CONNECT 0x080
#define HTTP_LINK    0x100  /* HTTP/1.0 only */
#define HTTP_UNLINK  0x200  /* HTTP/1.0 only */

#define HTTP09  0
#define HTTP10  1
#define HTTP11  2
#define HTTP2   3

/* Content Encoding or Accept Encoding */
#define HTTP_IDENTITY   0x01   /*  don't respond */
#define HTTP_GZIP       0x02
#define HTTP_DEFLATE    0x04
#define HTTP_COMPRESS   0x08

/**
 * Pragma: no-cache
 * HTTP/1.1: if request has Cache-Control: no-cache, SHOULD set it as no-cache
 */
#define HTTP_PRAGMA_UNSET       0
#define HTTP_PRAGMA_NO_CACHE    1

#define HTTP_CACHE_CONTROL_NO_STORE 0x01
#define HTTP_CACHE_CONTROL_NO_CACHE 0x02
#define HTTP_CACHE_CONTROL_PUBLIC   0x04
#define HTTP_CACHE_CONTROL_PRIVATE  0X08

/* language */
#define HTTP_LANG_CN    0x01
#define HTTP_LANG_EN    0x02
#define HTTP_LANG_JP    0x04

/* Transfer Encoding */
#define HTTP_UNCHUNKED 0x00
#define HTTP_CHUNKED   0x01

/* state code */
enum {
    HTTP_200 = 200,     /* OK */
    HTTP_202 = 202,     /* Accept */
    HTTP_206 = 206,     /* Partial Content */

    HTTP_301 = 301,     /* Moved Permanently */
    HTTP_304 = 304,     /* Not Modified */

    HTTP_400 = 400,     /* Bad Request */
    HTTP_403 = 403,     /* Forbidden */
    HTTP_404 = 404,     /* Not Found */
    HTTP_405 = 405,     /* Method Not Allowed */
    HTTP_406 = 406,     /* Not Acceptable, Unsupport Chunked */
    HTTP_413 = 413,     /* Request Entity Too Large */
    HTTP_414 = 414,     /* Request-URI TOo Large */
    HTTP_415 = 415,     /* Unsupported Media Type */
    HTTP_416 = 416,     /* Requested range not statisfiable */

    HTTP_500 = 500,     /* Internal Server Error */
    HTTP_501 = 501,     /* Not Implemented */
    HTTP_505 = 505,     /* HTTP Version not supported */
};

struct http_head_com {
    int cache_control_max_age;
    int file_size;
    int content_length;
    int content_range1, content_range2;

    time_t expires;
    time_t last_modified;
    time_t date;

    string_t update;
    string_t via;
    string_t warning;
    string_t content_md5;
    uint8_t keep_alive;

    unsigned cache_control:4;
    unsigned version:2;
    unsigned transfer_encoding:1;
    unsigned content_language:3;
    unsigned connection:1;
    unsigned pragma:1;
    unsigned allow:10;              /* method */
    unsigned trailer:1;             /* dont support */
    unsigned content_encoding:4;    /* only support identity */
    unsigned content_type:10;       /* mime */
};

struct http_head_req {
    string_t uri;
    string_t uri_params;
    char suffix[SUFFIX_LEN];
    int suffix_len;
    string_t query;

    string_t accept_charset;
    //string_t authorization;
    //string_t expect;
    string_t from;
    string_t host;
    time_t if_modified_since;
    string_t if_none_match;
    //string_t if_range;
    time_t if_unmodified_since;
    //int max_forwards;
    int range1, range2;
    //string_t proxy_authorication;
    string_t referer;
    //string_t te;
    string_t user_agent;
    string_t cookie;
    string_t origin;

    unsigned method:10;
    unsigned accept:2;
    unsigned accept_encoding:4; /* content_encoding */
    unsigned accept_language:3;
    uint16_t port;
};

struct http_head_res {
    int16_t code;

    //string_t accept_ranges;
    //int age;
    char etag[HTTP_ETAG_LEN+1];
    string_t location;
    //string_t proxy_authoricate;
    //string_t proxy_after;
    string_t server;
    //string_t vary;
    //string_t www_authenticate;
    string_t set_cookie;    /* TODO set_cookie may be not only one field */
};

/* a request struct */
typedef struct http_request_t {
    struct http_head_com hdr_com;           /* the common portion of head */
    struct http_head_req hdr_req;           /* the rest head of request */
    struct http_head_res hdr_res;           /* the rest head of response */

    buffer_t *hdr_buffer;
    buffer_t *ety_buffer;                   /* buffer of request entity */
    buffer_t *res_buffer;                   /* buffer of response */
    off_t pos, last;

    union {
        http_file_t file;
        http_page_t const *page;        /* cache page or error page */
        http_fastcgi_t fcgi;
    } backend;

    char *parse_p;                      /* current position at the buffer */
    char *parse_pos;
    int parse_state;                    /* state of parsing */
    uint8_t hdr_type;
    uint8_t hdr_buffer_large:1;
} http_request_t;

extern int http_init();
extern void http_exit();
extern http_request_t *http_request_malloc();
extern void http_request_free(http_request_t *r);
extern void http_request_destroy(http_request_t *r);
extern void http_request_reuse(http_request_t *r);
extern void http_request_init(http_request_t *r);
extern int http_request_extend_hdr_buffer(http_request_t *r);

extern void http_print_request(http_request_t *r);

extern int http_check_request(http_request_t *r);
extern int http_build_response_head(http_request_t *r);
extern void http_init_error_page(event_t *ev);
extern void http_init_response(event_t *ev, struct setting_static *sta);
extern void http_generate_etag(string_t const *url, time_t ctime, size_t size,
        char *out, int *out_n);
extern int http_chunk_transmit_over(char const *start, char const *end);
extern void http_fastcgi_respond(event_t *ev, struct setting_fastcgi *fcgi);
extern void http_fastcgi_build(http_request_t *r);
extern void http_fastcgi_build_extend_head(http_request_t *r);
extern int http_fastcgi_parse_response(http_request_t *r, char const *s, char const *e);

typedef uint64_t http_dispatcher_t;
extern http_dispatcher_t http_dispatch(http_request_t *r);
#define HTTP_DISPATCHER(method, code)   (((code)<<10)|(method))
#define HTTP_DISPATCHER_METHOD(dp)      ((dp)&0x03ff)
#define HTTP_DISPATCHER_CODE(dp)        (((dp)&0x03ff)>>10)

#endif /* HTTP_H__ */
