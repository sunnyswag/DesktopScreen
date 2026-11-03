#ifndef _DS_HTTP_REQUEST_RULES_H_
#define _DS_HTTP_REQUEST_RULES_H_

#include <stddef.h>
#include "http_request.h"

typedef struct
{
    // Returns the required URL length, excluding the NUL terminator, or a negative error.
    int (*gen_url)(char *url, size_t size);
    void (*parse_func)(char *text);
} http_request_rule_t;

// Returns NULL for an unsupported request type.
const http_request_rule_t *ds_http_request_rule_get(HTTP_REQUEST_TYPE_E type);

#endif
