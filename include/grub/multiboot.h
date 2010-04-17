/* multiboot.h - multiboot header file with grub definitions. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2007,2008,2010  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_MULTIBOOT_HEADER
#define GRUB_MULTIBOOT_HEADER 1

#include <grub/file.h>

#ifdef GRUB_USE_MULTIBOOT2
#include <multiboot2.h>
/* Same thing as far as our loader is concerned.  */
#define MULTIBOOT_BOOTLOADER_MAGIC	MULTIBOOT2_BOOTLOADER_MAGIC
#define MULTIBOOT_HEADER_MAGIC		MULTIBOOT2_HEADER_MAGIC
#else
#include <multiboot.h>
#endif

#include <grub/types.h>
#include <grub/err.h>

void grub_multiboot (int argc, char *argv[]);
void grub_module (int argc, char *argv[]);

grub_size_t grub_multiboot_get_mbi_size (void);
grub_err_t grub_multiboot_make_mbi (void *orig, grub_uint32_t dest,
				    grub_off_t buf_off, grub_size_t bufsize);
void grub_multiboot_free_mbi (void);
grub_err_t grub_multiboot_init_mbi (int argc, char *argv[]);
grub_err_t grub_multiboot_add_module (grub_addr_t start, grub_size_t size,
				      int argc, char *argv[]);
void grub_multiboot_set_bootdev (void);

grub_uint32_t grub_get_multiboot_mmap_count (void);
grub_err_t grub_multiboot_set_video_mode (void);

#if defined (GRUB_MACHINE_PCBIOS) || defined (GRUB_MACHINE_COREBOOT) || defined (GRUB_MACHINE_QEMU)
#include <grub/i386/pc/vbe.h>
#define GRUB_MACHINE_HAS_VGA_TEXT 1
#else
#define GRUB_MACHINE_HAS_VGA_TEXT 0
#endif

#define GRUB_MULTIBOOT_CONSOLE_EGA_TEXT 1
#define GRUB_MULTIBOOT_CONSOLE_FRAMEBUFFER 2 

grub_err_t
grub_multiboot_set_console (int console_type, int accepted_consoles,
			    int width, int height, int depth,
			    int console_required);
grub_err_t
grub_multiboot_load (grub_file_t file);
/* Load ELF32 or ELF64.  */
grub_err_t
grub_multiboot_load_elf (grub_file_t file, void *buffer);
extern grub_size_t grub_multiboot_pure_size;
extern grub_size_t grub_multiboot_alloc_mbi;

#endif /* ! GRUB_MULTIBOOT_HEADER */
