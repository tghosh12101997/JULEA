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
#include <jbackground-operation.h>
#include "benchmark.h"

gint benchmark_background_operation_counter;

static gpointer
on_background_operation_completed(gpointer data)
{
    (void)data;
    g_atomic_int_inc(&benchmark_background_operation_counter);
    return NULL;
}

static void
benchmark_background_operation_new_ref_unref(BenchmarkRun* run)
{
    guint const n = 10000; // Number of operations you want to perform
    JBackgroundOperation* background_operation;
    GTimer* op_timer = g_timer_new(); // Timer for each operation

    // Ensure that the timing array is initialized in the BenchmarkRun structure
    if (run->timings == NULL) {
        run->timings = g_array_new(FALSE, FALSE, sizeof(gdouble));
    } else {
        g_array_set_size(run->timings, 0); // Reset the array for reuse
    }

    j_benchmark_timer_start(run);

    for (guint iter = 0; iter < run->iterations; iter++) // Fixed number of iterations
    {
        g_atomic_int_set(&benchmark_background_operation_counter, 0);

        for (guint i = 0; i < n; i++)
        {
            g_timer_start(op_timer); // Start the timer for the individual operation
            background_operation = j_background_operation_new(on_background_operation_completed, NULL);
            j_background_operation_unref(background_operation);
            gdouble elapsed_time = g_timer_elapsed(op_timer, NULL);
            g_timer_stop(op_timer); // Stop the timer for the individual operation
            g_array_append_val(run->timings, elapsed_time); // Store the elapsed time
        }

        // Wait for all operations to complete
        while ((guint64)g_atomic_int_get(&benchmark_background_operation_counter) != n)
        {
        }
    }

    j_benchmark_timer_stop(run);
    g_timer_destroy(op_timer); // Clean up the timer

    run->operations = n * run->iterations; // Total number of operations performed
}

void
benchmark_background_operation(void)
{
    j_benchmark_add("/background-operation/new", benchmark_background_creation_new_ref_unref);
}

