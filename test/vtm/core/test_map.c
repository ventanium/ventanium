/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <stdlib.h>
#include <stdio.h>
#include <vtm/core/map.h>

#define VTM_MAP_COUNT       20

static void test_map(void)
{
	int i;
	bool removed;
	vtm_map *map;

	/* allocation */
	map = vtm_map_new(VTM_ELEM_INT, VTM_ELEM_INT, 8);
	VTM_TEST_ASSERT(map != NULL, "map allocated");

	/* initial fill */
	for (i=0; i < VTM_MAP_COUNT; i++) {
		vtm_map_put_va(map, i, i);
	}
	VTM_TEST_CHECK(vtm_map_size(map) == VTM_MAP_COUNT, "map size check");

	/* overwriting existing keys */
	for (i=0; i < VTM_MAP_COUNT; i++) {
		vtm_map_put(map, VTM_MK_INT(i), VTM_MV_INT(i*2));
	}
	VTM_TEST_CHECK(vtm_map_size(map) == VTM_MAP_COUNT, "map size check");

	/* removal va */
	removed = vtm_map_remove_va(map, 0);
	VTM_TEST_CHECK(removed == true, "map remove va");
	VTM_TEST_CHECK(vtm_map_size(map) == VTM_MAP_COUNT - 1, "map size check after removal");

	/* removal */
	removed = vtm_map_remove(map, VTM_MK_INT(1));
	VTM_TEST_CHECK(removed == true, "map remove");
	VTM_TEST_CHECK(vtm_map_size(map) == VTM_MAP_COUNT - 2, "map size check after removal");

	/* release */
	vtm_map_free(map);
	VTM_TEST_PASSED("map free");
}

static void test_free_func(void)
{
	unsigned int i;
	double *val;
	vtm_map *map;

	map = vtm_map_new(VTM_ELEM_INT, VTM_ELEM_POINTER, 8);
	VTM_TEST_ASSERT(map != NULL, "map allocated");

	vtm_map_set_free_func(map, free);

	for (i=0; i < VTM_MAP_COUNT; i++) {
		val = malloc(sizeof(double));
		VTM_TEST_ASSERT(val != NULL, "map value allocated");
		*val = i;
		vtm_map_put_va(map, i, val);
	}

	for (i=0; i < VTM_MAP_COUNT; i++) {
		val = vtm_map_get_pointer_va(map, i);
		VTM_TEST_CHECK(*val == i, "map pointer get");
	}

	vtm_map_free(map);
	VTM_TEST_PASSED("map free");
}

static void test_string_key(void)
{
	vtm_map *map;

	map = vtm_map_new(VTM_ELEM_STRING, VTM_ELEM_INT, 8);
	VTM_TEST_ASSERT(map != NULL, "map allocated");

	vtm_map_put_va(map, "KEY", 1024);
	vtm_map_remove_va(map, "KEY");

	VTM_TEST_CHECK(vtm_map_size(map) == 0, "map size check");

	vtm_map_free(map);
	VTM_TEST_PASSED("map free");
}

extern void test_vtm_core_map(void)
{
	VTM_TEST_LABEL("map");
	test_map();
	test_free_func();
	test_string_key();
}
