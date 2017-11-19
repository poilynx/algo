#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

typedef void(*htable_clean_callback)(void*);

struct node_struct {
	char* key;
	void* value;
	struct node_struct * next;
};

typedef struct node_struct Node;
struct hashtable_struct {
	size_t size;
	htable_clean_callback fclean;
	Node **vect;
};

typedef struct hashtable_struct HTable;
//typedef Node ** HTable;
struct iterator_struct {
	int idx;
	Node * cur;
};
typedef struct iterator_struct Iterator;


int hash(char *str);

void htable_create(int size, HTable *ptab, htable_clean_callback clean);

void htable_set(HTable *T,char *key,void* value);

int htable_get(HTable *T,char *key,void **value) ;

void htable_remove(HTable *T,char *key);

int htable_exists(HTable *T,char*key);

void htable_clear(HTable *T);

void htable_iterator_first(HTable *T,Iterator *it);

void htable_iterator_next(HTable *T,Iterator *it);

int htable_iterator_eof(Iterator *it);

char * htable_iterator_pair(Iterator *it,void** value);

////
/**
 * hash(str) 
 *
 * 散列函数，将字符串str进行散列
 */
int hash(char *str) {
	int h = 0,i;
	for(i=0;str[i];i++) {
		h+=(int)str[i]<<((i%sizeof(int))*8);//累加之前需要移位，雪崩效应
	}
	h = h * 31 + 0;
	return h;
}
/**
 * htable_create(size, ptab, clean)
 *
 * 创建哈希表，长度为size，返回到ptab 指向的内存
 * clean 是删除元素内存清理回调
 */
void htable_create(int size, HTable *ptab, htable_clean_callback clean) {
	Node **p = (Node **)malloc(size*sizeof(Node*));
	assert(p);
	memset(p,0,size*sizeof(Node*));
	ptab->size = size;
	ptab->vect = p;
	ptab->fclean = clean;
}
/**
 * htable_set(T,key,value)
 *
 * 向T所指向的哈希表添加键值对
 */
void htable_set(HTable *T,char *key,void* value) {
	assert(T);
	assert(key);

	int pos = (unsigned int)hash(key) % T->size; //将散列值转化为 向量表下标
	//printf("pos = %d\n",pos);
	Node ** p;
	for(p=T->vect + pos;*p;p=&(*p)->next) {//遍历元素指向的链表,判断key是否已经存在
		//printf("p = %p\n",p);
		if(strcmp((*p)->key,key) == 0)
			break;
	}
	if(*p == NULL) { //key不存在，则创建新的节点
		Node *q = malloc(sizeof(Node));
		q->next = *p;
		*p = q;
	}
	(*p)->key = strdup(key);//拷贝key
	assert((*p)->key);
	(*p)->value = value;
}
/**
 * htable_get(T,key,value)
 *
 * 从T指向的哈希表中获取键为key的元素，并返回到指针value指向的内存
 * 如果元素不存在返回 0，否则返回1
 */
int htable_get(HTable *T,char *key,void **value) {
	assert(T);
	assert(key);
	assert(value);
	int pos = (unsigned int)hash(key) % T->size;
	Node ** p;
	for(p=T->vect + pos;*p;p=&((*p)->next)) {
		if(strcmp((*p)->key,key) == 0)
			break;
	}
	if(*p == NULL) return 0;
	if(value) *value = (*p)->value;
	return 1;
}


void htable_remove(HTable *T,char *key) {
	assert(T);
	assert(key);
	int pos = (unsigned int)hash(key) % T->size;
	Node ** p;
	for(p=T->vect + pos;*p;p=&((*p)->next)) {
		if(strcmp((*p)->key,key) == 0) {
			if(T->fclean)
				T->fclean((*p)->value);
			Node *q=(*p)->next;
			free(*p);
			*p = q;
			break;
		}
	}

}

/**
 * htable_exists(T,key)
 *
 * 判断key是否在T指向的哈希表中
 * 存在返回1，否则返回0
 */
int htable_exists(HTable *T,char*key) {
	return htable_get(T,key,NULL);
}


