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
#include <julea.h>
#include <jcache.h>
#include "benchmark.h"

static void
benchmark_cache_get_release(BenchmarkRun* run)
{
    guint const n = 100000;  // This is the fixed number of operations.

    JCache* cache;
    cache = j_cache_new(n);
    GTimer* op_timer;  // Timer for each cache operation.

    // Initialize the timings array for storing individual operation times.
    run->timings = g_array_new(FALSE, FALSE, sizeof(gdouble));

    j_benchmark_timer_start(run);

    while (j_benchmark_iterate(run))
    {
        for (guint i = 0; i < n; i++)
        {
            gpointer buf;

            // Start timing the cache operation.
            op_timer = g_timer_new();
            buf = j_cache_get(cache, 1);  // Simulate getting a cache item.
            j_cache_release(cache, buf);  // Release the cache item.

            // Stop timing and store the elapsed time in the array.
            g_array_append_val(run->timings, g_timer_elapsed(op_timer, NULL));
            g_timer_destroy(op_timer);
        }
    }

    j_benchmark_timer_stop(run);

    // Optionally print all operation times or process them further.
    for (guint i = 0; i < run->timings->len; i++) {
        gdouble time = g_array_index(run->timings, gdouble, i);
        g_print("Operation %u: %.6f seconds\n", i, time);
    }

    g_array_free(run->timings, TRUE);  // Clean up the timings array
    run->timings = NULL;  // Reset pointer to avoid use-after-free

    j_cache_free(cache);

    run->operations = n * 2;  // Count both get and release as separate operations.
}

void
benchmark_cache(void)
{
    j_benchmark_add("/cache/get-release", benchmark_cache_get_release);
}
