/*
 * Part 9 - MM64 Paging
 */

#include "mm64.h"
#include <stdlib.h>
#include <stdio.h>

int get_pd_from_address(addr_t addr,
                        addr_t* pgd,
                        addr_t* p4d,
                        addr_t* pud,
                        addr_t* pmd,
                        addr_t* pt)
{
    *pgd = PAGING64_ADDR_PGD(addr);
    *p4d = PAGING64_ADDR_P4D(addr);
    *pud = PAGING64_ADDR_PUD(addr);
    *pmd = PAGING64_ADDR_PMD(addr);
    *pt  = PAGING64_ADDR_PT(addr);

    return 0;
}

int get_pd_from_pagenum(addr_t pgn,
                        addr_t* pgd,
                        addr_t* p4d,
                        addr_t* pud,
                        addr_t* pmd,
                        addr_t* pt)
{
    return get_pd_from_address(
        pgn << PAGING64_ADDR_PT_SHIFT,
        pgd,
        p4d,
        pud,
        pmd,
        pt
    );
}

int pte_set_fpn(struct pcb_t *caller,
                addr_t pgn,
                addr_t fpn)
{
    struct krnl_t *krnl = caller->krnl;

    addr_t *pte;

    pte = &krnl->mm->pt[pgn];

    SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
    CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

    SETVAL(*pte,
           fpn,
           PAGING_PTE_FPN_MASK,
           PAGING_PTE_FPN_LOBIT);

    return 0;
}

uint32_t pte_get_entry(struct pcb_t *caller,
                       addr_t pgn)
{
    struct krnl_t *krnl = caller->krnl;

    return krnl->mm->pt[pgn];
}

addr_t alloc_pages_range(struct pcb_t *caller,
                         int req_pgnum,
                         struct framephy_struct **frm_lst)
{
    int i;

    addr_t fpn;

    struct framephy_struct *head = NULL;
    struct framephy_struct *tail = NULL;

    for (i = 0; i < req_pgnum; i++)
    {
        if (MEMPHY_get_freefp(caller->krnl->mram,
                              &fpn) < 0)
        {
            return -3000;
        }

        struct framephy_struct *newfp =
            malloc(sizeof(struct framephy_struct));

        if (newfp == NULL)
            return -1;

        newfp->fpn = fpn;
        newfp->fp_next = NULL;

        if (head == NULL)
        {
            head = newfp;
            tail = newfp;
        }
        else
        {
            tail->fp_next = newfp;
            tail = newfp;
        }
    }

    *frm_lst = head;

    return 0;
}

addr_t vmap_page_range(struct pcb_t *caller,
                       addr_t addr,
                       int pgnum,
                       struct framephy_struct *frames,
                       struct vm_rg_struct *ret_rg)
{
    int pgit;

    addr_t pgn;

    struct framephy_struct *fpit = frames;

    ret_rg->rg_start = addr;
    ret_rg->rg_end =
        addr + pgnum * PAGING_PAGESZ;

    for (pgit = 0; pgit < pgnum; pgit++)
    {
        if (fpit == NULL)
            return -1;

        pgn = PAGING_PGN(addr) + pgit;

        pte_set_fpn(caller,
                    pgn,
                    fpit->fpn);

        enlist_pgn_node(
            &caller->krnl->mm->fifo_pgn,
            pgn
        );

        fpit = fpit->fp_next;
    }

    return 0;
}

int init_mm(struct mm_struct *mm,
            struct pcb_t *caller)
{
    struct vm_area_struct *vma0;

    vma0 = malloc(sizeof(struct vm_area_struct));

    if (vma0 == NULL)
        return -1;

    mm->pgd = calloc(PAGING64_MAX_PGN,
                     sizeof(addr_t));

    mm->p4d = calloc(PAGING64_MAX_PGN,
                     sizeof(addr_t));

    mm->pud = calloc(PAGING64_MAX_PGN,
                     sizeof(addr_t));

    mm->pmd = calloc(PAGING64_MAX_PGN,
                     sizeof(addr_t));

    mm->pt = calloc(PAGING64_MAX_PGN,
                    sizeof(addr_t));

    if (mm->pgd == NULL ||
        mm->p4d == NULL ||
        mm->pud == NULL ||
        mm->pmd == NULL ||
        mm->pt == NULL)
    {
        return -1;
    }

    vma0->vm_id = 0;
    vma0->vm_start = 0;
    vma0->vm_end = 0;
    vma0->sbrk = 0;

    vma0->vm_next = NULL;

    vma0->vm_mm = mm;

    vma0->vm_freerg_list =
        init_vm_rg(0, 0);

    mm->mmap = vma0;

    mm->fifo_pgn = NULL;

    return 0;
}
