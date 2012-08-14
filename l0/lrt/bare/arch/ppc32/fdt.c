/*
 * Copyright (C) 2012 by Project SESA, Boston University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <config.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <l0/lrt/bare/arch/ppc32/fdt.h>
#include <lrt/string.h>

const uint32_t fdt_validmagic = 0xd00dfeed;

const uint32_t fdt_begin_node = 0x1;
const uint32_t fdt_end_node = 0x2;
const uint32_t fdt_prop = 0x3;
const uint32_t fdt_nop = 0x4;
const uint32_t fdt_end = 0x9;

struct fdt {
  uint32_t magic;
  uint32_t size;
  uint32_t offset_struct;
  uint32_t offset_strings;
  uint32_t offset_mmap;
  uint32_t version;
  uint32_t last_compatible_version;
  uint32_t boot_cpuid;
  uint32_t string_size;
  uint32_t struct_size;
};

struct fdt_node {
  uint32_t token;
};

struct fdt_header {
  uint32_t token;
  char name[0];
};

struct fdt_prop {
  uint32_t token;
  uint32_t len;
  uint32_t name_off;
  char data[0];
};

static struct fdt *fdt;

static bool
fdt_isvalid(struct fdt *fdt)
{
  return fdt->magic == fdt_validmagic; 
}

bool 
fdt_init(struct fdt *oldfdt)
{
  if (!fdt_isvalid(oldfdt)) {
    return false;
  }
  //check if where we copy the fdt is overlapping with the fdt
  extern char *mem_start; //from mem.c
  if (((uintptr_t)mem_start + oldfdt->size) >= (uintptr_t)oldfdt) {
      return false;
  }

  uintptr_t *newfdt = (uintptr_t *)mem_start;
  for (int i = 0; i < (oldfdt->size / sizeof(uintptr_t)); i++) {
    newfdt[i] = ((uintptr_t *)oldfdt)[i];
  }
  fdt=(struct fdt*)newfdt;
  mem_start += fdt->size;
  return true;
}

struct fdt_node *
fdt_get_root_node()
{
  char *ptr = (char *)fdt;
  ptr += fdt->offset_struct;
  return (struct fdt_node *)ptr;
}

struct fdt_node *
fdt_get(char *path)
{
  struct fdt_node *node = fdt_get_root_node();

  //remove leading '/' from the path
  while (*path == '/') {
    path++;
  }

  char *next_path;
  while (1) {
    //find the next '/' otherwise next_path is NULL
    next_path = path;
    while (*next_path != '/') {
      if (*next_path == '\0') {
	next_path = NULL;
	break;
      }
      next_path++;
    }
    if (next_path != NULL) {
      *next_path = '\0'; //temporarily terminate the string (so it is one level)
      node = fdt_get_in_node(node, path); //find that node
      *next_path = '/'; //put back the path
      path = next_path + 1; //advance the path to the next level
      if (!node) {
	return NULL; //if we didn't find the node, we failed
      }
    } else {
      return fdt_get_in_node(node, path); //last level, return the node, success or not
    }
  }
}

struct fdt_node *
fdt_get_in_node(struct fdt_node *node, char *path) {
  int level = 0;
  do {
    if (node->token == fdt_begin_node) {
      struct fdt_header *hdr = (struct fdt_header *)node;
      if ((level == 1) && (strcmp(path, hdr->name) == 0)) {
	return node;
      }
      level++;
      char *name = hdr->name;
      while (*name != '\0') {
	name++;
      }
      node = (struct fdt_node *)(((uintptr_t)(name + 4)) & ~0x3);
    } else if (node->token == fdt_prop) {
      struct fdt_prop *prop = (struct fdt_prop *)node;
      char *name = ((char *)fdt) + fdt->offset_strings + prop->name_off;
      if ((level == 1) && (strcmp(path, name) == 0)) {
	return node;
      }
      //4 byte alignment after property
      node = (struct fdt_node *)(((uintptr_t)(prop->data + prop->len + 3)) & ~0x3); 
    } else if (node->token == fdt_end_node) {
      level--;
      node++;
    } else if (node->token == fdt_nop) {
      node++;
    } else {
      return NULL;
    }
  } while (level > 0);
  return NULL;
}

uint32_t
fdt_read_prop_u32(struct fdt_node *prop, int offset) {
  struct fdt_prop *p = (struct fdt_prop *)prop;
  return *(uint32_t *)&(p->data[offset]);
}

uint64_t
fdt_read_prop_u64(struct fdt_node *prop, int offset) {
  struct fdt_prop *p = (struct fdt_prop *)prop;
  return *(uint64_t *)&(p->data[offset]);
}
