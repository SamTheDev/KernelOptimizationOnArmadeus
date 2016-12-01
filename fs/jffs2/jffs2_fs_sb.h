/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright © 2001-2007 Red Hat, Inc.
 *
 * Created by David Woodhouse <dwmw2@infradead.org>
 *
 * For licensing information, see the file 'LICENCE' in this directory.
 *
 */

#ifndef _JFFS2_FS_SB
#define _JFFS2_FS_SB

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/rwsem.h>
/* Pierre : */
#include "pierre.h"

#if JFFS2_DISABLE_RA == 1
#include <linux/backing-dev.h>
#endif

#define JFFS2_SB_FLAG_RO 1
#define JFFS2_SB_FLAG_SCANNING 2 /* Flash scanning is in progress */
#define JFFS2_SB_FLAG_BUILDING 4 /* File system building is in progress */

struct jffs2_inodirty;

/* A struct for the overall file system control.  Pointers to
   jffs2_sb_info structs are named `c' in the source code.
   Nee jffs_control
*/
struct jffs2_sb_info {
	struct mtd_info *mtd;
	
	/* Pierre */
#if JFFS2_DISABLE_RA == 1
	struct backing_dev_info bdi;
#endif

	uint32_t highest_ino;
	uint32_t checked_ino;

	unsigned int flags;

	struct task_struct *gc_task;	/* GC task struct */
	struct completion gc_thread_start; /* GC thread start completion */
	struct completion gc_thread_exit; /* GC thread exit completion port */

	struct mutex alloc_sem;		/* Used to protect all the following
					   fields, and also to protect against
					   out-of-order writing of nodes. And GC. */
	uint32_t cleanmarker_size;	/* Size of an _inline_ CLEANMARKER
					 (i.e. zero for OOB CLEANMARKER */

	uint32_t flash_size;
	uint32_t used_size;
	uint32_t dirty_size;
	uint32_t wasted_size;
	uint32_t free_size;
	uint32_t erasing_size;
	uint32_t bad_size;
	uint32_t sector_size;
	uint32_t unchecked_size;

	uint32_t nr_free_blocks;
	uint32_t nr_erasing_blocks;

	/* Number of free blocks there must be before we... */
	uint8_t resv_blocks_write;	/* ... allow a normal filesystem write */
	uint8_t resv_blocks_deletion;	/* ... allow a normal filesystem deletion */
	uint8_t resv_blocks_gctrigger;	/* ... wake up the GC thread */
	uint8_t resv_blocks_gcbad;	/* ... pick a block from the bad_list to GC */
	uint8_t resv_blocks_gcmerge;	/* ... merge pages when garbage collecting */
	/* Number of 'very dirty' blocks before we trigger immediate GC */
	uint8_t vdirty_blocks_gctrigger;

	uint32_t nospc_dirty_size;

	uint32_t nr_blocks;
	struct jffs2_eraseblock *blocks;	/* The whole array of blocks. Used for getting blocks
						 * from the offset (blocks[ofs / sector_size]) */
	struct jffs2_eraseblock *nextblock;	/* The block we're currently filling */

	struct jffs2_eraseblock *gcblock;	/* The block we're currently garbage-collecting */

	struct list_head clean_list;		/* Blocks 100% full of clean data */
	struct list_head very_dirty_list;	/* Blocks with lots of dirty space */
	struct list_head dirty_list;		/* Blocks with some dirty space */
	struct list_head erasable_list;		/* Blocks which are completely dirty, and need erasing */
	struct list_head erasable_pending_wbuf_list;	/* Blocks which need erasing but only after the current wbuf is flushed */
	struct list_head erasing_list;		/* Blocks which are currently erasing */
	struct list_head erase_checking_list;	/* Blocks which are being checked and marked */
	struct list_head erase_pending_list;	/* Blocks which need erasing now */
	struct list_head erase_complete_list;	/* Blocks which are erased and need the clean marker written to them */
	struct list_head free_list;		/* Blocks which are free and ready to be used */
	struct list_head bad_list;		/* Bad blocks. */
	struct list_head bad_used_list;		/* Bad blocks with valid data in. */

	spinlock_t erase_completion_lock;	/* Protect free_list and erasing_list
						   against erase completion handler */
	wait_queue_head_t erase_wait;		/* For waiting for erases to complete */

	wait_queue_head_t inocache_wq;
	struct jffs2_inode_cache **inocache_list;
	spinlock_t inocache_lock;

	/* Sem to allow jffs2_garbage_collect_deletion_dirent to
	   drop the erase_completion_lock while it's holding a pointer
	   to an obsoleted node. I don't like this. Alternatives welcomed. */
	struct mutex erase_free_sem;

	uint32_t wbuf_pagesize; /* 0 for NOR and other flashes with no wbuf */

#ifdef CONFIG_JFFS2_FS_WBUF_VERIFY
	unsigned char *wbuf_verify; /* read-back buffer for verification */
#endif
#ifdef CONFIG_JFFS2_FS_WRITEBUFFER
	unsigned char *wbuf; /* Write-behind buffer for NAND flash */
	uint32_t wbuf_ofs;
	uint32_t wbuf_len;
	struct jffs2_inodirty *wbuf_inodes;
	struct rw_semaphore wbuf_sem;	/* Protects the write buffer */

	unsigned char *oobbuf;
	int oobavail; /* How many bytes are available for JFFS2 in OOB */
#endif

	struct jffs2_summary *summary;		/* Summary information */

#ifdef CONFIG_JFFS2_FS_XATTR
#define XATTRINDEX_HASHSIZE	(57)
	uint32_t highest_xid;
	uint32_t highest_xseqno;
	struct list_head xattrindex[XATTRINDEX_HASHSIZE];
	struct list_head xattr_unchecked;
	struct list_head xattr_dead_list;
	struct jffs2_xattr_ref *xref_dead_list;
	struct jffs2_xattr_ref *xref_temp;
	struct rw_semaphore xattr_sem;
	uint32_t xdatum_mem_usage;
	uint32_t xdatum_mem_threshold;
#endif
	/* OS-private pointer for getting back to master superblock info */
	void *os_priv;
};

#endif /* _JFFS2_FB_SB */
