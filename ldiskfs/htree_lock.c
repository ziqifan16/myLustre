/*
 * fs/ldiskfs/htree_lock.c
 *
 * Copyright (c) 2011, 2012, Intel Corporation.
 *
 * Author: Liang Zhen <liang@whamcloud.com>
 */
#include <linux/jbd2.h>
#include <linux/hash.h>
#include <linux/module.h>
#include <linux/htree_lock.h>

enum {
	HTREE_LOCK_BIT_EX	= (1 << HTREE_LOCK_EX),
	HTREE_LOCK_BIT_PW	= (1 << HTREE_LOCK_PW),
	HTREE_LOCK_BIT_PR	= (1 << HTREE_LOCK_PR),
	HTREE_LOCK_BIT_CW	= (1 << HTREE_LOCK_CW),
	HTREE_LOCK_BIT_CR	= (1 << HTREE_LOCK_CR),
};

enum {
	HTREE_LOCK_COMPAT_EX	= 0,
	HTREE_LOCK_COMPAT_PW	= HTREE_LOCK_COMPAT_EX | HTREE_LOCK_BIT_CR,
	HTREE_LOCK_COMPAT_PR	= HTREE_LOCK_COMPAT_PW | HTREE_LOCK_BIT_PR,
	HTREE_LOCK_COMPAT_CW	= HTREE_LOCK_COMPAT_PW | HTREE_LOCK_BIT_CW,
	HTREE_LOCK_COMPAT_CR	= HTREE_LOCK_COMPAT_CW | HTREE_LOCK_BIT_PR |
				  HTREE_LOCK_BIT_PW,
};

static int htree_lock_compat[] = {
	[HTREE_LOCK_EX]		HTREE_LOCK_COMPAT_EX,
	[HTREE_LOCK_PW]		HTREE_LOCK_COMPAT_PW,
	[HTREE_LOCK_PR]		HTREE_LOCK_COMPAT_PR,
	[HTREE_LOCK_CW]		HTREE_LOCK_COMPAT_CW,
	[HTREE_LOCK_CR]		HTREE_LOCK_COMPAT_CR,
};

/* max allowed htree-lock depth.
 * We only need depth=3 for ldiskfs although user can have higher value. */
#define HTREE_LOCK_DEP_MAX	16

#ifdef HTREE_LOCK_DEBUG

static char *hl_name[] = {
	[HTREE_LOCK_EX]		"EX",
	[HTREE_LOCK_PW]		"PW",
	[HTREE_LOCK_PR]		"PR",
	[HTREE_LOCK_CW]		"CW",
	[HTREE_LOCK_CR]		"CR",
};

/* lock stats */
struct htree_lock_node_stats {
	unsigned long long	blocked[HTREE_LOCK_MAX];
	unsigned long long	granted[HTREE_LOCK_MAX];
	unsigned long long	retried[HTREE_LOCK_MAX];
	unsigned long long	events;
};

struct htree_lock_stats {
	struct htree_lock_node_stats	nodes[HTREE_LOCK_DEP_MAX];
	unsigned long long	granted[HTREE_LOCK_MAX];
	unsigned long long	blocked[HTREE_LOCK_MAX];
};

static struct htree_lock_stats hl_stats;

void htree_lock_stat_reset(void)
{
	memset(&hl_stats, 0, sizeof(hl_stats));
}

void htree_lock_stat_print(int depth)
{
	int     i;
	int	j;

	printk(KERN_DEBUG "HTREE LOCK STATS:\n");
	for (i = 0; i < HTREE_LOCK_MAX; i++) {
		printk(KERN_DEBUG "[%s]: G [%10llu], B [%10llu]\n",
		       hl_name[i], hl_stats.granted[i], hl_stats.blocked[i]);
	}
	for (i = 0; i < depth; i++) {
		printk(KERN_DEBUG "HTREE CHILD [%d] STATS:\n", i);
		for (j = 0; j < HTREE_LOCK_MAX; j++) {
			printk(KERN_DEBUG
				"[%s]: G [%10llu], B [%10llu], R [%10llu]\n",
				hl_name[j], hl_stats.nodes[i].granted[j],
				hl_stats.nodes[i].blocked[j],
				hl_stats.nodes[i].retried[j]);
		}
	}
}

