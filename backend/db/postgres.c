#include <julea-config.h>
#include <glib.h>
#include <gmodule.h>
#include <libpq-fe.h>
#include <julea.h>
#include <julea-db.h>
#include <db-util/jbson.h>
#include <db-util/sql-generic.h>

#define MAX_BUF_SIZE 4096

struct JPostgreSQLData
{
	gchar* db_host;
	gchar* db_database;
	gchar* db_user;
	gchar* db_password;
};

typedef struct JPostgreSQLData JPostgreSQLData;

struct pg_stmt_wrapper
{
	PGconn* conn;
	PGresult* res;
	gchar* query;
	gboolean active;
};

typedef struct pg_stmt_wrapper pg_stmt_wrapper;

static gboolean
j_sql_finalize(gpointer backend_db, void* _stmt, GError** error)
{
	J_TRACE_FUNCTION(NULL);
	pg_stmt_wrapper* wrapper = _stmt;

	g_return_val_if_fail(backend_db != NULL, FALSE);
	g_return_val_if_fail(_stmt != NULL, FALSE);

	if (wrapper->res != NULL)
	{
		PQclear(wrapper->res);
	}

	g_free(wrapper->query);
	g_free(wrapper);

	return TRUE;
}

static gboolean
j_sql_prepare(gpointer backend_db, const char* sql, void* _stmt, GArray* types_in, GArray* types_out, GError** error)
{
	J_TRACE_FUNCTION(NULL);
	pg_stmt_wrapper** _wrapper = _stmt;
	pg_stmt_wrapper* wrapper;

	g_return_val_if_fail(backend_db != NULL, FALSE);
	g_return_val_if_fail(sql != NULL, FALSE);
	g_return_val_if_fail(_stmt != NULL, FALSE);

	wrapper = *_wrapper = g_new0(pg_stmt_wrapper, 1);
	wrapper->conn = backend_db;
	wrapper->query = g_strdup(sql);

	return TRUE;
}

static gboolean
j_sql_bind_null(gpointer backend_db, void* _stmt, guint idx, GError** error)
{
	J_TRACE_FUNCTION(NULL);

	// PostgreSQL does not have a direct bind null function like SQLite or MySQL.
	// Instead, we handle null values during the execution.

	return TRUE;
}

static gboolean
j_sql_bind_value(gpointer backend_db, void* _stmt, guint idx, JDBType type, JDBTypeValue* value, GError** error)
{
	J_TRACE_FUNCTION(NULL);
	pg_stmt_wrapper* wrapper = _stmt;

	// PostgreSQL binding happens in the execution function since libpq does not support direct binding like MySQL/SQLite.
	// Instead, use PQexecParams and handle parameters as part of query execution.

	return TRUE;
}

static gboolean
j_sql_step(gpointer backend_db, void* _stmt, gboolean* found, GError** error)
{
	J_TRACE_FUNCTION(NULL);
	pg_stmt_wrapper* wrapper = _stmt;

	// Execute the query and fetch the result
	wrapper->res = PQexec(wrapper->conn, wrapper->query);
	if (PQresultStatus(wrapper->res) != PGRES_TUPLES_OK)
	{
		g_set_error(error, J_BACKEND_SQL_ERROR, J_BACKEND_SQL_ERROR_STEP, "SQL step failed: %s", PQerrorMessage(wrapper->conn));
		return FALSE;
	}

	*found = (PQntuples(wrapper->res) > 0);

	return TRUE;
}

static gboolean
j_sql_exec(gpointer backend_db, const char* sql, GError** error)
{
	J_TRACE_FUNCTION(NULL);
	PGconn* conn = backend_db;
	PGresult* res;

	g_return_val_if_fail(backend_db != NULL, FALSE);
	g_return_val_if_fail(sql != NULL, FALSE);

	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		g_set_error(error, J_BACKEND_SQL_ERROR, J_BACKEND_SQL_ERROR_STEP, "SQL execution failed: %s", PQerrorMessage(conn));
		PQclear(res);
		return FALSE;
	}
	PQclear(res);
	return TRUE;
}

static gboolean
j_sql_reset(gpointer backend_db, void* _stmt, GError** error)
{
	J_TRACE_FUNCTION(NULL);
	pg_stmt_wrapper* wrapper = _stmt;

	// Reset the statement
	if (wrapper->res != NULL)
	{
		PQclear(wrapper->res);
		wrapper->res = NULL;
	}

	wrapper->active = FALSE;

	return TRUE;
}

static gboolean
j_sql_column(gpointer backend_db, void* _stmt, guint idx, JDBType type, JDBTypeValue* value, GError** error)
{
	J_TRACE_FUNCTION(NULL);
	pg_stmt_wrapper* wrapper = _stmt;
	PGresult* res = wrapper->res;

	g_return_val_if_fail(res != NULL, FALSE);

	memset(value, 0, sizeof(*value));

	switch (type)
	{
		case J_DB_TYPE_SINT32:
			value->val_sint32 = atoi(PQgetvalue(res, 0, idx));
			break;
		case J_DB_TYPE_UINT32:
			value->val_uint32 = (guint32)strtoul(PQgetvalue(res, 0, idx), NULL, 10);
			break;
		case J_DB_TYPE_SINT64:
			value->val_sint64 = atoll(PQgetvalue(res, 0, idx));
			break;
		case J_DB_TYPE_UINT64:
			value->val_uint64 = (guint64)strtoull(PQgetvalue(res, 0, idx), NULL, 10);
			break;
		case J_DB_TYPE_FLOAT32:
			value->val_float32 = (gfloat)atof(PQgetvalue(res, 0, idx));
			break;
		case J_DB_TYPE_FLOAT64:
			value->val_float64 = atof(PQgetvalue(res, 0, idx));
			break;
		case J_DB_TYPE_STRING:
			value->val_string = g_strdup(PQgetvalue(res, 0, idx));
			break;
		case J_DB_TYPE_BLOB:
			value->val_blob = PQgetvalue(res, 0, idx);
			value->val_blob_length = PQgetlength(res, 0, idx);
			break;
		case J_DB_TYPE_ID:
		default:
			g_set_error_literal(error, J_BACKEND_SQL_ERROR, J_BACKEND_SQL_ERROR_INVALID_TYPE, "SQL invalid type");
			return FALSE;
	}

	return TRUE;
}

