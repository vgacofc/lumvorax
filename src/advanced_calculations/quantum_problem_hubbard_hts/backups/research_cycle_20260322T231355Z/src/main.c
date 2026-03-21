#include "../include/hubbard_hts_module.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    const char* root = (argc > 1) ? argv[1] : ".";
    int cpu_target = 99;
    int mem_target = 99;

    if (argc > 2) cpu_target = atoi(argv[2]);
    if (argc > 3) mem_target = atoi(argv[3]);

    int rc = hubbard_run_campaign(root, cpu_target, mem_target);
    if (rc != 0) {
        fprintf(stderr, "Echec execution campagne Hubbard HTS (rc=%d)\n", rc);
        return rc;
    }
    printf("Execution terminee avec succes.\n");
    return 0;
}