#define lk_grant_inc(m)       do { hl_stats.granted[m]++; } while (0)
#define lk_block_inc(m)       do { hl_stats.blocked[m]++; } while (0)
#define ln_grant_inc(d, m)    do { hl_stats.nodes[d].granted[m]++; } while (0)
#define ln_block_inc(d, m)    do { hl_stats.nodes[d].blocked[m]++; } while (0)
#define ln_retry_inc(d, m)    do { hl_stats.nodes[d].retried[m]++; } while (0)
#define ln_event_inc(d)       do { hl_stats.nodes[d].events++; } while (0)

#else /* !DEBUG */

void htree_lock_stat_reset(void) {}
void htree_lock_stat_print(int depth) {}

#define lk_grant_inc(m)	      do {} while (0)
#define lk_block_inc(m)	      do {} while (0)
#define ln_grant_inc(d, m)    do {} while (0)
#define ln_block_inc(d, m)    do {} while (0)
#define ln_retry_inc(d, m)    do {} while (0)
#define ln_event_inc(d)	      do {} while (0)

#endif /* DEBUG */

EXPORT_SYMBOL(htree_lock_stat_reset);
EXPORT_SYMBOL(htree_lock_stat_print);

#define HTREE_DEP_ROOT		  (-1)

#define htree_spin_lock(lhead, dep)				\
	bit_spin_lock((dep) + 1, &(lhead)->lh_lock)
#define htree_spin_unlock(lhead, dep)				\
	bit_spin_unlock((dep) + 1, &(lhead)->lh_lock)

#define htree_key_event_ignore(child, ln)			\
	(!((child)->lc_events & (1 << (ln)->ln_mode)))

static int
htree_key_list_empty(struct htree_lock_node *ln)
{
	return list_empty(&ln->ln_major_list) && list_empty(&ln->ln_minor_list);
}

static void
htree_key_list_del_init(struct htree_lock_node *ln)
{
	struct htree_lock_node *tmp = NULL;

	if (!list_empty(&ln->ln_minor_list)) {
		tmp = list_entry(ln->ln_minor_list.next,
				 struct htree_lock_node, ln_minor_list);
		list_del_init(&ln->ln_minor_list);
	}

	if (list_empty(&ln->ln_major_list))
		return;

	if (tmp == NULL) { /* not on minor key list */
		list_del_init(&ln->ln_major_list);
	} else {
		BUG_ON(!list_empty(&tmp->ln_major_list));
		list_replace_init(&ln->ln_major_list, &tmp->ln_major_list);
	}
}

static void
htree_key_list_replace_init(struct htree_lock_node *old,
			    struct htree_lock_node *new)
{
	if (!list_empty(&old->ln_major_list))
		list_replace_init(&old->ln_major_list, &new->ln_major_list);

	if (!list_empty(&old->ln_minor_list))
		list_replace_init(&old->ln_minor_list, &new->ln_minor_list);
}

static void
htree_key_event_enqueue(struct htree_lock_child *child,
			struct htree_lock_node *ln, int dep, void *event)
{
	struct htree_lock_node *tmp;

	/* NB: ALWAYS called holding lhead::lh_lock(dep) */
	BUG_ON(ln->ln_mode == HTREE_LOCK_NL);
	if (event == NULL || htree_key_event_ignore(child, ln))
		return;

	/* shouldn't be a very long list */
	list_for_each_entry(tmp, &ln->ln_alive_list, ln_alive_list) {
		if (tmp->ln_mode == HTREE_LOCK_NL) {
			ln_event_inc(dep);
			if (child->lc_callback != NULL)
				child->lc_callback(tmp->ln_ev_target, event);
		}
	}
}

