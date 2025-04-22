#ifndef _HTTP_HANDLE_H_
#define _HTTP_HANDLE_H_

#define RETURN_HTTP_HEAD            "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
#define ERR_RETURN_HTTP_HEAD        "HTTP/1.1 404 OK\r\nTransfer-Encoding: chunked\r\n\r\n"

#define STR_RESULT                  "result"
#define STR_RESINFO                 "info"

#define REQUEST_GET_RESULT		"/getresult"
#define REQUEST_COMMAND			"/operate"
#define REQUEST_SET_SN			"/setsn"


#endif
