#include "utfs.h"

void destroy_sector(Sector* sector_start) /* freeing the sectors linked list recursively */
{
	if(sector_start == NULL)
		return;
	
	if(sector_start->next != NULL)
		destroy_sector(sector_start->next);
	printf("removed sector %d\n",sector_start->sector_index);
	free(sector_start);
}

int init_storage(Storage* strg,unsigned int sector_size,unsigned int disk_size)
{
	int i,sector_count;
	Sector* temp_sector;
	strg->disk_size = disk_size;
	strg->sector_size = sector_size;
	strg->free_space = disk_size;
	if(sector_size > disk_size)
		return -1;
	strg->disk_start = (char*)malloc(disk_size); /* allocating that n byte */
	if(strg->disk_start == NULL)
		return -1;
	sector_count = disk_size / sector_size; /* they told us that disk_size will be divisible by sector_size */
	strg->total_sector_count = sector_count;
	strg->free_sector_head = (Sector*)malloc(sizeof(Sector)); /* creating a dummy head */
	if(strg->free_sector_head == NULL)
	{
		free(strg->disk_start);
		strg->disk_start = NULL;
		return -1;
	}
	temp_sector = strg->free_sector_head;
	for(i = 0; i < sector_count; i ++)
	{
		temp_sector->next = (Sector*)malloc(sizeof(Sector)); /* creating a new sector */
		temp_sector->next->data = strg->disk_start + i*sector_size; /* mapping the new sector to the proper offset from data */
		temp_sector->next->sector_index = i;
		temp_sector->next->previous = temp_sector;
		temp_sector = temp_sector->next; /* moving through the linked list */
	}
	temp_sector = NULL; /* setting the tail to NULL */

	strg->root_directory = (Directory*)malloc(sizeof(Directory));
	if(strg->root_directory == NULL)
	{
		free(strg->disk_start);
		strg->disk_start = NULL;
		destroy_sector(strg->free_sector_head);
		return -1;
	}
	strg->root_directory->parent_directory = NULL; /* root has no parent */
	strcpy(strg->root_directory->directory_name,"/");
	vector_init(&strg->root_directory->child_directories,sizeof(Directory*)); /* creating the child directories vector */
	vector_init(&strg->root_directory->child_files,sizeof(File*)); /* creating the child files vector */
	strg->current_directory = strg->root_directory;
	return 0;
}

void destroy_directory(Storage* strg,Directory* root_directory)
{
	int i,count;
	File* temp_file;
	if(root_directory == NULL)
		return;
	count = vector_count(&(root_directory->child_directories));

	for(i = 0; i < count; i++)
		destroy_directory(strg, (Directory*)(vector_get(&(root_directory->child_directories),i)));

	for(i = 0;i < vector_count(&(root_directory->child_files));i++) /* in each directory, try to remove child files first */
	{
		temp_file = (File*)vector_get(&(root_directory->child_files),i);
		delete_file_in_directory(strg,temp_file->file_name,root_directory);
		temp_file = NULL; /* setting NULL for caution */
	}
	
	if(root_directory->parent_directory)
	{
		vector_delete(&(root_directory->parent_directory->child_directories),root_directory->index_in_parent_directory);
		if(root_directory)
			free(root_directory);
	}
	else
		free(root_directory); /* removing the directory */
	return;
}

void free_storage(Storage* strg) /* File,Directories,root,sector,disk_start */
{
	destroy_directory(strg,strg->root_directory); /* [directory -> file -> sector , that n byte] */ 
	free(strg->disk_start);
}

unsigned int get_file_size(FILE* file)
{
	unsigned int size;
	fseek(file , 0 , SEEK_END);
  size = ftell (file);
  rewind(file); /* setting it back to the first of file */
	return size;
}
	
File* get_file(Directory* parent_directory,char* file_name)
{
	int i;
	File* temp;
	for(i = 0; i < vector_count(&(parent_directory->child_files));i++)
		if(strcmp((temp=(File*)vector_get(&(parent_directory->child_files),i))->file_name,file_name) == 0)
			return temp;
	
	return NULL;
}

Directory* get_directory(Directory* current_directory,char* directory_name)
{
	int i;
	Directory* temp;
	for(i = 0;i < vector_count(&current_directory->child_directories);i++)
		if(strcmp((temp = (Directory*)vector_get(&current_directory->child_directories,i))->directory_name,directory_name) == 0)
			return temp;
	return NULL;
}

