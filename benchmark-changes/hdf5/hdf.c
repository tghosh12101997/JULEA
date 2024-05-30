/*
 * JULEA - Flexible storage framework
 * Copyright (C) 2018-2019 Johannes Coym
 * Copyright (C) 2019-2024 Michael Kuhn
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

/**
 * \file
 **/

#include <julea-config.h>
#include <glib.h>
#include <julea.h>
#include "benchmark.h"

#ifdef HAVE_HDF5

#include <julea-hdf5.h>
#include <hdf5.h>

static gchar const* vol_connector = NULL;

static void set_semantics(void) {
    g_autoptr(JSemantics) semantics = NULL;
    semantics = j_benchmark_get_semantics();
    j_hdf5_set_semantics(semantics);
}

static void sync_file(gchar const* path) {
    if (g_strcmp0(vol_connector, "native") == 0) {
        g_autoptr(JSemantics) semantics = NULL;
        semantics = j_benchmark_get_semantics();
        if (j_semantics_get(semantics, J_SEMANTICS_PERSISTENCY) == J_SEMANTICS_PERSISTENCY_NONE) {
            return;
        }
        j_helper_file_sync(path);
    }
}

static void discard_file(gchar const* path) {
    if (g_strcmp0(vol_connector, "native") == 0) {
        j_helper_file_discard(path);
    }
}

static guint dimensions_to_size(guint dimensions) {
    guint size = 1024;
    switch (dimensions) {
        case 1:
            size = 1024;
            break;
        case 2:
            size = 1024 * 1024;
            break;
        default:
            g_assert_not_reached();
    }
    return size;
}