static int
htree_node_lock_enqueue(struct htree_lock *newlk, struct htree_lock *curlk,
			unsigned dep, int wait, void *event)
{
	struct htree_lock_child *child = &newlk->lk_head->lh_children[dep];
	struct htree_lock_node *newln = &newlk->lk_nodes[dep];
	struct htree_lock_node *curln = &curlk->lk_nodes[dep];

	/* NB: ALWAYS called holding lhead::lh_lock(dep) */
	/* NB: we only expect PR/PW lock mode at here, only these two modes are
	 * allowed for htree_node_lock(asserted in htree_node_lock_internal),
	 * NL is only used for listener, user can't directly require NL mode */
	if ((curln->ln_mode == HTREE_LOCK_NL) ||
	    (curln->ln_mode != HTREE_LOCK_PW &&
	     newln->ln_mode != HTREE_LOCK_PW)) {
		/* no conflict, attach it on granted list of @curlk */
		if (curln->ln_mode != HTREE_LOCK_NL) {
			list_add(&newln->ln_granted_list,
				 &curln->ln_granted_list);
		} else {
			/* replace key owner */
			htree_key_list_replace_init(curln, newln);
		}

		list_add(&newln->ln_alive_list, &curln->ln_alive_list);
		htree_key_event_enqueue(child, newln, dep, event);
		ln_grant_inc(dep, newln->ln_mode);
		return 1; /* still hold lh_lock */
	}

	if (!wait) { /* can't grant and don't want to wait */
		ln_retry_inc(dep, newln->ln_mode);
		newln->ln_mode = HTREE_LOCK_INVAL;
		return -1; /* don't wait and just return -1 */
	}

	newlk->lk_task = current;
	set_current_state(TASK_UNINTERRUPTIBLE);
	/* conflict, attach it on blocked list of curlk */
	list_add_tail(&newln->ln_blocked_list, &curln->ln_blocked_list);
	list_add(&newln->ln_alive_list, &curln->ln_alive_list);
	ln_block_inc(dep, newln->ln_mode);

	htree_spin_unlock(newlk->lk_head, dep);
	/* wait to be given the lock */
	if (newlk->lk_task != NULL)
		schedule();
	/* granted, no doubt, wake up will set me RUNNING */
	if (event == NULL || htree_key_event_ignore(child, newln))
		return 0; /* granted without lh_lock */

	htree_spin_lock(newlk->lk_head, dep);
	htree_key_event_enqueue(child, newln, dep, event);
	return 1; /* still hold lh_lock */
}

/*
 * get PR/PW access to particular tree-node according to @dep and @key,
 * it will return -1 if @wait is false and can't immediately grant this lock.
 * All listeners(HTREE_LOCK_NL) on @dep and with the same @key will get
 * @event if it's not NULL.
 * NB: ALWAYS called holding lhead::lh_lock
 */
