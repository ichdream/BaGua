# include "memory.h"
# include "types.h"
# include "print.h"
# include "string.h"
# include "debug.h"

# define PAGE_SIZE 4096

// 位图地址
# define MEM_BITMAP_BASE 0xc009a000

// 内核使用的起始虚拟地址
// 跳过低端1MB内存，中间10为代表页表项偏移，即0x100，即256 * 4KB = 1MB
# define K_HEAD_START 0xc0100000

// 获取高10位页目录项标记
# define PDE_INDEX(addr) ((addr & 0xffc00000) >> 22)
// 获取中间10位页表标记 
# define PTE_INDEX(addr) ((addr & 0x003ff000) >> 12) 

// static functions declarations
static void printKernelPoolInfo(struct pool p);
static void printUserPoolInfo(struct pool p);
static void* vaddr_get(enum pool_flags pf, uint32_t pg_count);
static uint32_t* pte_ptr(uint32_t vaddr);
static uint32_t* pde_ptr(uint32_t vaddr);
static void* palloc(struct pool* m_pool);
static void page_table_add(void* _vaddr, void* _page_phyaddr);

struct pool {
    struct bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
};

struct pool kernel_pool, user_pool;
struct virtual_addr kernel_addr;

/**
 * 初始化内存池.
 */ 
static void mem_pool_init(uint32_t all_memory) {
    put_str("Start init Memory pool...\n");

    // 页表(一级和二级)占用的内存大小，256的由来:
    // 一页的页目录，页目录的第0和第768项指向一个页表，此页表分配了低端1MB内存(其实此页表中也只是使用了256个表项)，
    // 剩余的254个页目录项实际没有分配对应的真实页表，但是需要为内核预留分配的空间
    uint32_t page_table_size = PAGE_SIZE * 256;

    // 已经使用的内存为: 低端1MB内存 + 现有的页表和页目录占据的空间
    uint32_t used_mem = (page_table_size + 0x100000);
#ifdef DEBUG   
    printk("all_memory: %d MB\n", all_memory / (1024 * 1024));
    printk("used_mem: %d KB\n", used_mem / 1024);
#endif
    uint32_t free_mem = (all_memory - used_mem);
    uint32_t free_pages = free_mem / PAGE_SIZE;

    // 剩余内存分配方案：内核和用户空间各分一半
    uint32_t kernel_free_pages = (free_pages >> 1);
    uint32_t user_free_pages = (free_pages - kernel_free_pages);
#ifdef DEBUG    
    printk("kernel_free_pages: %d\n", kernel_free_pages);
    printk("user_free_pages: %d\n", user_free_pages);
#endif
    // 内核空间bitmap长度(字节)，每一位代表一页
    uint32_t kernel_bitmap_length = kernel_free_pages / 8;
    uint32_t user_bitmap_length = user_free_pages / 8;

    // 内核内存池起始物理地址，注意内核的虚拟地址占据地址空间的顶端，但是实际映射的物理地址是在这里
    uint32_t kernel_pool_start = used_mem;
    uint32_t user_pool_start = (kernel_pool_start + kernel_free_pages * PAGE_SIZE);

    kernel_pool.phy_addr_start = kernel_pool_start;
    user_pool.phy_addr_start = user_pool_start;

    kernel_pool.pool_size = kernel_free_pages * PAGE_SIZE;
    user_pool.pool_size = user_free_pages * PAGE_SIZE;

    kernel_pool.pool_bitmap.btmp_bytes_len = kernel_bitmap_length;
    user_pool.pool_bitmap.btmp_bytes_len = user_bitmap_length;

    // 内核bitmap和user bitmap bit数组的起始地址
    kernel_pool.pool_bitmap.bits = (void*) MEM_BITMAP_BASE;
    user_pool.pool_bitmap.bits = (void*) (MEM_BITMAP_BASE + kernel_bitmap_length);

    printKernelPoolInfo(kernel_pool);
    printUserPoolInfo(user_pool);

    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    kernel_addr.vaddr_bitmap.btmp_bytes_len = kernel_bitmap_length;
    // 内核虚拟地址池仍然保存在低端内存以内
    kernel_addr.vaddr_bitmap.bits = (void*) (MEM_BITMAP_BASE + kernel_bitmap_length + user_bitmap_length);
    kernel_addr.vaddr_start = K_HEAD_START;

    bitmap_init(&kernel_addr.vaddr_bitmap);
    put_str("Init memory pool done.\n");
}

