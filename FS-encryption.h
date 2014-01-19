#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sector.h"

int encrypt(FILE* input_file, unsigned int key, unsigned int sectorSize,Sector* free_sector);
int decrypt(FILE* output_file, unsigned int key, unsigned int sectorSize,Sector* sector_start,int file_size);