int store_file(Storage* strg,char* file_name,unsigned char key)
{
	int used_sector;
	unsigned int file_size;
	int i;
	FILE* input_file = fopen(file_name, "rb");
	File* new_file = (File*)malloc(sizeof(File));

	if (input_file == NULL)
		return -2;

	if (strg == NULL || strlen(file_name) == 0)
	{
		fclose(input_file);
		return -1;
	}
		
	if (strg->free_space < (file_size = get_file_size(input_file)))
	{
		fclose(input_file);
		return -4;
	}

	if(get_file(strg->current_directory,file_name) != NULL)
	{
		fclose(input_file);
		return -5;
	}
	used_sector = encrypt(input_file,key,strg->sector_size,strg->free_sector_head->next);
	if(used_sector == -3)
	{
		free(new_file);
		fclose(input_file);
		new_file = NULL;
		return -3;
	}
	strcpy(new_file->file_name,file_name);
	new_file->file_size = file_size;
	
	vector_add(&strg->current_directory->child_files,new_file); /* adding the new file to the vector */
	new_file->index_in_parent_directory = vector_count(&strg->current_directory->child_files) - 1; /* file need index in vector to be deleted */
	if(used_sector == 0) /* is it a empty file? */
	{
		new_file->sector_start = NULL;
		new_file->sector_end = NULL;
	}
	else /* or a not empty file? */
	{
		new_file->sector_start = strg->free_sector_head->next;
		new_file->sector_start->previous = NULL;
	}
	
	new_file->sector_end = new_file->sector_start;
	for(i = 0; i < used_sector-1; i++)
		new_file->sector_end = new_file->sector_end->next; /* setting the end sector file */

	if(new_file->sector_end != NULL)
	{
		strg->free_sector_head->next = new_file->sector_end->next;
		new_file->sector_end->next = NULL; /* file sector termination */
	}
	strg->free_space -= file_size;

	fclose(input_file);
	return used_sector;	
}

int retrieve_file(Storage* strg, char* file_name, unsigned char key)
{
	FILE* output_file = fopen(file_name, "wb");
	File* file_on_heap;
	int temp_size;

	if (strg == NULL)
	{
		fclose(output_file);
		return -1;
	}

	if (strlen(file_name) == 0)
	{
		fclose(output_file);
		return -1;
	}
	if (output_file == NULL)
	{
		fclose(output_file);
		return -2;
	}
		
	if ((file_on_heap = get_file(strg->current_directory, file_name)) == NULL) /* does it exist or not? */
	{
		fclose(output_file);
		return -4;
	}
	if((file_on_heap->sector_start != NULL) && (file_on_heap->sector_end != NULL)) /* if this is not an empty file */
	{
		if (decrypt(output_file, key, strg->sector_size, file_on_heap->sector_start, file_on_heap->file_size) == -3)
		{
			fclose(output_file);
			return -3;
		}
	}
	temp_size = get_file_size(output_file);
	fclose(output_file);
	output_file = NULL;
	return temp_size;
}

int print_file_sequence(Storage* strg,char* file_name)
{
	File* requested_file;
	Sector* temp_sector;
	if(strg == NULL)
		return -1;
	if(strlen(file_name) == 0)
		return -1;
	if((requested_file = get_file(strg->current_directory,file_name)) == NULL) /* does it exist or not? */
		return -2;
	for(temp_sector = requested_file->sector_start; temp_sector != NULL; temp_sector = temp_sector->next) /* moving through the linked list */
	{
		if(temp_sector == NULL) /* for debug and caution */
		{
			printf("BUG: NULL among file sequence!\n");
			return -3; /*  NULL among file sequence?! */
		}
		printf("%d,",temp_sector->sector_index);
	}
	printf("\n");
	return 0;
}

/* it removes a bunch of sectors and add them to the free sectors linked list recursively in reverse order as told in the PDF */
void delete_and_add_sector(Sector* sector_tail,Sector* free_sector_head,unsigned int sector_size)
{
	Sector* temp_sector;
	if(sector_tail == NULL)
		return;
	
	if(sector_tail->previous != NULL)
		delete_and_add_sector(sector_tail->previous,free_sector_head,sector_size);
	temp_sector = free_sector_head->next;
	free_sector_head->next = sector_tail; /* pushing it after the head */
	sector_tail->next = temp_sector;
	memset(sector_tail->data,'\0',sector_size); /* clearing its datas */
}

