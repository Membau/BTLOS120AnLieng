/*
 * Part 7 supplement for mm-vm.c
 * Alloc/Free helper logic
 */

#include "mm.h"
#include <stdlib.h>

int get_free_vmrg_area(struct pcb_t *caller,
                       int vmaid,
                       int size,
                       struct vm_rg_struct *newrg)
{
    struct vm_area_struct *cur_vma;
    struct vm_rg_struct *curr;
    struct vm_rg_struct *prev;

    cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

    if (cur_vma == NULL)
        return -1;

    curr = cur_vma->vm_freerg_list;
    prev = NULL;

    while (curr != NULL)
    {
        addr_t rgsize = curr->rg_end - curr->rg_start;

        if (rgsize >= size)
        {
            newrg->rg_start = curr->rg_start;
            newrg->rg_end = curr->rg_start + size;
            newrg->vmaid = vmaid;
            newrg->rg_next = NULL;

            if (rgsize == size)
            {
                if (prev == NULL)
                    cur_vma->vm_freerg_list = curr->rg_next;
                else
                    prev->rg_next = curr->rg_next;

                free(curr);
            }
            else
            {
                curr->rg_start += size;
            }

            return 0;
        }

        prev = curr;
        curr = curr->rg_next;
    }

    return -1;
}