static void printKernelPoolInfo(struct pool p) {
    put_str("Kernel pool bitmap address: ");
    put_int(p.pool_bitmap.bits);
    put_str("; Kernel pool physical address: ");
    put_int(p.phy_addr_start);
    put_char('\n');
}

static void printUserPoolInfo(struct pool p) {
    put_str("User pool bitmap address: ");
    put_int(p.pool_bitmap.bits);
    put_str("; User pool physical address: ");
    put_int(p.phy_addr_start);
    put_char('\n');
}

/**
 * 申请指定个数的虚拟页.返回虚拟页的起始地址，失败返回NULL.
 */ 
static void* vaddr_get(enum pool_flags pf, uint32_t pg_count) {
    printk("vaddr_get...\n");
    int vaddr_start = 0, bit_idx_start = -1;
    uint32_t count = 0;

    if (pf == PF_KERNEL) {
        bit_idx_start = bitmap_scan(&kernel_addr.vaddr_bitmap, pg_count);
        if (bit_idx_start == -1) {
            // 申请失败，虚拟内存不足
            return NULL;
        }

        // 修改bitmap，占用虚拟内存
        while (count < pg_count) {
            bitmap_set(&kernel_addr.vaddr_bitmap, (bit_idx_start + count), 1);
            ++count;
        }

        vaddr_start = (kernel_addr.vaddr_start + bit_idx_start * PAGE_SIZE); 
    } else {
        // 用户内存分配暂不支持
    }
    printk("vaddr_start:\t0x%08X\n", vaddr_start);
    printk("vaddr_get end.\n");

    return (void*) vaddr_start;
}

/**
 * 得到虚拟地址对应的PTE的指针.
 */ 
static uint32_t* pte_ptr(uint32_t vaddr) {
#ifdef DEBUG
    printk("pte_ptr...\n");
#endif
    return (uint32_t*) (0xffc00000 + ((vaddr & 0xffc00000) >> 10) + (PTE_INDEX(vaddr) << 2));
}

/**
 * 得到虚拟地址对应的PDE指针.
 */ 
static uint32_t* pde_ptr(uint32_t vaddr) {
    // 0xfffff000用来访问页表本身所在的地址， 说明是最后一个虚拟页面
    // 最后一个页目录项对应的页表项虚拟地址就是：0xfffff000
#ifdef DEBUG
    printk("pde_ptr...\n");
#endif
    return (uint32_t*) ((0xfffff000) + (PDE_INDEX(vaddr) << 2));
}

/**
 * 页目录表实际的物理地址在CR3寄存器中。。。CPU通过查虚拟地址的对应的页表来找到物理地址
    获取高10位页目录项标记
    #define PDE_INDEX(addr) ((addr & 0xffc00000) >> 22)
    获取中间10位页表标记 
    #define PTE_INDEX(addr) ((addr & 0x003ff000) >> 12)  
 */

/**
 * 在给定的物理内存池中分配一个物理页，返回其物理地址.
 */ 
static void* palloc(struct pool* m_pool) {
#ifdef DEBUG
    printk("palloc...\n");
#endif
    int bit_index = bitmap_scan(&m_pool->pool_bitmap, 1);
    if (bit_index == -1) {
        return NULL;
    }
#ifdef DEBUG
    printk("will run bitmap_set, set bit_index: %d\n", bit_index);
#endif
    bitmap_set(&m_pool->pool_bitmap, bit_index, 1);
    uint32_t page_phyaddr = ((bit_index * PAGE_SIZE) + m_pool->phy_addr_start);
#ifdef DEBUG
    printk("page_phyaddr:\t0x%08X\n", page_phyaddr);
#endif
#ifdef DEBUG
    printk("palloc end\n");
#endif
    return (void*) page_phyaddr;
}