int delete_file(Storage* strg, char* file_name)
{
	File* requested_file;
	if(strg == NULL)
		return -1;
	if(strlen(file_name) == 0)
		return -1;
	if((requested_file = get_file(strg->current_directory,file_name)) == NULL) /* does it exist or not? */
		return -2;

	delete_and_add_sector(requested_file->sector_end,strg->free_sector_head,strg->sector_size); /* start deleting from the first sector */
	vector_delete(&(strg->current_directory->child_files),requested_file->index_in_parent_directory); /* removing from child files vector */
	strg->free_space += requested_file->file_size; /* add the free space */
	free(requested_file);
	requested_file = NULL;
	return 0;
}

int delete_file_in_directory(Storage* strg,char* file_name, Directory* current_directory)
{
	File* requested_file;
	if(strg == NULL)
		return -1;
	if(strlen(file_name) == 0)
		return -1;
	if((requested_file = get_file(current_directory,file_name)) == NULL) /* does it exist or not? */
		return -2;

	delete_and_add_sector(requested_file->sector_end,strg->free_sector_head,strg->sector_size); /* start deleting from the first sector */
	vector_delete(&(current_directory->child_files),requested_file->index_in_parent_directory); /* removing from child files vector */
	strg->free_space += requested_file->file_size; /* add the free space */
	free(requested_file);
	requested_file = NULL;
	return 0;
}

int avail_space(Storage* strg)
{
	if(strg == NULL)
		return -1;
	return strg->free_space;
}

void list_this_dir(Directory* directory)
{
	int i;
	if(directory == NULL)
		return;
	
	printf("Listing files and directories in %s\n",directory->directory_name);
	printf("Files:\n");
	for(i = 0;i < vector_count(&directory->child_files); i++)
		printf("%s\n",((File*)vector_get(&directory->child_files,i))->file_name);
	printf("Directories:\n");
	for(i = 0;i < vector_count(&directory->child_directories);i++)
		printf("%s\n",((Directory*)vector_get(&directory->child_directories,i))->directory_name);
}

int list_dir(Storage* strg,char* path)
{
	Directory* dir;
	if(strg == NULL)
		return -1;
	
	if(path == NULL) /* we should list the current directory */
	{
		list_this_dir(strg->current_directory);
		return 0;
	}

	if(strcmp(path,"..") == 0) /* should we list the parent? */
	{
		if(strg->current_directory == strg->root_directory) /* there is no parent anymore! */
		 return -2;
	 list_this_dir(strg->current_directory->parent_directory);
	 return 0;
	}
	
	if((dir = get_directory(strg->current_directory,path)) == NULL) /* does it exist in the current directory or not? */
		return -2;
	
	list_this_dir(dir);
	return 0;
}

int dump_storage(Storage* strg,char* dname) /* dumping the storage */
{
	FILE* output_file = fopen(dname,"wb");
	unsigned int i;
	if(output_file == NULL)
		return -1;
	for(i = 0; i < strg->total_sector_count; i++)
		fwrite(strg->disk_start + i*strg->sector_size,sizeof(char),strg->sector_size,output_file); /* calculating the offset and dumping it */

	fclose(output_file);
	output_file = NULL;
	return 0;
}

int change_dir(Storage* strg,char* path)
{
	Directory* dir;
	if(strg == NULL)
		return -1;
	if(strlen(path) == 0)
		return -1;
	if(strcmp(path,"..") == 0)
	{
		if(strg->current_directory == strg->root_directory) /* there is no parent anymore! */
			return -2;
		strg->current_directory = strg->current_directory->parent_directory;
		return 0;
	}
	if((dir = get_directory(strg->current_directory,path)) == NULL)
		return -2;
	strg->current_directory = dir;
	return 0;
}

int make_dir(Storage* strg,char* dir_name)
{
	Directory* dir;
	Directory* new_directory = (Directory*)malloc(sizeof(Directory));
	if(strg == NULL || strlen(dir_name) == 0)
		return -1;
	if((dir = get_directory(strg->current_directory,dir_name)) != NULL)
		return -2;
	strcpy(new_directory->directory_name ,dir_name);
	if(new_directory == NULL)
		return -5; /* for caution */
 	new_directory->parent_directory = strg->current_directory; /* setting the parent directory */
	vector_add(&(strg->current_directory->child_directories),new_directory); /* adding to the child directories vector */
	vector_init(&new_directory->child_directories,sizeof(Directory*)); /* initializing the child directories vector for the new directory */
	vector_init(&new_directory->child_files,sizeof(File*)); /* initializing the child files vector for the new directory */
	new_directory->index_in_parent_directory = vector_count(&(strg->current_directory->child_directories)) - 1;
	return 0;
}












