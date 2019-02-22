#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "libpmemkv.h"

#define LOG(msg) printf("[KVTree-Test] %s\n", msg)
#define MAX_VAL_LEN 64

KVEngine* kv;

void StartFailureCallback(void* context, const char* engine, const char* config, const char* msg) {
    printf("ERROR: %s\n", msg);
    exit(-1);
}

// Assumes a poool of size 100MB
void engine_start(char * path) {
        char *config_str = (char*)malloc(100 * sizeof(char));
        sprintf(config_str, "{\"path\":\"%s\", \"size\":104857600}",path );
        kv = kvengine_start(NULL, "kvtree3", config_str, &StartFailureCallback);
}

// Consistently set a value to a key
void kv_put(char* path) {
    	LOG("Starting engine");
	engine_start(path);
    	LOG("Putting new key");
    	char* key1 = "key1";
    	char* value1 = "value1";
    	KVStatus s = kvengine_put(kv, strlen(key1), key1, strlen(value1), value1);
    	assert(s == OK && kvengine_count(kv) == 1);
    	LOG("Stopping engine");
    	kvengine_stop(kv);
}

// Check the value of a key
int check_consistency(char * path) {
	int check = 1;
	LOG("Starting engine");
	engine_start(path);
	LOG("Start check");
	char val[MAX_VAL_LEN];
	char* key1 = "key1";
	char* value1 = "value1";
	KVStatus s = kvengine_get_copy(kv, strlen(key1), key1, MAX_VAL_LEN, val);
	//assert(s == OK);
	FILE *f;
	f = fopen("pmreorder.log", "a");

	if (!strcmp(val, value1)){
		check = 0;
		LOG("Consistent");
		printf("Value : %s\n", val);
		fprintf(f, "\nConsistent : %s\n", val);
	}
	else {
		check = 1;
		LOG("INCONSISTENT");
		printf("Value : %s\n", val);
		fprintf(f, "\nINCONSISTENT : %s\n", val);
	}
	LOG("Stopping engine");
        kvengine_stop(kv);
	LOG("----------------------------------");
	fclose(f);
	return check; 
}

void create_pool(char * path){
	engine_start(path);
	kvengine_stop(kv);
}


int main(int argc, char* argv[]){
	
	/*
	* Opt k = Creates a new pool at the given path
	* Opt p = Writes a consistent value to key using KVTree engine
	* Opt c = Checks if the value is correctly set for the key
	*/
	if (argc != 3 || strchr("ckp", argv[1][0]) == NULL || argv[1][1] !='\0'){
		LOG("Usage : ./kvtree <c|k|p> <path to kv pool>");
		exit(1);
	}

	// Clear off any existing pool by the same name, if we are creating one
	char opt = argv[1][0];
	if (strchr("k", opt)) {
		if( access( argv[2], F_OK ) != -1 ) {
			int ret = remove(argv[2]);
			if (ret == 0)
				LOG("Existing pool cleared");
			else {
				printf("ERROR: %s", strerror(ret));
				exit(1);
			}
		}
	}

	// asuume its inconsistent 
	int check = 1;

	switch (opt) {

		case 'k':
			create_pool(argv[2]);
			break;
 		case 'p':
			kv_put(argv[2]);
			break;
		case 'c':
			check = check_consistency(argv[2]);
			return check;
		default:
			LOG("Unrecognized option");
			abort();
	}	
	return 0;
}
