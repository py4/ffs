#include <stdio.h>
#include "utfs.h"
int main()
{
	Storage strg;
	int result = init_storage(&strg,1,800000);
	result = store_file(&strg,"utfs.pdf",78);
	result = retrieve_file(&strg,"utfs.pdf",78);
	printf("god: %d\n",result);
	destroy_directory(&strg,strg.root_directory);
	list_dir(&strg,NULL);
	//printf("%d\n",result);
	//dump_storage(&strg,"pooya_result");
	//retrieve_file(&strg,"utfss.pdf",78);
	return 0;
}
