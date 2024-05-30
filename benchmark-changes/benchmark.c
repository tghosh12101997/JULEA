/*
 * JULEA - Flexible storage framework
 * Copyright (C) 2010-2024 Michael Kuhn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <julea-config.h>
#include <glib.h>
#include <locale.h>
#include <string.h>
#include <julea.h>
#include "benchmark.h"

static gint opt_duration = 1;
static gboolean opt_list = FALSE;
static gchar* opt_machine_separator = NULL;
static gboolean opt_machine_readable = FALSE;
static gchar* opt_path = NULL;
static gchar* opt_semantics = NULL;
static gchar* opt_template = NULL;

static JSemantics* j_benchmark_semantics = NULL;
static GList* j_benchmarks = NULL;
static gsize j_benchmark_name_max = 0;

JSemantics* j_benchmark_get_semantics(void) {
    return j_semantics_ref(j_benchmark_semantics);
}

void j_benchmark_timer_start(BenchmarkRun* run) {
    g_return_if_fail(run != NULL);
    if (!run->timer_started) {
        g_timer_start(run->timer);
        run->timer_started = TRUE;
    } else {
        g_timer_continue(run->timer);
    }
}

void j_benchmark_timer_stop(BenchmarkRun* run) {
    g_return_if_fail(run != NULL);
    g_timer_stop(run->timer);
}

gboolean j_benchmark_iterate(BenchmarkRun* run) {
    g_return_val_if_fail(run != NULL, FALSE);
    if (run->iterations == 0 || g_timer_elapsed(run->timer, NULL) < opt_duration) {
        run->iterations++;
        return TRUE;
    }
    return FALSE;
}

void j_benchmark_add(gchar const* name, BenchmarkFunc benchmark_func) {
    BenchmarkRun* run;
    g_return_if_fail(name != NULL && benchmark_func != NULL);
    run = g_new0(BenchmarkRun, 1);
    run->name = g_strdup(name);
    run->func = benchmark_func;
    run->timer = g_timer_new();
    run->timer_started = FALSE;
    run->iterations = 0;
    run->operations = 0;
    run->bytes = 0;
    run->timings = g_array_new(FALSE, FALSE, sizeof(gdouble));  // Initialize the timings array
    j_benchmarks = g_list_prepend(j_benchmarks, run);
}

static void j_benchmark_run_one(BenchmarkRun* run) {
    g_return_if_fail(run != NULL);

    if (opt_list) {
        g_print("%s\n", run->name);
        return;
    }

    (*run->func)(run, run->timings);  // Use the timings array from the run struct

    for (guint i = 0; i < run->timings->len; i++) {
        gdouble time = g_array_index(run->timings, gdouble, i);
        g_print("Operation %u: %.3f seconds\n", i, time);
    }

    g_array_free(run->timings, TRUE);  // Clean up the timings array
    run->timings = NULL;  // Reset pointer to avoid use-after-free
}

static void j_benchmark_run_all(void) {
    GList* benchmark;
    for (benchmark = j_benchmarks; benchmark != NULL; benchmark = benchmark->next) {
        j_benchmark_run_one(benchmark->data);
    }
    g_list_free_full(j_benchmarks, (GDestroyNotify) g_free);
    j_benchmarks = NULL;
}

int main(int argc, char** argv) {
    GError* error = NULL;
    GOptionContext* context = g_option_context_new(NULL);
    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);

    j_benchmark_semantics = j_semantics_new_from_string(opt_template, opt_semantics);
    // Register benchmarks here

    j_benchmark_run_all();
    j_semantics_unref(j_benchmark_semantics);
    g_free(opt_machine_separator);
    g_free(opt_path);
    g_free(opt_semantics);
    g_free(opt_template);

    return 0;
}
