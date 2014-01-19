#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vector.h" /* THIS IS MY CUSTOM VECTOR AND "NOT THAT VECTOR HEAD" */
#include "FS-encryption.h"
#include "sector.h"

#ifndef __UTFS__
#define __UTFS__
#define _LEVEL_3

typedef struct file File;
typedef struct directory Directory;

struct file {
	Sector* sector_start;
	Sector* sector_end;
	char file_name[100];
	int file_size;
	int index_in_parent_directory;
};

struct directory {
	Directory* parent_directory;
	vector child_directories;
	vector child_files;
	char directory_name[100];
	int index_in_parent_directory;
};

typedef struct storage {
	unsigned int disk_size;
	unsigned int sector_size;
	unsigned int free_space;
	Sector* free_sector_head;
	unsigned int total_sector_count;
	char* disk_start;
	Directory* root_directory;
	Directory* current_directory;
} Storage;

void destroy_sector(Sector* sector_start);
void destroy_directory(Storage* strg,Directory* root_directory);
int init_storage( Storage *strg, unsigned int sector_size, unsigned int disk_size);
void free_storage( Storage *strg);
unsigned int get_file_size(FILE* file);
Sector* previous_free_sector(Storage* strg,Sector* sector);
Sector* next_free_sector(Storage* strg,Sector* sector);
File* get_file(Directory* parent_directory,char* file_name);
Directory* get_directory(Directory* parent_directory,char* directory_name);
int store_file( Storage *strg, char* file_name, unsigned char key);
int retrieve_file( Storage *strg, char* file_name, unsigned char key);
int print_file_sequence( Storage *strg, char* file_name);
void delete_and_add_sector(Sector* sector_tail,Sector* free_sector_head,unsigned int sector_size);
int delete_file( Storage *strg, char* file_name);
int delete_file_in_directory(Storage* strg, char* file_name,Directory* current_directory);
int avail_space( Storage* strg );
void list_this_dir(Directory* directory);
int list_dir( Storage* strg, char* path);
int dump_storage( Storage *strg, char* dname); /* no decryption */

#if defined _LEVEL_3
int change_dir( Storage *strg, char* path);
int make_dir( Storage *strg, char* dir_name);
#endif

#endif
