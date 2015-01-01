#include <error.h>
#include <proc/elf.h>
#include <disk/ext2.h>
#include <mm/memcpy.h>
#include <mm/paging.h>

namespace Elf
{

/*
 * Teste si le fichier dont l'adresse est passee en argument
 * est au format ELF
 */
bool isElf(u8* file)
{
	Elf32_Ehdr *hdr;

	hdr = (Elf32_Ehdr *) file;
	if (hdr->e_ident[0] == 0x7f && hdr->e_ident[1] == 'E'
	    && hdr->e_ident[2] == 'L' && hdr->e_ident[3] == 'F')
        return true;
	else
        return false;
}

u64 loadElf(u8* file, Process *proc)
{
    u8 *p;
    u64 v_begin, v_end;
	Elf32_Ehdr *hdr;
	Elf32_Phdr *p_entry;
    u64 i, pe;

	hdr = (Elf32_Ehdr *) file;
	p_entry = (Elf32_Phdr *) (file + hdr->e_phoff);	/* Program header table offset */

    if (!isElf(file)) {
        error("loadElf() : file not in ELF format !\n");
		return 0;
	}

    // Read each entry
    for (pe = 0; pe < hdr->e_phnum; pe++, p_entry++)
    {
        if (p_entry->p_type == PT_LOAD)
        {
			v_begin = p_entry->p_vaddr;
			v_end = p_entry->p_vaddr + p_entry->p_memsz;

			if (v_begin < USER_OFFSET) {
                error("loadElf: can't load executable below %p\n", USER_OFFSET);
				return 0;
			}

			if (v_end > USER_STACK) {
                error("loadElf: can't load executable above %p\n", USER_STACK);
				return 0;
			}
			
			/* Description de la zone exec + rodata */
			if (p_entry->p_flags == PF_X + PF_R) {	
                proc->bExec = (u8*) v_begin;
                proc->eExec = (u8*) v_end;
			}

			/* Description de la zone bss */
			if (p_entry->p_flags == PF_W + PF_R) {	
                proc->bBss = (u8*) v_begin;
                proc->eBss = (u8*) v_end;
			}
			

            // Map pages for the file data. We could let the page fault handler do it, but let's stay clean
            for (u64 ptr=v_begin&~0x7FFUL; ptr<v_begin+p_entry->p_filesz; ptr+=PAGESIZE)
                if (!Paging::getPAddr((u8*)ptr))
                    Paging::mapPage((u8*)ptr,nullptr,PAGE_ATTR_USER,proc->pml4);

            memcpy((u8*) v_begin, (u8*) (file + p_entry->p_offset), p_entry->p_filesz);
			if (p_entry->p_memsz > p_entry->p_filesz)
                for (i = p_entry->p_filesz, p = (u8*)(u64) p_entry->p_vaddr; i < p_entry->p_memsz; i++)
					p[i] = 0;
		}
	}

	/* Return program entry point */
	return hdr->e_entry;
}

} // Elf
