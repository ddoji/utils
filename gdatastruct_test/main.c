#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#include <glib.h>

void __attribute__((constructor)) console_setting_for_eclipse_debugging( void ){
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
}

typedef struct SampleData{
	char *teststr;
	int index;
} SampleData;

static gint GCompareFunc_3(gconstpointer a, gconstpointer b){
	return strcmp(((SampleData*)a)->teststr,((SampleData*)(char*)b)->teststr);
}

static void _free_func(gpointer data){
	SampleData *val = (SampleData*)data;

	if(val){
		if(val->teststr) free(val->teststr);
		free(val);
	}
}

void test_linkedlist() {
	SampleData *struct1, *struct2, *struct3, *struct4;
	// 반드시 NULL로 초기화 필요함.
	GList *struct_list = NULL, *temp;

	// data 생성 : heap malloc by malloc() and strdup()
	struct1 = (SampleData*)malloc(sizeof(SampleData));
	struct2 = (SampleData*)malloc(sizeof(SampleData));
	struct3 = (SampleData*)malloc(sizeof(SampleData));
	struct4 = (SampleData*)malloc(sizeof(SampleData));
	struct1->teststr = strdup("val3");
	struct1->index = 1;
	struct2->teststr = strdup("val2");
	struct2->index = 2;
	struct3->teststr = strdup("val1");
	struct3->index = 3;
	struct4->teststr = strdup("val0");
	struct4->index = 4;

	struct_list = g_list_append(struct_list, struct1);
	struct_list = g_list_append(struct_list, struct2);
	struct_list = g_list_prepend(struct_list, struct3);
	struct_list = g_list_insert(struct_list, struct4, 2);
	//struct_list = g_list_append(struct_list, struct3);

	// 순차 조회
	for(temp=struct_list ; temp ; temp=temp->next){
		printf("구조체 [%s:%d]\n", ((SampleData*)temp->data)->teststr, ((SampleData*)temp->data)->index);
	}
	printf("\n");
	struct_list = g_list_remove(struct_list, struct4);
	for(temp=struct_list ; temp ; temp=temp->next){
		printf("구조체 [%s:%d]\n", ((SampleData*)temp->data)->teststr, ((SampleData*)temp->data)->index);
	}
	printf("\n");

	struct_list = g_list_insert_before (struct_list, struct_list->next, struct4);
	for(temp=struct_list ; temp ; temp=temp->next){
		printf("구조체 [%s:%d]\n", ((SampleData*)temp->data)->teststr, ((SampleData*)temp->data)->index);
	}
	printf("\n");

	// 정렬
	struct_list = g_list_sort(struct_list, GCompareFunc_3);

	// 순차 조회
	for(temp=struct_list ; temp ; temp=temp->next){
		printf("구조체 [%s:%d]\n", ((SampleData*)temp->data)->teststr, ((SampleData*)temp->data)->index);
	}

	//g_list_free(number_list);
	g_list_free_full(struct_list, _free_func);
	struct_list=NULL;
}


void test_queue() {

}

void test_stack() {

}

void test_hashtable() {
	GHashTable *hashTable = g_hash_table_new(g_str_hash, g_str_equal);
	g_hash_table_insert(hashTable, "a", "aaa");
	g_hash_table_insert(hashTable, "b", "bbb");
	g_hash_table_insert(hashTable, "c", "ccc");

	printf("hashtable size : %d\n", g_hash_table_size(hashTable));

	if (g_hash_table_contains(hashTable, "a")) {
		char *val = (char *)g_hash_table_lookup(hashTable, "a");
		printf("a contains : val %s\n", val);

	}
	g_hash_table_replace(hashTable, "a", "abc");
	g_hash_table_remove(hashTable, "b");

	printf("hashtable size : %d\n", g_hash_table_size(hashTable));

	GHashTableIter iter;
	g_hash_table_iter_init (&iter, hashTable);
	char *key, *val;
	while (g_hash_table_iter_next (&iter, (gpointer) &key, (gpointer) &val))
	{
		printf("key %s ---> %s\n", (char *)key, (char *)val);
	}

	GList *temp;
	GList *list = g_hash_table_get_keys(hashTable);
	for(temp=list ; temp ; temp=temp->next){
		printf("key [%s]\n", (char*)temp->data);
	}
	list = g_hash_table_get_values(hashTable);
	for(temp=list ; temp ; temp=temp->next){
		printf("val [%s]\n", (char*)temp->data);
	}

	g_hash_table_destroy(hashTable);
}

int main(int argc, char *argv[]){
	printf("===LINKED LIST===\n");
	test_linkedlist();


	printf("===STACK===\n");
	test_stack();

	printf("===QUEUE===\n");
	test_queue();

	printf("===hash map===\n");
	test_hashtable();
	return 0;
}

