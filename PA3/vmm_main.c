#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include <assert.h>

static struct
{
	unsigned int num_page_fault;
	unsigned int num_page_try;
	unsigned int num_tlb_miss;
	unsigned int num_tlb_try;
} perf_stat = {
	.num_page_fault = 0,
	.num_page_try = 0,
	.num_tlb_miss = 0,
	.num_tlb_try = 0
};

struct tlb_entry
{
	uint8_t page_num;
	uint8_t frame_num;
	int is_valid; // 1 is valid or not.
};

struct page_entry
{
	uint16_t frame_num;
	int is_valid; // 1 is valid or not.
};

uint8_t lookup_tlb(uint8_t page_num);
uint8_t lookup_page_table(uint8_t page_num);
uint8_t lookup_phy_mem(uint32_t phy_addr);

void page_fault_handler(uint8_t page_num);
uint32_t to_phy_addr(uint32_t virt_addr);

struct tlb_entry tlb[16];
int tlb_fifo_idx = 0;

struct page_entry page_table[256];
uint8_t phy_mem[256][256] = {0,};
//uint8_t sub[256*256] = {0,};

static int FRAME=0;

int main(int argc, char** argv)
{
	// Clean tlb and page table.
	//tlb : Translation Lookaside Buffer -> page table의 cache
	for (int it = 0; it < 16; ++it) {
		tlb[it].is_valid = 0;
	}

	for (int it = 0; it < 256; ++it) {
		page_table[it].is_valid = 0;
	}
	uint32_t virt_addr;
	while (scanf("%u", &virt_addr) != EOF) {
		uint32_t phy_addr = to_phy_addr(virt_addr);

		fprintf(stderr, "%d\n", lookup_phy_mem(phy_addr));
	}

	printf("pf: %lf\ntlb: %lf\n",
			(double)perf_stat.num_page_fault / perf_stat.num_page_try,
			(double)perf_stat.num_tlb_miss / perf_stat.num_tlb_try);

	return 0;
}


uint8_t lookup_tlb(uint8_t page_num)
{
	perf_stat.num_tlb_try++;

	for (struct tlb_entry* it = tlb;
			it < tlb + 16;
			it++) {
		if (it->is_valid && it->page_num == page_num) {
	//		printf("TLB_hit\n");
			return it->frame_num;
			
		}
	}

	//printf("TLB_miss\n");
	perf_stat.num_tlb_miss++;
	uint8_t frame_num = lookup_page_table(page_num);

	struct tlb_entry* it = tlb + tlb_fifo_idx;
	tlb_fifo_idx = ++tlb_fifo_idx % 16;

	it->page_num = page_num;
	it->frame_num = frame_num;
	it->is_valid = 1;

	return it->frame_num;
}

uint8_t lookup_page_table(uint8_t page_num)
{
	//printf("lookup_page_table\n");
	if (!page_table[page_num].is_valid) {
		page_fault_handler(page_num);
		perf_stat.num_page_fault++;
	}

	/* 필히 수정*/
	assert(page_table[page_num].is_valid); // page_table[page_num].is_valid가 false면 실행 중단

	perf_stat.num_page_try++;

	return page_table[page_num].frame_num;
}

// page_fault가 일어날 경우, BACKINGSTORE.bin 파일에서 요청된 page number 번째의 페이지 256B만큼을 읽어 해당 번째의 physical memory의 frame에 로딩
// Page table에 frame number 기록
void page_fault_handler(uint8_t page_num)
{

	//printf("page_fault_handler\n");
	FILE* fp = fopen("./input/BACKINGSTORE.bin", "r");
	//uint8_t *phy_mem_p;
	
	//phy_mem_p = phy_mem;

	// TODO: Fill this!
	//printf("page_num:%d\n",page_num);
	fseek(fp, page_num*256, SEEK_SET);
	

	fread(phy_mem[FRAME], 1, 256, fp);
	//for(int i=0; i<256; i++){
		//printf("phy_mem[%d]:%d\n",i,phy_mem[i][1]);

		//printf("phy_mem[%d]:%d\n",i,phy_mem[i][255]);
	//}

	//memcpy(&phy_mem[(FRAME-1)*256],buffer,256);
	//fread(phy_mem[(FRAME-1)*256], 256,1, fp);
	//fread(sub, 256, 1, fp);

	//fseek(fp, page_num, SEEK_SET);
	//fread(phy_mem,sizeof(256),1,fp);
	//printf("page_num:%d\n",page_num);
	//printf("value:%d\n",page_num*256);
	//for(int i=0; i< 500; i++){
		//printf("phy_mem[%d]:%d\n",i,phy_mem[i]);
	//}

	page_table[page_num].is_valid=1;
	page_table[page_num].frame_num = FRAME;

	
	FRAME++;
	fclose(fp);
}

// Virtual address가 들어왔을 때, 위 흐름도를 통해 physical memory에서의 address로 번역
uint32_t to_phy_addr(uint32_t virt_addr)
{
	uint8_t page_number;
	uint8_t frame_number;
	uint32_t phy_addr;
	uint8_t offset;

	//printf("virt_addr:%d\n",virt_addr);
    
	page_number=(virt_addr>>8) & 0xff;
	//printf("page_number:%d\n",page_number);
	offset=virt_addr & 0xff;
	
	frame_number=lookup_tlb(page_number);

	phy_addr = (frame_number<<8)+ offset;
	//printf("page_number:%u\n",page_number);
	//printf("offset:%u\n",offset);
	/*
	long  binary, sum=0, i=1;
    long number = virt_addr;
	while(number>0)
    {
        binary=number%2;
        sum+=binary*i;
        number=number/2;
        i*=10;
    }
    printf("2진수 : %ld\n", sum);
	*/

	return phy_addr;
	//return 0xdeadbeaf; // TODO: Make it work!
}



// physical address 위치의 physical memory 값을 읽어서 리턴
uint8_t lookup_phy_mem(uint32_t phy_addr)
{
	uint8_t frame_number2 = (phy_addr>>8) & 0xff;
	uint8_t offset2 = phy_addr & 0xff;
	//printf("CHECK!!\n");
	//for(int i=0; i<256; i++){
//		printf("phy_mem[%d]:%hhn\n",i,phy_mem[i]);
//	}
	//printf("frame_number2:%d\n", frame_number2);
	//printf("offset2:%d\n", offset2);
	return phy_mem[frame_number2][offset2]; 
	// TODO: Make it work!
}
