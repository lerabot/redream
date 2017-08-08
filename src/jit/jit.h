#ifndef JIT_H
#define JIT_H

#include <stdio.h>
#include "core/list.h"
#include "core/rb_tree.h"

struct address_space;
struct cfa;
struct cprop;
struct dce;
struct ir;
struct lse;
struct ra;
struct val;

enum {
  JIT_BRANCH_STATIC,
  JIT_BRANCH_STATIC_TRUE,
  JIT_BRANCH_STATIC_FALSE,
  JIT_BRANCH_DYNAMIC,
  JIT_BRANCH_DYNAMIC_TRUE,
  JIT_BRANCH_DYNAMIC_FALSE,
};

enum {
  JIT_REASON_UNKNOWN,
  JIT_REASON_FASTMEM,
};

struct jit_block {
  /* address of source block in guest memory */
  uint32_t guest_addr;
  int guest_size;

  /* destination address of terminating branch */
  int branch_type;
  uint32_t branch_addr;

  /* address of next instruction after branch */
  uint32_t next_addr;

  /* is block an idle loop */
  int idle_loop;

  /* number of guest instructions in block */
  int num_instrs;

  /* estimated number of guest cycles to execute block */
  int num_cycles;

  /* maps guest instructions to host instructions */
  void **source_map;

  /* which guest instructions use fastmem */
  int8_t *fastmem;

  /* address of compiled block in host memory */
  void *host_addr;
  int host_size;

  /* reason the block was invalidated */
  int invalidate_reason;

  /* edges to other blocks */
  struct list in_edges;
  struct list out_edges;

  /* lookup map iterators */
  struct rb_node it;
  struct rb_node rit;
};

struct jit_edge {
  struct jit_block *src;
  struct jit_block *dst;

  /* location of branch instruction in host memory */
  void *branch;

  /* has this branch been patched */
  int patched;

  /* iterators for edge lists */
  struct list_node in_it;
  struct list_node out_it;
};

struct jit {
  char tag[32];

  struct jit_frontend *frontend;
  struct jit_backend *backend;
  struct exception_handler *exc_handler;

  /* passes */
  struct cfa *cfa;
  struct lse *lse;
  struct cprop *cprop;
  struct esimp *esimp;
  struct dce *dce;
  struct ra *ra;

  /* scratch compilation buffer */
  uint8_t ir_buffer[1024 * 1024 * 2];

  /* compiled blocks */
  struct rb_tree blocks;
  struct rb_tree reverse_blocks;

  /* compiled block perf map */
  FILE *perf_map;

  /* dump ir to application directory as blocks compile */
  int dump_code;
};

struct jit *jit_create(const char *tag, struct jit_frontend *frontend,
                       struct jit_backend *backend);
void jit_destroy(struct jit *jit);

void jit_run(struct jit *jit, int cycles);

void jit_compile_code(struct jit *jit, uint32_t guest_addr);
void jit_link_code(struct jit *jit, void *code, uint32_t target);

void jit_invalidate_code(struct jit *jit);
void jit_free_code(struct jit *jit);

#endif