static int
htree_node_lock_internal(struct htree_lock_head *lhead, struct htree_lock *lck,
			 htree_lock_mode_t mode, u32 key, unsigned dep,
			 int wait, void *event)
{
	LIST_HEAD		(list);
	struct htree_lock	*tmp;
	struct htree_lock	*tmp2;
	u16			major;
	u16			minor;
	u8			reverse;
	u8			ma_bits;
	u8			mi_bits;

	BUG_ON(mode != HTREE_LOCK_PW && mode != HTREE_LOCK_PR);
	BUG_ON(htree_node_is_granted(lck, dep));

	key = hash_long(key, lhead->lh_hbits);

	mi_bits = lhead->lh_hbits >> 1;
	ma_bits = lhead->lh_hbits - mi_bits;

	lck->lk_nodes[dep].ln_major_key = major = key & ((1U << ma_bits) - 1);
	lck->lk_nodes[dep].ln_minor_key = minor = key >> ma_bits;
	lck->lk_nodes[dep].ln_mode = mode;

	/*
	 * The major key list is an ordered list, so searches are started
	 * at the end of the list that is numerically closer to major_key,
	 * so at most half of the list will be walked (for well-distributed
	 * keys). The list traversal aborts early if the expected key
	 * location is passed.
	 */
	reverse = (major >= (1 << (ma_bits - 1)));

	if (reverse) {
		list_for_each_entry_reverse(tmp,
					&lhead->lh_children[dep].lc_list,
					lk_nodes[dep].ln_major_list) {
			if (tmp->lk_nodes[dep].ln_major_key == major) {
				goto search_minor;

			} else if (tmp->lk_nodes[dep].ln_major_key < major) {
				/* attach _after_ @tmp */
				list_add(&lck->lk_nodes[dep].ln_major_list,
					 &tmp->lk_nodes[dep].ln_major_list);
				goto out_grant_major;
			}
		}

		list_add(&lck->lk_nodes[dep].ln_major_list,
			 &lhead->lh_children[dep].lc_list);
		goto out_grant_major;

	} else {
		list_for_each_entry(tmp, &lhead->lh_children[dep].lc_list,
				    lk_nodes[dep].ln_major_list) {
			if (tmp->lk_nodes[dep].ln_major_key == major) {
				goto search_minor;

			} else if (tmp->lk_nodes[dep].ln_major_key > major) {
				/* insert _before_ @tmp */
				list_add_tail(&lck->lk_nodes[dep].ln_major_list,
					&tmp->lk_nodes[dep].ln_major_list);
				goto out_grant_major;
			}
		}

		list_add_tail(&lck->lk_nodes[dep].ln_major_list,
			      &lhead->lh_children[dep].lc_list);
		goto out_grant_major;
	}

 search_minor:
	/*
	 * NB: minor_key list doesn't have a "head", @list is just a
	 * temporary stub for helping list searching, make sure it's removed
	 * after searching.
	 * minor_key list is an ordered list too.
	 */
	list_add_tail(&list, &tmp->lk_nodes[dep].ln_minor_list);

	reverse = (minor >= (1 << (mi_bits - 1)));

	if (reverse) {
		list_for_each_entry_reverse(tmp2, &list,
					    lk_nodes[dep].ln_minor_list) {
			if (tmp2->lk_nodes[dep].ln_minor_key == minor) {
				goto out_enqueue;

			} else if (tmp2->lk_nodes[dep].ln_minor_key < minor) {
				/* attach _after_ @tmp2 */
				list_add(&lck->lk_nodes[dep].ln_minor_list,
					 &tmp2->lk_nodes[dep].ln_minor_list);
				goto out_grant_minor;
			}
		}

		list_add(&lck->lk_nodes[dep].ln_minor_list, &list);

	} else {
		list_for_each_entry(tmp2, &list,
				    lk_nodes[dep].ln_minor_list) {
			if (tmp2->lk_nodes[dep].ln_minor_key == minor) {
				goto out_enqueue;

			} else if (tmp2->lk_nodes[dep].ln_minor_key > minor) {
				/* insert _before_ @tmp2 */
				list_add_tail(&lck->lk_nodes[dep].ln_minor_list,
					&tmp2->lk_nodes[dep].ln_minor_list);
				goto out_grant_minor;
			}
		}

		list_add_tail(&lck->lk_nodes[dep].ln_minor_list, &list);
	}

 out_grant_minor:
	if (list.next == &lck->lk_nodes[dep].ln_minor_list) {
		/* new lock @lck is the first one on minor_key list, which
		 * means it has the smallest minor_key and it should
		 * replace @tmp as minor_key owner */
		list_replace_init(&tmp->lk_nodes[dep].ln_major_list,
				  &lck->lk_nodes[dep].ln_major_list);
	}
	/* remove the temporary head */
	list_del(&list);

 out_grant_major:
	ln_grant_inc(dep, lck->lk_nodes[dep].ln_mode);
	return 1; /* granted with holding lh_lock */

 out_enqueue:
	list_del(&list); /* remove temprary head */
	return htree_node_lock_enqueue(lck, tmp2, dep, wait, event);
}

/*
 * release the key of @lck at level @dep, and grant any blocked locks.
 * caller will still listen on @key if @event is not NULL, which means
 * caller can see a event (by event_cb) while granting any lock with
 * the same key at level @dep.
 * NB: ALWAYS called holding lhead::lh_lock
 * NB: listener will not block anyone because listening mode is HTREE_LOCK_NL
 */
static void
htree_node_unlock_internal(struct htree_lock_head *lhead,
			   struct htree_lock *curlk, unsigned dep, void *event)
{
	struct htree_lock_node	*curln = &curlk->lk_nodes[dep];
	struct htree_lock	*grtlk = NULL;
	struct htree_lock_node	*grtln;
	struct htree_lock	*poslk;
	struct htree_lock	*tmplk;

	if (!htree_node_is_granted(curlk, dep))
		return;

	if (!list_empty(&curln->ln_granted_list)) {
		/* there is another granted lock */
		grtlk = list_entry(curln->ln_granted_list.next,
				   struct htree_lock,
				   lk_nodes[dep].ln_granted_list);
		list_del_init(&curln->ln_granted_list);
	}

