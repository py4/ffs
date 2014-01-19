#ifndef SECTOR_H
#define SECTOR_H

#define true 1
#define false 0

typedef struct sector Sector;
struct sector {
	Sector* next;
	Sector* previous;
	char* data;
	unsigned int sector_index;
	int busy;
};

#endif
