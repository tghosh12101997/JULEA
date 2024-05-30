#include <julea-config.h>
#include <glib.h>
#include <gmodule.h>
#include <julea.h>
#include <hiredis/hiredis.h>

struct JRedisData {
    gchar* hostname;
    gint port;
    redisContext* context;
};

typedef struct JRedisData JRedisData;

static gboolean
redis_init(gchar const* path, gpointer* backend_data)
{
    J_TRACE_FUNCTION(NULL);

    JRedisData* rd = g_new(JRedisData, 1);
    rd->hostname = g_strdup("127.0.0.1"); // Default hostname
    rd->port = 6379; // Default port

    rd->context = redisConnect(rd->hostname, rd->port);
    if (rd->context == NULL || rd->context->err) {
        if (rd->context) {
            g_warning("Connection error: %s", rd->context->errstr);
            redisFree(rd->context);
        } else {
            g_warning("Connection error: can't allocate redis context");
        }
        g_free(rd->hostname);
        g_free(rd);
        return FALSE;
    }

    *backend_data = rd;
    return TRUE;
}

static void
redis_fini(gpointer backend_data)
{
    J_TRACE_FUNCTION(NULL);

    JRedisData* rd = backend_data;

    if (rd->context != NULL) {
        redisFree(rd->context);
    }

    g_free(rd->hostname);
    g_free(rd);
}

static gboolean
redis_write(gpointer backend_data, const gchar* key, const gchar* value, GError** error)
{
    J_TRACE_FUNCTION(NULL);

    JRedisData* rd = backend_data;
    redisReply* reply = redisCommand(rd->context, "SET %s %s", key, value);
    if (reply == NULL) {
        g_set_error(error, J_BACKEND_ERROR, J_BACKEND_ERROR_WRITE, "Redis write failed");
        return FALSE;
    }
    freeReplyObject(reply);
    return TRUE;
}

static gboolean
redis_read(gpointer backend_data, const gchar* key, gchar** value, GError** error)
{
    J_TRACE_FUNCTION(NULL);

    JRedisData* rd = backend_data;
    redisReply* reply = redisCommand(rd->context, "GET %s", key);
    if (reply == NULL || reply->type == REDIS_REPLY_NIL) {
        g_set_error(error, J_BACKEND_ERROR, J_BACKEND_ERROR_READ, "Redis read failed");
        return FALSE;
    }
    *value = g_strdup(reply->str);
    freeReplyObject(reply);
    return TRUE;
}

static gboolean
redis_delete(gpointer backend_data, const gchar* key, GError** error)
{
    J_TRACE_FUNCTION(NULL);

    JRedisData* rd = backend_data;
    redisReply* reply = redisCommand(rd->context, "DEL %s", key);
    if (reply == NULL) {
        g_set_error(error, J_BACKEND_ERROR, J_BACKEND_ERROR_DELETE, "Redis delete failed");
        return FALSE;
    }
    freeReplyObject(reply);
    return TRUE;
}

static JBackend redis_backend = {
    .type = J_BACKEND_TYPE_KV,
    .component = J_BACKEND_COMPONENT_SERVER,
    .flags = 0,
    .kv = {
        .backend_init = redis_init,
        .backend_fini = redis_fini,
        .backend_create = NULL,
        .backend_delete = redis_delete,
        .backend_open = NULL,
        .backend_close = NULL,
        .backend_status = NULL,
        .backend_sync = NULL,
        .backend_read = redis_read,
        .backend_write = redis_write,
    },
};

G_MODULE_EXPORT
JBackend*
backend_info(void)
{
    return &redis_backend;
}