	if (grtlk == NULL && !list_empty(&curln->ln_blocked_list)) {
		/*
		 * @curlk is the only granted lock, so we confirmed:
		 * a) curln is key owner (attached on major/minor_list),
		 *    so if there is any blocked lock, it should be attached
		 *    on curln->ln_blocked_list
		 * b) we always can grant the first blocked lock
		 */
		grtlk = list_entry(curln->ln_blocked_list.next,
				   struct htree_lock,
				   lk_nodes[dep].ln_blocked_list);
		BUG_ON(grtlk->lk_task == NULL);
		wake_up_process(grtlk->lk_task);
	}

	if (event != NULL &&
	    lhead->lh_children[dep].lc_events != HTREE_EVENT_DISABLE) {
		curln->ln_ev_target = event;
		curln->ln_mode = HTREE_LOCK_NL; /* listen! */
	} else {
		curln->ln_mode = HTREE_LOCK_INVAL;
	}

	if (grtlk == NULL) { /* I must be the only one locking this key */
		struct htree_lock_node *tmpln;

		BUG_ON(htree_key_list_empty(curln));

		if (curln->ln_mode == HTREE_LOCK_NL) /* listening */
			return;

		/* not listening */
		if (list_empty(&curln->ln_alive_list)) { /* no more listener */
			htree_key_list_del_init(curln);
			return;
		}

		tmpln = list_entry(curln->ln_alive_list.next,
				   struct htree_lock_node, ln_alive_list);

		BUG_ON(tmpln->ln_mode != HTREE_LOCK_NL);

		htree_key_list_replace_init(curln, tmpln);
		list_del_init(&curln->ln_alive_list);

		return;
	}

	/* have a granted lock */
	grtln = &grtlk->lk_nodes[dep];
	if (!list_empty(&curln->ln_blocked_list)) {
		/* only key owner can be on both lists */
		BUG_ON(htree_key_list_empty(curln));

		if (list_empty(&grtln->ln_blocked_list)) {
			list_add(&grtln->ln_blocked_list,
				 &curln->ln_blocked_list);
		}
		list_del_init(&curln->ln_blocked_list);
	}
	/*
	 * NB: this is the tricky part:
	 * We have only two modes for child-lock (PR and PW), also,
	 * only owner of the key (attached on major/minor_list) can be on
	 * both blocked_list and granted_list, so @grtlk must be one
	 * of these two cases:
	 *
	 * a) @grtlk is taken from granted_list, which means we've granted
	 *    more than one lock so @grtlk has to be PR, the first blocked
	 *    lock must be PW and we can't grant it at all.
	 *    So even @grtlk is not owner of the key (empty blocked_list),
	 *    we don't care because we can't grant any lock.
	 * b) we just grant a new lock which is taken from head of blocked
	 *    list, and it should be the first granted lock, and it should
	 *    be the first one linked on blocked_list.
	 *
	 * Either way, we can get correct result by iterating blocked_list
	 * of @grtlk, and don't have to bother on how to find out
	 * owner of current key.
	 */
	list_for_each_entry_safe(poslk, tmplk, &grtln->ln_blocked_list,
				 lk_nodes[dep].ln_blocked_list) {
		if (grtlk->lk_nodes[dep].ln_mode == HTREE_LOCK_PW ||
		    poslk->lk_nodes[dep].ln_mode == HTREE_LOCK_PW)
			break;
		/* grant all readers */
		list_del_init(&poslk->lk_nodes[dep].ln_blocked_list);
		list_add(&poslk->lk_nodes[dep].ln_granted_list,
			 &grtln->ln_granted_list);

		BUG_ON(poslk->lk_task == NULL);
		wake_up_process(poslk->lk_task);
	}

	/* if @curln is the owner of this key, replace it with @grtln */
	if (!htree_key_list_empty(curln))
		htree_key_list_replace_init(curln, grtln);

	if (curln->ln_mode == HTREE_LOCK_INVAL)
		list_del_init(&curln->ln_alive_list);
}

/*
 * it's just wrapper of htree_node_lock_internal, it returns 1 on granted
 * and 0 only if @wait is false and can't grant it immediately
 */
