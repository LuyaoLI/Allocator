#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

#define PAGESIZE 4096

typedef struct meta{
	int block_size;//integer 1 to 10 to represent 2 to 1024
        void* nextPage; 
}meta;

meta* header[11]={NULL};

typedef struct meta2{
	int block_size;//use 11 to represent big bag(data size>1024B)
	int pageCount;
}meta2;

void init(){
	int i;
	meta* metadata;
	for(i=0;i<10;i++){
		int fd=open("/dev/zero", O_RDWR);
		void* page=mmap(NULL, PAGESIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
		metadata=(meta*)page;
		metadata->block_size=i+1;
		metadata->nextPage=(void*)-1;
		header[i]=metadata;
	}
}
void destroy(){
	int i;
	void* tmp;
	for(i=0;i<10;i++){
		if(header[i]!=NULL){
			while(header[i]->nextPage!=(void*)-1){
				tmp=header[i]->nextPage;
				munmap((void*)header[i], PAGESIZE);
				header[i]=(meta*)tmp;
			}	
			munmap((void*)header[i], PAGESIZE);
		}
	}
}
int hasBlockAvail(meta* metadata){
	int i;
	char* map;
	map=(char*)metadata+sizeof(meta);
	i=0;
	while(i<4088/((1<<metadata->block_size)+1)){
		if(map[i]==0) //has block in page available
			return i;
		else i++;
	}
	return -1;// All occupied
}

void* block_alloc(int size){ //for object size <= 1024
	int block_size,index;
	char* map;
	meta* metadata; 
	block_size=1;
	while((1<<block_size)<size) block_size+=1;
	metadata=header[block_size-1];
	index=hasBlockAvail(metadata);
	while(index==-1){// search next page
		if(metadata->nextPage==(void*)-1) break;
		metadata=(meta*)(metadata->nextPage);
		index=hasBlockAvail(metadata);
	}
	if(index!=-1){ //has block available
		map=(char*)metadata+sizeof(meta);
		map[index]=(char)1;
		return (void*)((char*)metadata+sizeof(meta)+4088/((1<<block_size)+1)+index*(1<<block_size));
	}
	else{ // require new page
		int fd=open("/dev/zero", O_RDWR);
		void *page=mmap(NULL, PAGESIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
		metadata->nextPage=page;
		metadata=(meta*)page;
		metadata->block_size=block_size;
		metadata->nextPage=(void*)-1;
		map=(char*)page+sizeof(meta);
		index=hasBlockAvail(metadata);
		map[index]=(char)1;
		return (void*)((char*)metadata+sizeof(meta)+4088/((1<<block_size)+1)+index*(1<<block_size));
	}
}

void* page_alloc(int size){ //for object size > 1024
	int pageCount;
	meta2* metadata;
	pageCount=(size+sizeof(meta2))/4096;
	if((size+sizeof(meta2))%4096) pageCount+=1;
	int fd=open("/dev/zero", O_RDWR);
	void *page=mmap(NULL, pageCount*PAGESIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	metadata=(meta2*)page;
	metadata->block_size=11;
	metadata->pageCount=pageCount;
	return (void*)((char*)metadata+sizeof(meta2));
}

void* malloc(size_t size){
	void* addr;
	init();
	if(size<=1024) addr=block_alloc((int)size);
	else addr=page_alloc((int)size);
	return addr;
}

void free(void* addr){
	if(!addr)return;
	else{
	void* initaddr;
	int block_size;
	initaddr=(void*)((size_t)addr/4096*4096);
	block_size=*((int*)initaddr);
	if(block_size==11) { //free memory of big bag
		int pageCount;
		pageCount=*((int*)initaddr+1);
		munmap(initaddr, pageCount*PAGESIZE);
		return;
	}
	else{ //free memory of small bag
	void* initdata;
	char* map;
	meta* metadata;
	int index,i;
	initdata=(void*)((char*)initaddr+sizeof(meta)+4088/((1<<block_size)+1));
	map=(char*)initaddr+sizeof(meta);
	index=((size_t)addr-(size_t)initdata)/(1<<block_size);
	map[index]=0;
	for(i=0;i<4088/((1<<block_size)+1);i++){
		if(map[i]==(char)1) return;
	}
	metadata=header[block_size-1];
	if(metadata->nextPage==(void*)-1) return;
	else{
		while(metadata->nextPage!=initaddr)
			metadata=(meta*)(metadata->nextPage);
		metadata->nextPage=((meta*)initaddr)->nextPage;
		munmap(initaddr, PAGESIZE);
		return;
	}
	}
	}
}

void* realloc(void* memaddr,size_t newsize){
if(!memaddr) malloc(newsize);
else if(memaddr&&(!newsize)) free(memaddr);
else{
	int block_size;
	void* initaddr;
	void* newaddr;
	void* initdata;
	initaddr=(void*)((size_t)memaddr/4096*4096);
 	block_size=*((int*)initaddr);        
	if(block_size==11){
		int pageCount;
		pageCount=*((int*)initaddr+1);
		if(pageCount*4096-sizeof(meta2)>=newsize) return memaddr;
		else {
		newaddr=page_alloc(newsize);
		initdata=(void*)((char*)initaddr+sizeof(meta2));
                memcpy(newaddr,memaddr,(int)(((size_t)memaddr-(size_t)initdata))/8);
		free(memaddr);
		return newaddr;
		}
	}
	else{
	char* map;
	int index,i,j,space;
	map=(char*)initaddr+sizeof(meta);
	initdata=(void*)((char*)initaddr+sizeof(meta)+4088/((1<<block_size)+1));
	index=((size_t)memaddr-(size_t)initdata)/(1<<block_size);
	space=0;
	for(i=index;i<4088/((1<<block_size)+1)&&map[i]==0;i++){
		space+=(1<<block_size);
		if(space>=(int)newsize){
			for(j=i;j>=index;j--){
				map[j]=(char)1;
			}
		        return memaddr; 
		}
	}
	newaddr=block_alloc(newsize);	
	memcpy(newaddr,memaddr,(int)(((size_t)memaddr-(size_t)initdata))/8);
	free(memaddr);
	return newaddr;
	}
}
}

void* calloc(size_t numElements,size_t sizeOfElement){
	void* addr;
	addr=malloc(numElements*sizeOfElement);
	memset(addr, 0, numElements*sizeOfElement);
	return addr;
}