static void*
j_sql_open(gpointer backend_data)
{
	J_TRACE_FUNCTION(NULL);
	JPostgreSQLData* bd = backend_data;
	PGconn* conn;

	conn = PQsetdbLogin(bd->db_host, "5432", NULL, NULL, bd->db_database, bd->db_user, bd->db_password);
	if (PQstatus(conn) != CONNECTION_OK)
	{
		fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
		PQfinish(conn);
		return NULL;
	}

	return conn;
}

static void
j_sql_close(gpointer backend_db)
{
	J_TRACE_FUNCTION(NULL);
	PGconn* conn = backend_db;

	if (conn != NULL)
	{
		PQfinish(conn);
	}
}

static gboolean
j_sql_start_transaction(gpointer backend_db, GError** error)
{
	J_TRACE_FUNCTION(NULL);
	return j_sql_exec(backend_db, "BEGIN", error);
}

static gboolean
j_sql_commit_transaction(gpointer backend_db, GError** error)
{
	J_TRACE_FUNCTION(NULL);
	return j_sql_exec(backend_db, "COMMIT", error);
}

static gboolean
j_sql_abort_transaction(gpointer backend_db, GError** error)
{
	J_TRACE_FUNCTION(NULL);
	return j_sql_exec(backend_db, "ROLLBACK", error);
}

static JSQLSpecifics specifics = {
	.single_threaded = FALSE,
	.backend_data = NULL,
	.func = {
		.connection_open = j_sql_open,
		.connection_close = j_sql_close,
		.transaction_start = j_sql_start_transaction,
		.transaction_commit = j_sql_commit_transaction,
		.transaction_abort = j_sql_abort_transaction,
		.statement_prepare = j_sql_prepare,
		.statement_finalize = j_sql_finalize,
		.statement_bind_null = j_sql_bind_null,
		.statement_bind_value = j_sql_bind_value,
		.statement_step = j_sql_step,
		.statement_step_and_reset_check_done = j_sql_step_and_reset_check_done,
		.statement_reset = j_sql_reset,
		.statement_column = j_sql_column,
		.sql_exec = j_sql_exec,
	},
	.sql = {
		.autoincrement = " SERIAL ",
		.uint64_type = " BIGINT ",
		.id_type = " BIGINT ",
		.select_last = " SELECT lastval() ",
		.quote = "\"",
	},
};

static gboolean
backend_init(gchar const* path, gpointer* backend_data)
{
	J_TRACE_FUNCTION(NULL);
	JPostgreSQLData* bd;
	g_auto(GStrv) split = NULL;

	g_return_val_if_fail(path != NULL, FALSE);

	split = g_strsplit(path, ":", 0);

	if (g_strv_length(split) != 4)
	{
		return FALSE;
	}

	bd = g_new(JPostgreSQLData, 1);
	bd->db_host = g_strdup(split[0]);
	bd->db_database = g_strdup(split[1]);
	bd->db_user = g_strdup(split[2]);
	bd->db_password = g_strdup(split[3]);

	g_return_val_if_fail(bd->db_host != NULL, FALSE);
	g_return_val_if_fail(bd->db_database != NULL, FALSE);
	g_return_val_if_fail(bd->db_user != NULL, FALSE);
	g_return_val_if_fail(bd->db_password != NULL, FALSE);

	*backend_data = bd;
	specifics.backend_data = bd;

	sql_generic_init(&specifics);

	return TRUE;
}

static void
backend_fini(gpointer backend_data)
{
	J_TRACE_FUNCTION(NULL);
	JPostgreSQLData* bd = backend_data;

	sql_generic_fini();

	g_free(bd->db_host);
	g_free(bd->db_database);
	g_free(bd->db_user);
	g_free(bd->db_password);
	g_free(bd);
}

static JBackend postgres_backend = {
	.type = J_BACKEND_TYPE_DB,
	.component = J_BACKEND_COMPONENT_CLIENT,
	.flags = 0,
	.db = {
		.backend_init = backend_init,
		.backend_fini = backend_fini,
		.backend_schema_create = sql_generic_schema_create,
		.backend_schema_get = sql_generic_schema_get,
		.backend_schema_delete = sql_generic_schema_delete,
		.backend_insert = sql_generic_insert,
		.backend_update = sql_generic_update,
		.backend_delete = sql_generic_delete,
		.backend_query = sql_generic_query,
		.backend_iterate = sql_generic_iterate,
		.backend_batch_start = sql_generic_batch_start,
		.backend_batch_execute = sql_generic_batch_execute,
	},
};

G_MODULE_EXPORT JBackend*
backend_info(void)
{
	J_TRACE_FUNCTION(NULL);
	return &postgres_backend;
}