/**
 * 通过页表建立虚拟页与物理页的映射关系.
 */ 
static void page_table_add(void* _vaddr, void* _page_phyaddr) {
#ifdef DEBUG
    printk("page_table_add...\n");
#endif
    uint32_t vaddr = (uint32_t) _vaddr, page_phyaddr = (uint32_t) _page_phyaddr;
    uint32_t* pde = pde_ptr(vaddr); uint32_t* pte = pte_ptr(vaddr);
#ifdef DEBUG
    printk("pde:\t0x%08X\n", pde);
    printk("pte:\t0x%08X\n", pte);
#endif

    if (*pde & 0x00000001) {
        // 页目录项已经存在
#ifdef DEBUG
    printk("目录项存在，继续判断物理页是否存在。。。\n");
#endif
        if (!(*pte & 0x00000001)) {
#ifdef DEBUG
    printk("目录项存在，但是物理页必定不存在，页表项指向我们新分配的物理页\n");
#endif
            // 物理页必定不存在，使页表项指向我们新分配的物理页
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
    } else {
        // 新分配一个物理页作为页表
#ifdef DEBUG
    printk("目录项不存在，即页表不存在，新分配一个物理页作为页表\n");
#endif
        uint32_t pde_phyaddr = (uint32_t) palloc(&kernel_pool);
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        // 清理物理页
        memset((void*) ((int) pte & 0xfffff000), 0, PAGE_SIZE);
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
}

/**
 * 分配page_count个页空间，自动建立虚拟页与物理页的映射.
 */ 
void* malloc_page(enum pool_flags pf, uint32_t page_count) {
#ifdef DEBUG
    printk("malloc_page...\n");
#endif
    ASSERT(page_count > 0 && page_count < 3840);

    // 在虚拟地址池中申请虚拟内存
    // printk("malloc_page:在虚拟地址池中申请虚拟内存\n");
    void* vaddr_start = vaddr_get(pf, page_count);
    if (vaddr_start == NULL) {
        return NULL;
    }

    uint32_t vaddr = (uint32_t) vaddr_start, count = page_count;
    struct pool* mem_pool = (pf & PF_KERNEL) ? &kernel_pool : &user_pool;

    // 物理页不必连续，逐个与虚拟页做映射
    while (count > 0) {
        void* page_phyaddr = palloc(mem_pool);
        if (page_phyaddr == NULL) {
            return NULL;
        }
#ifdef DEBUG
    printk("phy pages alloc succeed..., vaddr:\t0x%08X\n", vaddr);
    printk("phy pages alloc succeed..., page_phyaddr:\t0x%08X\n", page_phyaddr);
#endif
        page_table_add((void*) vaddr, page_phyaddr);
        vaddr += PAGE_SIZE;
        --count;
    }
    printk("malloc_page end.\n");

    return vaddr_start;
}

/**
 * 在内核内存池中申请page_count个页.
 */ 
void* get_kernel_pages(uint32_t page_count) {
#ifdef DEBUG
    printk("get_kernel_pages...\n");
#endif
    void* vaddr = malloc_page(PF_KERNEL, page_count);
    if (vaddr != NULL) {
        memset(vaddr, 0, page_count * PAGE_SIZE);
    }
#ifdef DEBUG
    printk("get_kernel_pages end.\n");
#endif
    return vaddr;
}

void mem_init(void) {
    put_str("Init memory start.\n");
    // uint32_t total_memory = (*(uint32_t*) (0xb00));
    uint32_t total_memory = (uint32_t) 0x20000000;
    mem_pool_init(total_memory);
    put_str("Init memory done.\n");
}