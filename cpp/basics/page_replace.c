#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TRUE         1
#define FALSE        0
#define INVALID     -1
#define TOTAL_INSTR  320
#define TOTAL_VP     32

typedef struct {
    int pn;       /* 页号 */
    int pfn;      /* 页框号 */
    int counter;  /* 计数器（预留扩展） */
    int time;     /* 最近访问时刻（LRU 用） */
} pl_type;
pl_type pl[TOTAL_VP];

/* 页框控制块：双向队列节点，同时用于忙队列和空闲队列 */
struct pfc_struct {
    int pn;                 /* 当前装入的页号 */
    int pfn;                /* 页框号 */
    struct pfc_struct *next;
};
typedef struct pfc_struct pfc_type;
pfc_type pfc[TOTAL_VP];                  /* 静态页框数组 */
pfc_type *freepf_head;                   /* 空闲队列头 */
pfc_type *busypf_head, *busypf_tail;     /* 忙队列头尾 */

int a[TOTAL_INSTR];       /* 指令地址流 */
int page[TOTAL_INSTR];    /* 页地址流 */
int offset[TOTAL_INSTR];  /* 页内偏移流 */
int diseffect;            /* 缺页次数 */

/* ---------- 初始化 ---------- */
void initialize(int total_pf)
{
    int i;

    /* 页表初始化 */
    for (i = 0; i < TOTAL_VP; i++) {
        pl[i].pn      = i;
        pl[i].pfn     = INVALID;
        pl[i].counter = 0;
        pl[i].time    = -1;
    }

    /* 页框链表初始化：全部链入空闲队列 */
    for (i = 0; i < total_pf; i++) {
        pfc[i].pfn  = i;
        pfc[i].pn   = INVALID;
        pfc[i].next = &pfc[i + 1];
    }
    pfc[total_pf - 1].next = NULL;

    freepf_head           = &pfc[0];
    busypf_head = busypf_tail = NULL;
    diseffect              = 0;
}

/* ---------- 生成指令序列 ---------- */
void generate_sequence(void)
{
    int i, M, Mp;

    srand((unsigned)time(NULL));

    for (i = 0; i < TOTAL_INSTR; ) {
        /* 取一条顺序指令 */
        M = rand() % 320;
        a[i++] = (M + 1) % 320;
        if (i >= TOTAL_INSTR) break;

        /* 前部随机跳转 [0, M+1] */
        Mp = rand() % (M + 2);
        a[i++] = Mp;
        if (i >= TOTAL_INSTR) break;

        /* 顺序执行 Mp+1 */
        a[i++] = (Mp + 1) % 320;
        if (i >= TOTAL_INSTR) break;

        /* 后部随机跳转 [Mp+2, 319] */
        if (Mp + 2 <= 319)
            a[i++] = Mp + 2 + rand() % (318 - Mp);
        else
            a[i++] = rand() % 320;
    }

    /* 转换为页地址流（每页 10 条指令） */
    for (i = 0; i < TOTAL_INSTR; i++) {
        page[i]   = a[i] / 10;
        offset[i] = a[i] % 10;
    }
}

/* ---------- 从忙队列中摘除指定页号的节点 ---------- */
pfc_type *remove_from_busy(int victim)
{
    pfc_type *p, *prev = NULL;

    p = busypf_head;
    while (p != NULL && p->pn != victim) {
        prev = p;
        p = p->next;
    }

    if (p == NULL) return NULL;               /* 未找到 */

    if (prev == NULL) {                       /* 是队头 */
        busypf_head = p->next;
    } else {
        prev->next = p->next;
    }

    if (busypf_tail == p)                     /* 是队尾 */
        busypf_tail = prev;

    /* busypf_head 和 tail 一致性：head 为 NULL ⇒ tail 也应为 NULL */
    if (busypf_head == NULL)
        busypf_tail = NULL;

    return p;
}

/* ---------- 分配一个空闲帧并装入指定页面 ---------- */
void allocate_frame(int pg)
{
    pfc_type *newp;

    newp = freepf_head;
    freepf_head = freepf_head->next;
    newp->next  = NULL;
    newp->pn    = pg;
    pl[pg].pfn  = newp->pfn;

    /* 挂入忙队列尾部 */
    if (busypf_tail == NULL) {
        busypf_head = busypf_tail = newp;
    } else {
        busypf_tail->next = newp;
        busypf_tail       = newp;
    }
}