int
htree_node_lock_try(struct htree_lock *lck, htree_lock_mode_t mode,
		    u32 key, unsigned dep, int wait, void *event)
{
	struct htree_lock_head *lhead = lck->lk_head;
	int rc;

	BUG_ON(dep >= lck->lk_depth);
	BUG_ON(lck->lk_mode == HTREE_LOCK_INVAL);

	htree_spin_lock(lhead, dep);
	rc = htree_node_lock_internal(lhead, lck, mode, key, dep, wait, event);
	if (rc != 0)
		htree_spin_unlock(lhead, dep);
	return rc >= 0;
}
EXPORT_SYMBOL(htree_node_lock_try);

/* it's wrapper of htree_node_unlock_internal */
void
htree_node_unlock(struct htree_lock *lck, unsigned dep, void *event)
{
	struct htree_lock_head *lhead = lck->lk_head;

	BUG_ON(dep >= lck->lk_depth);
	BUG_ON(lck->lk_mode == HTREE_LOCK_INVAL);

	htree_spin_lock(lhead, dep);
	htree_node_unlock_internal(lhead, lck, dep, event);
	htree_spin_unlock(lhead, dep);
}
EXPORT_SYMBOL(htree_node_unlock);

/* stop listening on child-lock level @dep */
void
htree_node_stop_listen(struct htree_lock *lck, unsigned dep)
{
	struct htree_lock_node *ln = &lck->lk_nodes[dep];
	struct htree_lock_node *tmp;

	BUG_ON(htree_node_is_granted(lck, dep));
	BUG_ON(!list_empty(&ln->ln_blocked_list));
	BUG_ON(!list_empty(&ln->ln_granted_list));

	if (!htree_node_is_listening(lck, dep))
		return;

	htree_spin_lock(lck->lk_head, dep);
	ln->ln_mode = HTREE_LOCK_INVAL;
	ln->ln_ev_target = NULL;

	if (htree_key_list_empty(ln)) { /* not owner */
		list_del_init(&ln->ln_alive_list);
		goto out;
	}

	/* I'm the owner... */
	if (list_empty(&ln->ln_alive_list)) { /* no more listener */
		htree_key_list_del_init(ln);
		goto out;
	}

	tmp = list_entry(ln->ln_alive_list.next,
			 struct htree_lock_node, ln_alive_list);

	BUG_ON(tmp->ln_mode != HTREE_LOCK_NL);
	htree_key_list_replace_init(ln, tmp);
	list_del_init(&ln->ln_alive_list);
 out:
	htree_spin_unlock(lck->lk_head, dep);
}
EXPORT_SYMBOL(htree_node_stop_listen);

/* release all child-locks if we have any */
static void
htree_node_release_all(struct htree_lock *lck)
{
	int	i;

	for (i = 0; i < lck->lk_depth; i++) {
		if (htree_node_is_granted(lck, i))
			htree_node_unlock(lck, i, NULL);
		else if (htree_node_is_listening(lck, i))
			htree_node_stop_listen(lck, i);
	}
}

/*
 * obtain htree lock, it could be blocked inside if there's conflict
 * with any granted or blocked lock and @wait is true.
 * NB: ALWAYS called holding lhead::lh_lock
 */
static int
htree_lock_internal(struct htree_lock *lck, int wait)
{
	struct htree_lock_head *lhead = lck->lk_head;
	int	granted = 0;
	int	blocked = 0;
	int	i;

	for (i = 0; i < HTREE_LOCK_MAX; i++) {
		if (lhead->lh_ngranted[i] != 0)
			granted |= 1 << i;
		if (lhead->lh_nblocked[i] != 0)
			blocked |= 1 << i;
	}
	if ((htree_lock_compat[lck->lk_mode] & granted) != granted ||
	    (htree_lock_compat[lck->lk_mode] & blocked) != blocked) {
		/* will block current lock even it just conflicts with any
		 * other blocked lock, so lock like EX wouldn't starve */
		if (!wait)
			return -1;
		lhead->lh_nblocked[lck->lk_mode]++;
		lk_block_inc(lck->lk_mode);

		lck->lk_task = current;
		list_add_tail(&lck->lk_blocked_list, &lhead->lh_blocked_list);

		set_current_state(TASK_UNINTERRUPTIBLE);
		htree_spin_unlock(lhead, HTREE_DEP_ROOT);
		/* wait to be given the lock */
		if (lck->lk_task != NULL)
			schedule();
		/* granted, no doubt. wake up will set me RUNNING */
		return 0; /* without lh_lock */
	}
	lhead->lh_ngranted[lck->lk_mode]++;
	lk_grant_inc(lck->lk_mode);
	return 1;
}

