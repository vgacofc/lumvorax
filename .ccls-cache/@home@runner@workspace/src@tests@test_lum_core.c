#include "../lum/lum_core.h"
#include <stdio.h>
#include <assert.h>

void test_lum_creation() {
    printf("Testing LUM creation...\n");
    
    lum_t* lum = lum_create(1, 10, 20, LUM_STRUCTURE_LINEAR);
    assert(lum != NULL);
    assert(lum->presence == 1);
    assert(lum->position_x == 10);
    assert(lum->position_y == 20);
    assert(lum->structure_type == LUM_STRUCTURE_LINEAR);
    
    lum_destroy(lum);
    printf("✓ LUM creation test passed\n");
}

void test_lum_group() {
    printf("Testing LUM group operations...\n");
    
    lum_group_t* group = lum_group_create(5);
    assert(group != NULL);
    assert(lum_group_size(group) == 0);
    
    // Add LUMs to group
    for (int i = 0; i < 3; i++) {
        lum_t* lum = lum_create(1, i, 0, LUM_STRUCTURE_LINEAR);
        assert(lum_group_add(group, lum));
        lum_destroy(lum);
    }
    
    assert(lum_group_size(group) == 3);
    
    lum_group_destroy(group);
    printf("✓ LUM group test passed\n");
}

void test_zones() {
    printf("Testing zone operations...\n");
    
    lum_zone_t* zone = lum_zone_create("TestZone");
    assert(zone != NULL);
    assert(lum_zone_is_empty(zone));
    
    // Create a group and add to zone
    lum_group_t* group = lum_group_create(3);
    lum_t* lum = lum_create(1, 0, 0, LUM_STRUCTURE_LINEAR);
    lum_group_add(group, lum);
    lum_destroy(lum);
    
    assert(lum_zone_add_group(zone, group));
    assert(!lum_zone_is_empty(zone));
    
    lum_zone_destroy(zone);
    printf("✓ Zone test passed\n");
}

int main() {
    printf("=== LUM Core Tests ===\n");
    
    test_lum_creation();
    test_lum_group();
    test_zones();
    
    printf("✅ All tests passed!\n");
    return 0;
}