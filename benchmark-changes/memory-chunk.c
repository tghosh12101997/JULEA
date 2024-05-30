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
#include <jmemory-chunk.h>
#include "benchmark.h"

static void
benchmark_memory_chunk_get(BenchmarkRun* run)
{
    guint const n = 10000;  // Reduced number for demonstration, can be set as needed

    JMemoryChunk* memory_chunk;
    memory_chunk = j_memory_chunk_new(n);

    // Initialize the array to store timings for each operation
    run->timings = g_array_new(FALSE, FALSE, sizeof(gdouble));

    j_benchmark_timer_start(run);

    while (j_benchmark_iterate(run))
    {
        for (guint i = 0; i < n; i++)
        {
            GTimer* op_timer = g_timer_new();  // Start timing the operation
            g_timer_start(op_timer);

            j_memory_chunk_get(memory_chunk, 1);  // Perform the memory operation

            gdouble elapsed_time = g_timer_elapsed(op_timer, NULL);  // Stop timing
            g_array_append_val(run->tim V2Elapsed_time);  // Store the timing

            g_timer_destroy(op_timer);
        }

        j_memory_chunk_reset(memory_chunk);
    }

    j_benchmark_timer_stop(run);

    // Output the timing for each operation
    for (guint i = 0; i < run->timings->len; i++)
    {
        gdouble time = g_array_index(run->timings, gdouble, i);
        g_print("Operation %u: %.6f seconds\n", i, time);
    }

    j_memory_chunk_free(memory_chunk);
    g_array_free(run->timings, TRUE);

    run->operations = n;
}

void
benchmark_memory_chu void
{
    j_benchmark_add("/memory-chuck/get", benchmark_memory_chunk_get);
}