/* release htree lock. NB: ALWAYS called holding lhead::lh_lock */
static void
htree_unlock_internal(struct htree_lock *lck)
{
	struct htree_lock_head *lhead = lck->lk_head;
	struct htree_lock *tmp;
	struct htree_lock *tmp2;
	int granted = 0;
	int i;

	BUG_ON(lhead->lh_ngranted[lck->lk_mode] == 0);

	lhead->lh_ngranted[lck->lk_mode]--;
	lck->lk_mode = HTREE_LOCK_INVAL;

	for (i = 0; i < HTREE_LOCK_MAX; i++) {
		if (lhead->lh_ngranted[i] != 0)
			granted |= 1 << i;
	}
	list_for_each_entry_safe(tmp, tmp2,
				 &lhead->lh_blocked_list, lk_blocked_list) {
		/* conflict with any granted lock? */
		if ((htree_lock_compat[tmp->lk_mode] & granted) != granted)
			break;

		list_del_init(&tmp->lk_blocked_list);

		BUG_ON(lhead->lh_nblocked[tmp->lk_mode] == 0);

		lhead->lh_nblocked[tmp->lk_mode]--;
		lhead->lh_ngranted[tmp->lk_mode]++;
		granted |= 1 << tmp->lk_mode;

		BUG_ON(tmp->lk_task == NULL);
		wake_up_process(tmp->lk_task);
	}
}

/* it's wrapper of htree_lock_internal and exported interface.
 * It always return 1 with granted lock if @wait is true, it can return 0
 * if @wait is false and locking request can't be granted immediately */
int
htree_lock_try(struct htree_lock *lck, struct htree_lock_head *lhead,
	       htree_lock_mode_t mode, int wait)
{
	int	rc;

	BUG_ON(lck->lk_depth > lhead->lh_depth);
	BUG_ON(lck->lk_head != NULL);
	BUG_ON(lck->lk_task != NULL);

	lck->lk_head = lhead;
	lck->lk_mode = mode;

	htree_spin_lock(lhead, HTREE_DEP_ROOT);
	rc = htree_lock_internal(lck, wait);
	if (rc != 0)
		htree_spin_unlock(lhead, HTREE_DEP_ROOT);
	return rc >= 0;
}
EXPORT_SYMBOL(htree_lock_try);

/* it's wrapper of htree_unlock_internal and exported interface.
 * It will release all htree_node_locks and htree_lock */
void
htree_unlock(struct htree_lock *lck)
{
	BUG_ON(lck->lk_head == NULL);
	BUG_ON(lck->lk_mode == HTREE_LOCK_INVAL);

	htree_node_release_all(lck);

	htree_spin_lock(lck->lk_head, HTREE_DEP_ROOT);
	htree_unlock_internal(lck);
	htree_spin_unlock(lck->lk_head, HTREE_DEP_ROOT);
	lck->lk_head = NULL;
	lck->lk_task = NULL;
}
EXPORT_SYMBOL(htree_unlock);

/* change lock mode */
void
htree_change_mode(struct htree_lock *lck, htree_lock_mode_t mode)
{
	BUG_ON(lck->lk_mode == HTREE_LOCK_INVAL);
	lck->lk_mode = mode;
}
EXPORT_SYMBOL(htree_change_mode);

/* release htree lock, and lock it again with new mode.
 * This function will first release all htree_node_locks and htree_lock,
 * then try to gain htree_lock with new @mode.
 * It always return 1 with granted lock if @wait is true, it can return 0
 * if @wait is false and locking request can't be granted immediately */
int
htree_change_lock_try(struct htree_lock *lck, htree_lock_mode_t mode, int wait)
{
	struct htree_lock_head *lhead = lck->lk_head;
	int rc;

	BUG_ON(lhead == NULL);
	BUG_ON(lck->lk_mode == mode);
	BUG_ON(lck->lk_mode == HTREE_LOCK_INVAL || mode == HTREE_LOCK_INVAL);

	htree_node_release_all(lck);

	htree_spin_lock(lhead, HTREE_DEP_ROOT);
	htree_unlock_internal(lck);
	lck->lk_mode = mode;
	rc = htree_lock_internal(lck, wait);
	if (rc != 0)
		htree_spin_unlock(lhead, HTREE_DEP_ROOT);
	return rc >= 0;
}
EXPORT_SYMBOL(htree_change_lock_try);