static hid_t create_group(hid_t file, gchar const* name) {
    return H5Gcreate2(file, name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
}

static hid_t open_group(hid_t file, gchar const* name) {
    return H5Gopen2(file, name, H5P_DEFAULT);
}

static void write_attribute(hid_t group, gchar const* name, BenchmarkRun* run) {
    GTimer* op_timer = g_timer_new();
    hid_t attribute, dataspace;
    hsize_t dims[1] = {1024};
    int data[1024];

    for (guint i = 0; i < 1024; i++) {
        data[i] = i * 10;
    }

    dataspace = H5Screate_simple(1, dims, NULL);
    attribute = H5Acreate2(group, name, H5T_NATIVE_INT, dataspace, H5P_DEFAULT, H5P_DEFAULT);

    g_timer_start(op_timer);
    H5Awrite(attribute, H5T_NATIVE_INT, data);
    gdouble elapsed_time = g_timer_elapsed(op_timer, NULL);
    g_array_append_val(run->timings, elapsed_time);

    g_timer_destroy(op_timer);
    H5Sclose(dataspace);
    H5Aclose(attribute);
}

static void read_attribute(hid_t group, gchar const* name, BenchmarkRun* run) {
    GTimer* op_timer = g_timer_new();
    hid_t attribute = H5Aopen(group, name, H5P_DEFAULT);
    hid_t dataspace = H5Aget_space(attribute);
    hssize_t elements = H5Sget_simple_extent_npoints(dataspace);
    int data[1024];

    g_assert_cmpuint(elements, ==, 1024);
    g_timer_start(op_timer);
    H5Aread(attribute, H5T_NATIVE_INT, data);
    gdouble elapsed_time = g_timer_elapsed(op_timer, NULL);
    g_array_append_val(run->timings, elapsed_time);

    g_timer_destroy(op_timer);
    H5Aclose(attribute);
    H5Sclose(dataspace);
}


static hid_t create_dataset(hid_t file, gchar const* name, guint dimensions, hid_t* dataspace_out, BenchmarkRun* run) {
    GTimer* op_timer = g_timer_new();
    hid_t dataset, dataspace;
    g_autofree hsize_t* dims = g_new0(hsize_t, dimensions);

    for (guint i = 0; i < dimensions; i++) {
        dims[i] = 1024;
    }
    dataspace = H5Screate_simple(dimensions, dims, NULL);
    dataset = H5Dcreate2(file, name, H5T_NATIVE_INT, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    if (dataspace_out != NULL) {
        *dataspace_out = dataspace;
    } else {
        H5Sclose(dataspace);
    }

    g_timer_start(op_timer);
    // Assuming some operation on dataset to time here, such as dataset write or modification
    gdouble elapsed_time = g_timer_elapsed(op_timer, NULL);
    g_array_append_val(run->timings, elapsed_time);

    g_timer_destroy(op_timer);
    return dataset;
}

static hid_t
open_dataset(hid_t file, gchar const* name)
{
	hid_t dataset;

	dataset = H5Dopen2(file, name, H5P_DEFAULT);

	return dataset;
}

static void write_dataset(hid_t dataset, guint dimensions, BenchmarkRun* run) {
    GTimer* op_timer = g_timer_new();
    guint size = dimensions_to_size(dimensions);
    g_autofree int* data = g_new(int, size);

    for (guint i = 0; i < size; i++) {
        data[i] = i;
    }

    g_timer_start(op_timer);
    H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    gdouble elapsed_time = g_timer_elapsed(op_timer, NULL);
    g_array_append_val(run->timings, elapsed_time);

    g_timer_destroy(op_timer);
}

static void read_dataset(hid_t dataset, guint dimensions, BenchmarkRun* run) {
    GTimer* op_timer = g_timer_new();
    hid_t dataspace = H5Dget_space(dataset);
    hssize_t elements = H5Sget_simple_extent_npoints(dataspace);
    guint size = dimensions_to_size(dimensions);
    g_autofree int* data = g_new(int, size);

    g_assert_cmpuint(elements, ==, size);
    g_timer_start(op_timer);
    H5Dread(dataset, H5T_NATIVE_INT, dataspace, H5S_ALL, H5P_DEFAULT, data);
    gdouble elapsed_time = g_timer_elapsed(op_timer, NULL);
    g_array_append_val(run->timings, elapsed_time);

    g_timer_destroy(op_timer);
    H5Sclose(dataspace);
}

static void benchmark_hdf_attribute_write(BenchmarkRun* run) {
    set_semantics();
    hid_t file = H5Fcreate("benchmark-attribute-write.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    hid_t group = create_group(file, "attributes");

    j_benchmark_timer_start(run);
    for (guint i = 0; i < 1000; i++) {
        gchar* name = g_strdup_printf("attribute-%u", i);
        write_attribute(group, name, run);
        g_free(name);
    }
    j_benchmark_timer_stop(run);

    H5Gclose(group);
    H5Fclose(file);
    sync_file("benchmark-attribute-write.h5");

    run->operations = 1000;
    run->bytes = 1000 * 1024 * sizeof(int); // Assuming each attribute is an array of 1024 ints.
}

static void benchmark_hdf_attribute_read(BenchmarkRun* run) {
    set_semantics();
    hid_t file = H5Fcreate("benchmark-attribute-read.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    hid_t group = create_group(file, "attributes");

    // Write attributes first to read them later.
    for (guint i = 0; i < 1000; i++) {
        gchar* name = g_strdup_printf("attribute-%u", i);
        write_attribute(group, name, run);
        g_free(name);
    }

    discard_file("benchmark-attribute-read.h5");

    j_benchmark_timer_start(run);
    for (guint i = 0; i < 1000; i++) {
        gchar* name = g_strdup_printf("attribute-%u", i);
        read_attribute(group, name, run);
        g_free(name);
    }
    j_benchmark_timer_stop(run);

    H5Gclose(group);
    H5Fclose(file);

    run->operations = 1000;
    run->bytes = 1000 * 1024 * sizeof(int);
}

static void
_benchmark_hdf_dataset_create(BenchmarkRun* run, guint dimensions)
{
	guint const n = 1000;

	hid_t file;
	guint iter = 0;

	set_semantics();

	file = H5Fcreate("benchmark-dataset-create.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

	j_benchmark_timer_start(run);

	while (j_benchmark_iterate(run))
	{
		for (guint i = 0; i < n; i++)
		{
			hid_t dataset;

			g_autofree gchar* name = NULL;

			name = g_strdup_printf("benchmark-dataset-create-%u", i + (iter * n));
			dataset = create_dataset(file, name, dimensions, NULL);
			H5Dclose(dataset);
		}

		iter++;
	}

	H5Fclose(file);

	sync_file("benchmark-dataset-create.h5");

	j_benchmark_timer_stop(run);

	run->operations = n;
}

static void
benchmark_hdf_dataset_create_4m(BenchmarkRun* run)
{
	_benchmark_hdf_dataset_create(run, 2);
}

static void
benchmark_hdf_dataset_create_4k(BenchmarkRun* run)
{
	_benchmark_hdf_dataset_create(run, 1);
}

static void
_benchmark_hdf_dataset_open(BenchmarkRun* run, guint dimensions)
{
	guint const n = 1000;

	hid_t file;
	guint iter = 0;

	set_semantics();

	file = H5Fcreate("benchmark-dataset-open.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

	while (j_benchmark_iterate(run))
	{
		for (guint i = 0; i < n; i++)
		{
			hid_t dataset;

			g_autofree gchar* name = NULL;

			name = g_strdup_printf("benchmark-dataset-open-%u", i + (iter * n));
			dataset = create_dataset(file, name, dimensions, NULL);
			H5Dclose(dataset);
		}

		discard_file("benchmark-dataset-open.h5");

		j_benchmark_timer_start(run);

		for (guint i = 0; i < n; i++)
		{
			hid_t dataset;

			g_autofree gchar* name = NULL;

			name = g_strdup_printf("benchmark-dataset-open-%u", i + (iter * n));
			dataset = H5Dopen2(file, name, H5P_DEFAULT);
			H5Dclose(dataset);
		}

		j_benchmark_timer_stop(run);

		iter++;
	}

	H5Fclose(file);

	run->operations = n;
}

static void
benchmark_hdf_dataset_open_4m(BenchmarkRun* run)
{
	_benchmark_hdf_dataset_open(run, 2);
}

static void
benchmark_hdf_dataset_open_4k(BenchmarkRun* run)
{
	_benchmark_hdf_dataset_open(run, 1);
}

static void
_benchmark_hdf_dataset_write(BenchmarkRun* run, guint dimensions)
{
	guint const n = 100;

	hid_t file;
	guint iter = 0;

	set_semantics();

	file = H5Fcreate("benchmark-dataset-write.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

	j_benchmark_timer_start(run);

	while (j_benchmark_iterate(run))
	{
		for (guint i = 0; i < n; i++)
		{
			hid_t dataset;
			hid_t dataspace;

			g_autofree gchar* name = NULL;

			name = g_strdup_printf("benchmark-dataset-write-%u", i + (iter * n));
			dataset = create_dataset(file, name, dimensions, &dataspace);
			write_dataset(dataset, dimensions);
			H5Dclose(dataset);
			H5Sclose(dataspace);
		}

		iter++;
	}

	H5Fclose(file);

	sync_file("benchmark-dataset-write.h5");

	j_benchmark_timer_stop(run);

	run->operations = n;
	run->bytes = n * dimensions_to_size(dimensions) * sizeof(int);
}

static void
benchmark_hdf_dataset_write_4m(BenchmarkRun* run)
{
	_benchmark_hdf_dataset_write(run, 2);
}

static void
benchmark_hdf_dataset_write_4k(BenchmarkRun* run)
{
	_benchmark_hdf_dataset_write(run, 1);
}

static void
_benchmark_hdf_dataset_read(BenchmarkRun* run, guint dimensions)
{
	guint const n = 100;

	hid_t file;
	guint iter = 0;

	set_semantics();

	file = H5Fcreate("benchmark-dataset-read.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

	while (j_benchmark_iterate(run))
	{
		for (guint i = 0; i < n; i++)
		{
			hid_t dataset;
			hid_t dataspace;

			g_autofree gchar* name = NULL;

			name = g_strdup_printf("benchmark-dataset-read-%u", i + (iter * n));
			dataset = create_dataset(file, name, dimensions, &dataspace);
			write_dataset(dataset, dimensions);
			H5Dclose(dataset);
			H5Sclose(dataspace);
		}

		discard_file("benchmark-dataset-read.h5");

		j_benchmark_timer_start(run);

		for (guint i = 0; i < n; i++)
		{
			hid_t dataset;

			g_autofree gchar* name = NULL;

			name = g_strdup_printf("benchmark-dataset-read-%u", i + (iter * n));
			dataset = open_dataset(file, name);
			read_dataset(dataset, dimensions);
			H5Dclose(dataset);
		}

		j_benchmark_timer_stop(run);

		iter++;
	}

	H5Fclose(file);

	run->operations = n;
	run->bytes = n * dimensions_to_size(dimensions) * sizeof(int);
}

static void
benchmark_hdf_dataset_read_4m(BenchmarkRun* run)
{
	_benchmark_hdf_dataset_read(run, 2);
}

static void
benchmark_hdf_dataset_read_4k(BenchmarkRun* run)
{
	_benchmark_hdf_dataset_read(run, 1);
}

static void
benchmark_hdf_file_create(BenchmarkRun* run)
{
	guint const n = 1000;

	guint iter = 0;

	set_semantics();

	j_benchmark_timer_start(run);

	while (j_benchmark_iterate(run))
	{
		for (guint i = 0; i < n; i++)
		{
			hid_t file;
			g_autofree gchar* name = NULL;

			name = g_strdup_printf("benchmark-file-create-%u.h5", i + (iter * n));
			file = H5Fcreate(name, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

			H5Fclose(file);

			sync_file(name);
		}

		iter++;
	}

	j_benchmark_timer_stop(run);

	run->operations = n;
}

static void
benchmark_hdf_file_open(BenchmarkRun* run)
{
	guint const n = 1000;

	guint iter = 0;

	set_semantics();

	while (j_benchmark_iterate(run))
	{
		for (guint i = 0; i < n; i++)
		{
			hid_t file;
			g_autofree gchar* name = NULL;

			name = g_strdup_printf("benchmark-file-open-%u.h5", i + (iter * n));
			file = H5Fcreate(name, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
			H5Fclose(file);

			discard_file(name);
		}

		j_benchmark_timer_start(run);

		for (guint i = 0; i < n; i++)
		{
			hid_t file;
			g_autofree gchar* name = NULL;

			name = g_strdup_printf("benchmark-file-open-%u.h5", i + (iter * n));
			file = H5Fopen(name, H5F_ACC_RDWR, H5P_DEFAULT);
			H5Fclose(file);
		}

		j_benchmark_timer_stop(run);

		iter++;
	}

	run->operations = n;
}

static void
benchmark_hdf_group_create(BenchmarkRun* run)
{
	guint const n = 1000;

	hid_t file;
	guint iter = 0;

	set_semantics();

	file = H5Fcreate("benchmark-group-create.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

	j_benchmark_timer_start(run);

	while (j_benchmark_iterate(run))
	{
		for (guint i = 0; i < n; i++)
		{
			hid_t group;

			g_autofree gchar* name = NULL;

			name = g_strdup_printf("benchmark-group-create-%u", i + (iter * n));
			group = create_group(file, name);
			H5Gclose(group);
		}

		iter++;
	}

	H5Fclose(file);

	sync_file("benchmark-group-create.h5");

	j_benchmark_timer_stop(run);

	run->operations = n;
}

static void
benchmark_hdf_group_open(BenchmarkRun* run)
{
	guint const n = 1000;

	hid_t file;
	guint iter = 0;

	set_semantics();

	file = H5Fcreate("benchmark-group-open.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

	while (j_benchmark_iterate(run))
	{
		for (guint i = 0; i < n; i++)
		{
			hid_t group;

			g_autofree gchar* name = NULL;

			name = g_strdup_printf("benchmark-group-open-%u", i + (iter * n));
			group = create_group(file, name);
			H5Gclose(group);
		}

		discard_file("benchmark-group-open.h5");

		j_benchmark_timer_start(run);

		for (guint i = 0; i < n; i++)
		{
			hid_t group;

			g_autofree gchar* name = NULL;

			name = g_strdup_printf("benchmark-group-open-%u", i + (iter * n));
			group = open_group(file, name);
			H5Gclose(group);
		}

		j_benchmark_timer_stop(run);

		iter++;
	}

	H5Fclose(file);

	run->operations = n;
}

#endif

void
benchmark_hdf(void)
{
	/// \todo repeated runs exhibit strange behavior, objects are distributed differently etc. (see #162)
	/// \todo Files should be deleted afterwards instead of being on truncated create
#ifdef HAVE_HDF5
	vol_connector = g_getenv("HDF5_VOL_CONNECTOR");

	if (vol_connector == NULL)
	{
		// Make sure we do not accidentally run benchmarks for native HDF5
		// If comparisons with native HDF5 are necessary, set HDF5_VOL_CONNECTOR to "native"
		return;
	}

	j_benchmark_add("/hdf5/attribute/write", benchmark_hdf_attribute_write);
	j_benchmark_add("/hdf5/attribute/read", benchmark_hdf_attribute_read);
	j_benchmark_add("/hdf5/dataset4M/create", benchmark_hdf_dataset_create_4m);
	j_benchmark_add("/hdf5/dataset4M/open", benchmark_hdf_dataset_open_4m);
	j_benchmark_add("/hdf5/dataset4M/write", benchmark_hdf_dataset_write_4m);
	j_benchmark_add("/hdf5/dataset4M/read", benchmark_hdf_dataset_read_4m);
	j_benchmark_add("/hdf5/dataset4K/create", benchmark_hdf_dataset_create_4k);
	j_benchmark_add("/hdf5/dataset4K/open", benchmark_hdf_dataset_open_4k);
	j_benchmark_add("/hdf5/dataset4K/write", benchmark_hdf_dataset_write_4k);
	j_benchmark_add("/hdf5/dataset4K/read", benchmark_hdf_dataset_read_4k);
	j_benchmark_add("/hdf5/file/create", benchmark_hdf_file_create);
	j_benchmark_add("/hdf5/file/open", benchmark_hdf_file_open);
	j_benchmark_add("/hdf5/group/create", benchmark_hdf_group_create);
	j_benchmark_add("/hdf5/group/open", benchmark_hdf_group_open);
#endif
}