void htable_clear(HTable *T) {
	assert(T);
	int i;
	for(i=0;i<T->size;i++) {
		Node ** p;
		for(p=T->vect + i;*p;p=&((*p)->next)) {
			if(T->fclean)
				T->fclean((*p)->value);
			Node *q=(*p)->next;
			free(*p);
			*p = q;
			break;
		}
	}
}

/**
 * htable_iterator_first(T,it)
 *
 * 根据哈希表指针T，获取第一个元素的迭代器
 */
void htable_iterator_first(HTable *T,Iterator *it) {
	assert(T);
	assert(it);
	
	Node * p;
	int i;
	p = *(T->vect);
	if(p == NULL) {
		for(i=1;i<T->size;i++) {
			if(*(T->vect + i) != NULL) {
				p = *(T->vect + i);
				break;
			}
		}
	}

	if(p == NULL) {
		it->idx = -1;
	} else {
		it->idx = i;
		it->cur = p;
	}
	//printf("htable_iterator_first idx:%d cur:%p\n",i,p);

}
/**
 * htable_iterator_next(T,it) 
 *
 * 根据哈希表T，从it中获取下一个元素的迭代器并返回到it
 */
void htable_iterator_next(HTable *T,Iterator *it) {
	assert(T);
	assert(it);
	Iterator inext;
	inext.idx = it->idx;//初始化返回值idx为当前idx
	if(it->idx < 0) {//如果迭代器有结尾标记，继续传递结尾标记
		inext.idx = -1;
	} else {
		Node * p;
		int i;
		p = it->cur->next;//指向下一个元素
		if(p == NULL) {//如果当前元素防重链表已经到尾部，则依次向后搜索HASH表，直到找到有效链表节点
			for(i=it->idx + 1;i<T->size;i++) {
				if(*(T->vect + i) != NULL) {
					inext.idx = i;//待返回的idx设为当前向量表索引
					p = *(T->vect + i);//有效节点指针
					break;
				}
			}
		}

		if(p == NULL) {//毛都没找到
			inext.idx = -1;
		} else {
			inext.cur = p;
		}
	}
	it->idx = inext.idx;
	it->cur = inext.cur;
	//printf("htable_iterator_next idx:%d cur:%p\n",it->idx,it->cur);
	
}
/**
 * htable_iterator_eofa(it)
 *
 * 判断迭代器指针it是否到达结尾
 */
int htable_iterator_eof(Iterator *it) {
	assert(it);
	if(it->idx<0) return 1;
	return 0;
}
/**
 * htable_iterator_pair(it,value)
 *
 * 从迭代器获取引用的键值对，返回值为键(Key name)
 * 如果it指向结尾，将会断言失败
 */
char * htable_iterator_pair(Iterator *it,void ** value) {
	assert(it);
	assert(it->idx>=0);
	if(value) *value = it->cur->value;
	return it->cur->key;
}

void clean(void *v) {
	printf("clean - %ld\n",(long)v);
}
int main() {
	int size = 10,i;
	HTable T;
	printf("creating hash table,size %d\n",size);
	htable_create(size,&T,clean);
	printf("initializing test cases\n");
	for(i=0;i<30;i++){
		char key[64];
		sprintf(key,"key%d",i);
		htable_set(&T,key,(void*)(long)i);
	}
	printf("total 30 nodes added\n");
	printf("removing 24 nodes\n");
	for(i=0;i<25;i++){
		char key[64];
		sprintf(key,"key%d",i);
		htable_remove(&T,key);
	}
	Iterator it;
	printf("clearing\n");
	htable_clear(&T);
	printf("show\n");
	for(htable_iterator_first(&T,&it); !htable_iterator_eof(&it); htable_iterator_next(&T,&it)) {
		char *key; int v1,v2;
		key = htable_iterator_pair(&it,(void**)&v1);
		htable_get(&T,key,(void**)&v2);//htable_get出现在这里只是为了测试
		printf("%s\t=%d\t%d\n",key,v1,v2);
	}
	return 0;
}