/* create a htree_lock head with @depth levels (number of child-locks),
 * it is a per resoruce structure */
struct htree_lock_head *
htree_lock_head_alloc(unsigned depth, unsigned hbits, unsigned priv)
{
	struct htree_lock_head *lhead;
	int  i;

	if (depth > HTREE_LOCK_DEP_MAX) {
		printk(KERN_ERR "%d is larger than max htree_lock depth %d\n",
			depth, HTREE_LOCK_DEP_MAX);
		return NULL;
	}

	lhead = kzalloc(offsetof(struct htree_lock_head,
				 lh_children[depth]) + priv, GFP_NOFS);
	if (lhead == NULL)
		return NULL;

	if (hbits < HTREE_HBITS_MIN)
		lhead->lh_hbits = HTREE_HBITS_MIN;
	else if (hbits > HTREE_HBITS_MAX)
		lhead->lh_hbits = HTREE_HBITS_MAX;

	lhead->lh_lock = 0;
	lhead->lh_depth = depth;
	INIT_LIST_HEAD(&lhead->lh_blocked_list);
	if (priv > 0) {
		lhead->lh_private = (void *)lhead +
			offsetof(struct htree_lock_head, lh_children[depth]);
	}

	for (i = 0; i < depth; i++) {
		INIT_LIST_HEAD(&lhead->lh_children[i].lc_list);
		lhead->lh_children[i].lc_events = HTREE_EVENT_DISABLE;
	}
	return lhead;
}
EXPORT_SYMBOL(htree_lock_head_alloc);

/* free the htree_lock head */
void
htree_lock_head_free(struct htree_lock_head *lhead)
{
	int     i;

	BUG_ON(!list_empty(&lhead->lh_blocked_list));
	for (i = 0; i < lhead->lh_depth; i++)
		BUG_ON(!list_empty(&lhead->lh_children[i].lc_list));
	kfree(lhead);
}
EXPORT_SYMBOL(htree_lock_head_free);

/* register event callback for @events of child-lock at level @dep */
void
htree_lock_event_attach(struct htree_lock_head *lhead, unsigned dep,
			unsigned events, htree_event_cb_t callback)
{
	BUG_ON(lhead->lh_depth <= dep);
	lhead->lh_children[dep].lc_events = events;
	lhead->lh_children[dep].lc_callback = callback;
}
EXPORT_SYMBOL(htree_lock_event_attach);

/* allocate a htree_lock, which is per-thread structure, @pbytes is some
 * extra-bytes as private data for caller */
struct htree_lock *
htree_lock_alloc(unsigned depth, unsigned pbytes)
{
	struct htree_lock *lck;
	int i = offsetof(struct htree_lock, lk_nodes[depth]);

	if (depth > HTREE_LOCK_DEP_MAX) {
		printk(KERN_ERR "%d is larger than max htree_lock depth %d\n",
			depth, HTREE_LOCK_DEP_MAX);
		return NULL;
	}
	lck = kzalloc(i + pbytes, GFP_NOFS);
	if (lck == NULL)
		return NULL;

	if (pbytes != 0)
		lck->lk_private = (void *)lck + i;
	lck->lk_mode = HTREE_LOCK_INVAL;
	lck->lk_depth = depth;
	INIT_LIST_HEAD(&lck->lk_blocked_list);

	for (i = 0; i < depth; i++) {
		struct htree_lock_node *node = &lck->lk_nodes[i];

		node->ln_mode = HTREE_LOCK_INVAL;
		INIT_LIST_HEAD(&node->ln_major_list);
		INIT_LIST_HEAD(&node->ln_minor_list);
		INIT_LIST_HEAD(&node->ln_alive_list);
		INIT_LIST_HEAD(&node->ln_blocked_list);
		INIT_LIST_HEAD(&node->ln_granted_list);
	}

	return lck;
}
EXPORT_SYMBOL(htree_lock_alloc);

/* free htree_lock node */
void
htree_lock_free(struct htree_lock *lck)
{
	BUG_ON(lck->lk_mode != HTREE_LOCK_INVAL);
	kfree(lck);
}
EXPORT_SYMBOL(htree_lock_free);
