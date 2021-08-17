# include "bitmap.h"
# include "types.h"
# include "string.h"
# include "print.h"
# include "idt.h"
# include "debug.h"

void bitmap_init(struct bitmap* btmap) {
    memset(btmap->bits, 0, btmap->btmp_bytes_len);
}

/**
 * 检测指定位是否为1,如果是,返回1.
 */
int bitmap_scan_test(struct bitmap* btmap, uint32_t index) {
    uint32_t byte_index = (index / 8);
    uint32_t bit_odd = byte_index % 8;

    return (btmap->bits[byte_index] & BITMAP_MASK << bit_odd);
}

/**
 * 在位图中申请连续的cnt个位.
 */
int bitmap_scan(struct bitmap* btmap, uint32_t cnt) {
// #ifndef DEBUG
//     #error "DEBUG undefined"
// #endif
#ifdef DEBUG
    printk("bitmap_scan...\n");
#endif
    uint32_t idx_byte = 0;

    // 以字节为单位进行查找
    while ((0xff == btmap->bits[idx_byte]) && idx_byte < btmap->btmp_bytes_len) {
        ++idx_byte;
    }
#ifdef DEBUG
    printk("idx_byte:\t%d\n", idx_byte);
#endif
    // 没有找到
    if (idx_byte == btmap->btmp_bytes_len) {
#ifdef DEBUG
    printk("idx_byte:\t not find\n");
#endif
        return -1;
    }

    // 找到了一个字节不全为1,那么在字节内部再次进行查找具体的起使位
    int idx_bit = 0;
    while ((uint8_t) BITMAP_MASK << idx_bit & btmap->bits[idx_byte]) {
        ++idx_bit;
    }
#ifdef DEBUG
    printk("idx_bit:\t%d\n", idx_bit);
#endif

    // 起始位
    int bit_idx_start = (idx_byte * 8 + idx_bit);
    if (cnt == 1) {
#ifdef DEBUG
    printk("cnt is 1, will return , and bit_idx_start:\t%d\n", bit_idx_start);
#endif
        return bit_idx_start;
    }

    uint32_t bit_left = (btmap->btmp_bytes_len * 8 - bit_idx_start);
    uint32_t count = 1;

    uint32_t next_bit = bit_idx_start + 1;

    bit_idx_start = -1;
    while (bit_left-- > 0) {
        if (!(bitmap_scan_test(btmap, next_bit))) {
            ++count;
        } else {
            count = 0;
        }

        if (count == cnt) {
            bit_idx_start = (next_bit - cnt + 1);
            break;
        }

        next_bit++;
    }
#ifdef DEBUG
    printk("bitmap_scan end.\n");
#endif

    return bit_idx_start;
}

void bitmap_set(struct bitmap* btmap, uint32_t index, int8_t value) {
#ifdef DEBUG
    printk("bitmap_set...\n");
#endif
    ASSERT(value == 0 || value == 1);

    uint32_t byte_index = index / 8;
    uint32_t bit_odd = index % 8;

    if (value) {
        btmap->bits[byte_index] |= (BITMAP_MASK << bit_odd);
    } else {
        btmap->bits[byte_index] &= ~(BITMAP_MASK << bit_odd);
    }
#ifdef DEBUG
    printk("bitmap_set end.\n");
#endif
}