/* ========================================================
 *   OPT — 最佳置换算法
 *   淘汰将来最远才会再次使用的页面
 * ======================================================== */
void OPT(int total_pf)
{
    initialize(total_pf);

    int i, j, k, max_dist, victim = 0;
    int dist[TOTAL_VP];

    for (i = 0; i < TOTAL_INSTR; i++) {
        if (pl[page[i]].pfn == INVALID) {       /* 缺页 */
            diseffect++;

            if (freepf_head == NULL) {          /* 无空闲帧，需淘汰 */
                /* 计算每个在内存中的页面距下次使用的距离 */
                for (j = 0; j < TOTAL_VP; j++) {
                    if (pl[j].pfn != INVALID) {
                        dist[j] = TOTAL_INSTR + 1;  /* 大哨兵：未来不再使用 */
                        for (k = i + 1; k < TOTAL_INSTR; k++) {
                            if (page[k] == j) {
                                dist[j] = k - i;
                                break;
                            }
                        }
                    }
                }

                /* 选出距离最大的淘汰 */
                max_dist = -1;
                for (j = 0; j < TOTAL_VP; j++) {
                    if (pl[j].pfn != INVALID && dist[j] > max_dist) {
                        max_dist = dist[j];
                        victim = j;
                    }
                }

                /* 淘汰 victim */
                pfc_type *recycled = remove_from_busy(victim);
                if (recycled != NULL) {
                    recycled->next  = freepf_head;
                    freepf_head     = recycled;
                }
                pl[victim].pfn = INVALID;
            }

            allocate_frame(page[i]);
        }
    }

    printf("OPT:%6.4f  ", 1.0 - (double)diseffect / TOTAL_INSTR);
}

/* ========================================================
 *   FIFO — 先进先出置换算法
 * ======================================================== */
void FIFO(int total_pf)
{
    initialize(total_pf);

    int i;
    pfc_type *p;

    for (i = 0; i < TOTAL_INSTR; i++) {
        if (pl[page[i]].pfn == INVALID) {       /* 缺页 */
            diseffect++;

            if (freepf_head == NULL) {          /* 无空闲帧，淘汰队首 */
                p = busypf_head;
                pl[p->pn].pfn = INVALID;

                /* 队首出队 */
                busypf_head = p->next;
                if (busypf_head == NULL)        /* 队列变空 */
                    busypf_tail = NULL;

                /* 回收至空闲队列 */
                p->next      = freepf_head;
                freepf_head  = p;
            }

            allocate_frame(page[i]);
        }
    }

    printf("FIFO:%6.4f  ", 1.0 - (double)diseffect / TOTAL_INSTR);
}

/* ========================================================
 *   LRU — 最近最久未使用置换算法
 * ======================================================== */
void LRU(int total_pf)
{
    initialize(total_pf);

    int i, j, min_time, victim = 0;

    for (i = 0; i < TOTAL_INSTR; i++) {
        if (pl[page[i]].pfn == INVALID) {       /* 缺页 */
            diseffect++;

            if (freepf_head == NULL) {          /* 淘汰最近最久未使用的页面 */
                min_time = TOTAL_INSTR + 1;
                for (j = 0; j < TOTAL_VP; j++) {
                    if (pl[j].pfn != INVALID && pl[j].time < min_time) {
                        min_time = pl[j].time;
                        victim = j;
                    }
                }

                pfc_type *recycled = remove_from_busy(victim);
                if (recycled != NULL) {
                    recycled->next  = freepf_head;
                    freepf_head     = recycled;
                }
                pl[victim].pfn = INVALID;
            }

            allocate_frame(page[i]);
        }

        /* 更新访问时间 */
        pl[page[i]].time = i;
    }

    printf("LRU:%6.4f  ", 1.0 - (double)diseffect / TOTAL_INSTR);
}

/* ========================================================
 *   main
 * ======================================================== */
int main(void)
{
    generate_sequence();
    printf("页地址流生成完毕。\n\n");
    printf("内存页数\tOPT\t\tFIFO\t\tLRU\n");
    printf("--------\t---\t\t----\t\t---\n");

    int pf;
    for (pf = 4; pf <= TOTAL_VP; pf++) {
        printf("%2d\t\t", pf);
        OPT(pf);
        FIFO(pf);
        LRU(pf);
        printf("\n");
    }

    return 0;
}
