/*
 * Part 7 + Part 8
 * Alloc/Free + Read/Write
 */

#include "mm.h"
#include <stdlib.h>

int __alloc(struct pcb_t *caller,
            int vmaid,
            int rgid,
            addr_t size,
            addr_t *alloc_addr)
{
    struct vm_rg_struct newrg;
    struct vm_area_struct *cur_vma;

    cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

    if (cur_vma == NULL)
        return -1;

    if (get_free_vmrg_area(caller,
                           vmaid,
                           size,
                           &newrg) < 0)
    {
        if (inc_vma_limit(caller,
                          vmaid,
                          size) < 0)
        {
            return -1;
        }

        if (get_free_vmrg_area(caller,
                               vmaid,
                               size,
                               &newrg) < 0)
        {
            return -1;
        }
    }

    caller->krnl->mm->symrgtbl[rgid] = newrg;

    *alloc_addr = newrg.rg_start;

    return 0;
}

int __free(struct pcb_t *caller,
           int vmaid,
           int rgid)
{
    struct vm_rg_struct *rgnode;
    struct vm_area_struct *cur_vma;

    cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

    if (cur_vma == NULL)
        return -1;

    rgnode = malloc(sizeof(struct vm_rg_struct));

    if (rgnode == NULL)
        return -1;

    *rgnode = caller->krnl->mm->symrgtbl[rgid];

    rgnode->rg_next = NULL;

    enlist_vm_rg_node(&cur_vma->vm_freerg_list,
                      rgnode);

    caller->krnl->mm->symrgtbl[rgid].rg_start = 0;
    caller->krnl->mm->symrgtbl[rgid].rg_end = 0;

    return 0;
}

int __read(struct pcb_t *caller,
           int vmaid,
           int rgid,
           addr_t offset,
           BYTE *data)
{
    struct vm_rg_struct *region;

    addr_t addr;
    addr_t pgn;
    addr_t off;
    addr_t fpn;
    addr_t phyaddr;

    uint32_t pte;

    region = &caller->krnl->mm->symrgtbl[rgid];

    addr = region->rg_start + offset;

    pgn = PAGING_PGN(addr);
    off = PAGING_OFFST(addr);

    pte = pte_get_entry(caller, pgn);

    if (!PAGING_PAGE_PRESENT(pte))
        return -1;

    fpn = PAGING_PTE_FPN(pte);

    phyaddr = (fpn * PAGING_PAGESZ) + off;

    MEMPHY_read(caller->krnl->mram,
                phyaddr,
                data);

    return 0;
}

int __write(struct pcb_t *caller,
            int vmaid,
            int rgid,
            addr_t offset,
            BYTE value)
{
    struct vm_rg_struct *region;

    addr_t addr;
    addr_t pgn;
    addr_t off;
    addr_t fpn;
    addr_t phyaddr;

    uint32_t pte;

    region = &caller->krnl->mm->symrgtbl[rgid];

    addr = region->rg_start + offset;

    pgn = PAGING_PGN(addr);
    off = PAGING_OFFST(addr);

    pte = pte_get_entry(caller, pgn);

    if (!PAGING_PAGE_PRESENT(pte))
        return -1;

    fpn = PAGING_PTE_FPN(pte);

    phyaddr = (fpn * PAGING_PAGESZ) + off;

    MEMPHY_write(caller->krnl->mram,
                 phyaddr,
                 value);

    return 0;
}
