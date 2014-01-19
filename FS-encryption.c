#include "FS-encryption.h"
#include <stdint.h>

uint8_t left_rotate(uint8_t n)
{
	uint8_t result = (n << 1) | (n >> 7); /* direct formula for circular rotate! */
	return result;
}

uint8_t right_rotate(uint8_t n)
{
	return (n >> 1) | (n << 7);
}

int first_byte(int n)
{
	return n & 0xFF; /* getting the first byte of the key */
}

uint8_t encrypt_byte(int byte,uint8_t key,int sector_index)
{
	int result = byte ^ key; /* XORing key with byte */
	if((key & 1) == (sector_index & 1)) /* in the table, it turned out that if they are both even are odd then left rotate is needed */
		return left_rotate(result);
	else
		return right_rotate(result);
}

uint8_t decrypt_byte(int byte,uint8_t key, int sector_index)
{
	uint8_t result;
	if((key & 1) == (sector_index & 1))
		result = right_rotate(byte);
	else
		result = left_rotate(byte);
	return result^key;
}

int encrypt(FILE* input_file, unsigned int key, unsigned int sectorSize, Sector* free_sector)
{
	int temp;
	char t;
	char* buffer;
	Sector* temp_sector = free_sector;
	int i,count=0;
	
	buffer = (char*)malloc(sectorSize);
	if(buffer == NULL)
		return -3;
	
	while((temp = fread(buffer,1,sectorSize,input_file)) > 0) /* reading from the file */
	{
		for(i = 0;i < temp;i++) /* encryping byte by byte */
		{
			t = encrypt_byte((int)buffer[i],first_byte(key),temp_sector->sector_index);
			buffer[i] = t;
		}
		memcpy(temp_sector->data,buffer,temp); /* copying the encrypted bytes to the sector's data */
		count++;
		if(temp_sector->next)
			temp_sector->next->previous = temp_sector;
		temp_sector = temp_sector->next; /* moving to the next free sector */
	}
	free(buffer);
	return count;
}

int decrypt(FILE* output_file, unsigned int key, unsigned int sectorSize, Sector* sector_start,int file_size)
{
	char t;
	char* buffer;
	unsigned int i,sector_index = 0;
	Sector* temp_sector = sector_start;
	int sector_count = 0;
	buffer = (char*)malloc(sectorSize); 
	if(buffer == NULL)
		return -3;
	for(temp_sector = sector_start;temp_sector != NULL; temp_sector = temp_sector->next) /* moving through the linked list */
	{
		sector_count++;
		if(temp_sector == NULL) /* for caution and debugging */
		{
			printf("BUG: NULL in file sector with index %d!\n",temp_sector->sector_index);
			return -3;
		}
		memcpy(buffer,temp_sector->data,sectorSize);
		for(i = 0; i < sectorSize ; i++) /* decrypting byte by byte */
		{
			t = decrypt_byte((int)buffer[i],first_byte(key),sector_index);
			buffer[i] = t;
		}
		if(temp_sector->next == NULL) /* in the last sector we should not decrypting the whole datas of the sector */
			fwrite(buffer,file_size % sectorSize,1,output_file); /* just to the end of file */
		else
			fwrite(buffer,sectorSize,1,output_file); /* not the last sector and should decrypt the whole datas of the sector */
		
		sector_index++;
	}
	free(buffer);
	return 0;
}
