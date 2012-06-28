/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005 Adi Zaimi, Dan Piponi <dan@tanelorn.demon.co.uk>,
 *					  Dave Clark <clarkd@skynet.ca>
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "prioqueue.h"
#include "top.h"
#include "logging.h"

/* hash table size - always a power of 2 */
#define HTABSIZE 256

struct process *first_process = 0;

unsigned long g_time = 0;

/* a simple hash table to speed up find_process() */
struct proc_hash_entry {
	struct proc_hash_entry *next;
	struct process *proc;
};
static struct proc_hash_entry proc_hash_table[HTABSIZE];

static void hash_process(struct process *p)
{
	struct proc_hash_entry *phe;
	static char first_run = 1;
	int bucket;

	/* better make sure all next pointers are zero upon first access */
	if (first_run) {
		memset(proc_hash_table, 0, sizeof(struct proc_hash_entry) * HTABSIZE);
		first_run = 0;
	}

	/* get the bucket index */
	bucket = p->pid & (HTABSIZE - 1);

	/* insert a new element on bucket's top */
	phe = (struct proc_hash_entry *)malloc(sizeof(struct proc_hash_entry));
	phe->proc = p;
	phe->next = proc_hash_table[bucket].next;
	proc_hash_table[bucket].next = phe;
}

static void unhash_process(struct process *p)
{
	struct proc_hash_entry *phe, *tmp;

	/* get the bucket head */
	phe = &proc_hash_table[p->pid & (HTABSIZE - 1)];
	/* find the entry pointing to p and drop it */
	while (phe->next) {
		if (phe->next->proc == p) {
			tmp = phe->next;
			phe->next = phe->next->next;
			free(tmp);
			return;
		}
		phe = phe->next;
	}
}

static void __unhash_all_processes(struct proc_hash_entry *phe)
{
	if (phe->next)
		__unhash_all_processes(phe->next);
	free(phe->next);
}

static void unhash_all_processes(void)
{
	int i;

	for (i = 0; i < HTABSIZE; i++) {
		__unhash_all_processes(&proc_hash_table[i]);
		proc_hash_table[i].next = NULL;
	}
}

struct process *get_first_process(void)
{
	return first_process;
}

void free_all_processes(void)
{
	struct process *next = NULL, *pr = first_process;

	while (pr) {
		next = pr->next;
		free_and_zero(pr->name);
		free(pr);
		pr = next;
	}
	first_process = NULL;

	/* drop the whole hash table */
	unhash_all_processes();
}

struct process *get_process_by_name(const char *name)
{
	struct process *p = first_process;

	while (p) {
		if (p->name && !strcmp(p->name, name))
			return p;
		p = p->next;
	}
	return 0;
}

static struct process *find_process(pid_t pid)
{
	struct proc_hash_entry *phe;

	phe = &proc_hash_table[pid & (HTABSIZE - 1)];
	while (phe->next) {
		if (phe->next->proc->pid == pid)
			return phe->next->proc;
		phe = phe->next;
	}
	return NULL;
}

static struct process *new_process(pid_t pid)
{
	struct process *p = (struct process *) malloc(sizeof(struct process));

	/* Do stitching necessary for doubly linked list */
	p->previous = NULL;
	p->next = first_process;
	if (p->next) {
		p->next->previous = p;
	}
	first_process = p;

	p->pid = pid;
	p->name = 0;
	p->amount = 0;
	p->user_time = 0;
	p->total = 0;
	p->kernel_time = 0;
	p->previous_user_time = ULONG_MAX;
	p->previous_kernel_time = ULONG_MAX;
	p->total_cpu_time = 0;
	p->vsize = 0;
	p->rss = 0;
#ifdef BUILD_IOSTATS
	p->read_bytes = 0;
	p->previous_read_bytes = ULLONG_MAX;
	p->write_bytes = 0;
	p->previous_write_bytes = ULLONG_MAX;
	p->io_perc = 0;
#endif /* BUILD_IOSTATS */
	p->time_stamp = 0;
	p->counted = 1;
	p->changed = 0;

	/* process_find_name(p); */

	/* add the process to the hash table */
	hash_process(p);

	return p;
}

/* Get / create a new process object and insert it into the process list */
struct process *get_process(pid_t pid)
{
	struct process *p = find_process(pid);
	return p ? p : new_process(pid);
}

/******************************************
 * Functions							  *
 ******************************************/

/******************************************
 * Destroy and remove a process           *
 ******************************************/

static void delete_process(struct process *p)
{
#if defined(PARANOID)
	assert(p->id == 0x0badfeed);

	/*
	 * Ensure that deleted processes aren't reused.
	 */
	p->id = 0x007babe;
#endif /* defined(PARANOID) */

	/*
	 * Maintain doubly linked list.
	 */
	if (p->next)
		p->next->previous = p->previous;
	if (p->previous)
		p->previous->next = p->next;
	else
		first_process = p->next;

	free_and_zero(p->name);
	/* remove the process from the hash table */
	unhash_process(p);
	free(p);
}

/******************************************
 * Strip dead process entries			  *
 ******************************************/

static void process_cleanup(void)
{

	struct process *p = first_process;

	while (p) {
		struct process *current = p;

#if defined(PARANOID)
		assert(p->id == 0x0badfeed);
#endif /* defined(PARANOID) */

		p = p->next;
		/* Delete processes that have died */
		if (current->time_stamp != g_time) {
			delete_process(current);
		}
	}
}

/******************************************
 * Find the top processes				  *
 ******************************************/

/* cpu comparison function for prio queue */
static int compare_cpu(void *va, void *vb)
{
	struct process *a = (struct process *)va, *b = (struct process *)vb;

	if (b->amount > a->amount) {
		return 1;
	} else if (a->amount > b->amount) {
		return -1;
	} else {
		return 0;
	}
}

/* mem comparison function for prio queue */
static int compare_mem(void *va, void *vb)
{
	struct process *a = (struct process *)va, *b = (struct process *)vb;

	if (b->rss > a->rss) {
		return 1;
	} else if (a->rss > b->rss) {
		return -1;
	} else {
		return 0;
	}
}

/* CPU time comparision function for prio queue */
static int compare_time(void *va, void *vb)
{
	struct process *a = (struct process *)va, *b = (struct process *)vb;

	if (b->total_cpu_time > a->total_cpu_time) {
		return 1;
	} else if (b->total_cpu_time < a->total_cpu_time) {
		return -1;
	} else {
		return 0;
	}
}

#ifdef BUILD_IOSTATS
/* I/O comparision function for prio queue */
static int compare_io(void *va, void *vb)
{
	struct process *a = (struct process *)va, *b = (struct process *)vb;

	if (b->io_perc > a->io_perc) {
		return 1;
	} else if (a->io_perc > b->io_perc) {
		return -1;
	} else {
		return 0;
	}
}
#endif /* BUILD_IOSTATS */

/* ****************************************************************** *
 * Get a sorted list of the top cpu hogs and top mem hogs.			  *
 * Results are stored in the cpu,mem arrays in decreasing order[0-9]. *
 * ****************************************************************** */

static void process_find_top(struct process **cpu, struct process **mem,
		struct process **ptime
#ifdef BUILD_IOSTATS
		, struct process **io
#endif /* BUILD_IOSTATS */
		)
{
	prio_queue_t cpu_queue, mem_queue, time_queue;
#ifdef BUILD_IOSTATS
	prio_queue_t io_queue;
#endif
	struct process *cur_proc = NULL;
	int i;

	if (!top_cpu && !top_mem && !top_time
#ifdef BUILD_IOSTATS
			&& !top_io
#endif /* BUILD_IOSTATS */
			&& !top_running
	   ) {
		return;
	}

	cpu_queue = init_prio_queue();
	pq_set_compare(cpu_queue, &compare_cpu);
	pq_set_max_size(cpu_queue, MAX_SP);

	mem_queue = init_prio_queue();
	pq_set_compare(mem_queue, &compare_mem);
	pq_set_max_size(mem_queue, MAX_SP);

	time_queue = init_prio_queue();
	pq_set_compare(time_queue, &compare_time);
	pq_set_max_size(time_queue, MAX_SP);

#ifdef BUILD_IOSTATS
	io_queue = init_prio_queue();
	pq_set_compare(io_queue, &compare_io);
	pq_set_max_size(io_queue, MAX_SP);
#endif

	/* g_time is the time_stamp entry for process.  It is updated when the
	 * process information is updated to indicate that the process is still
	 * alive (and must not be removed from the process list in
	 * process_cleanup()) */
	++g_time;

	/* OS-specific function updating process list */
	get_top_info();

	process_cleanup();			/* cleanup list from exited processes */

	cur_proc = first_process;

	while (cur_proc != NULL) {
		if (top_cpu) {
			insert_prio_elem(cpu_queue, cur_proc);
		}
		if (top_mem) {
			insert_prio_elem(mem_queue, cur_proc);
		}
		if (top_time) {
			insert_prio_elem(time_queue, cur_proc);
		}
#ifdef BUILD_IOSTATS
		if (top_io) {
			insert_prio_elem(io_queue, cur_proc);
		}
#endif /* BUILD_IOSTATS */
		cur_proc = cur_proc->next;
	}

	for (i = 0; i < MAX_SP; i++) {
		if (top_cpu)
			cpu[i] = (process*)pop_prio_elem(cpu_queue);
		if (top_mem)
			mem[i] = (process*)pop_prio_elem(mem_queue);
		if (top_time)
			ptime[i] = (process*)pop_prio_elem(time_queue);
#ifdef BUILD_IOSTATS
		if (top_io)
			io[i] = (process*)pop_prio_elem(io_queue);
#endif /* BUILD_IOSTATS */
	}
	free_prio_queue(cpu_queue);
	free_prio_queue(mem_queue);
	free_prio_queue(time_queue);
#ifdef BUILD_IOSTATS
	free_prio_queue(io_queue);
#endif /* BUILD_IOSTATS */
}

int update_top(void)
{
	// XXX: this was a separate callback. and it should be again, as soon as it's possible
	update_meminfo();

	process_find_top(info.cpu, info.memu, info.time
#ifdef BUILD_IOSTATS
					 , info.io
#endif
					);
	info.first_process = get_first_process();
	return 0;
}

static char *format_time(unsigned long timeval, const int width)
{
	char buf[10];
	unsigned long nt;	// narrow time, for speed on 32-bit
	unsigned cc;		// centiseconds
	unsigned nn;		// multi-purpose whatever

	nt = timeval;
	cc = nt % 100;		// centiseconds past second
	nt /= 100;			// total seconds
	nn = nt % 60;		// seconds past the minute
	nt /= 60;			// total minutes
	if (width >= snprintf(buf, sizeof buf, "%lu:%02u.%02u",
				nt, nn, cc)) {
		return strndup(buf, text_buffer_size.get(*state));
	}
	if (width >= snprintf(buf, sizeof buf, "%lu:%02u", nt, nn)) {
		return strndup(buf, text_buffer_size.get(*state));
	}
	nn = nt % 60;		// minutes past the hour
	nt /= 60;			// total hours
	if (width >= snprintf(buf, sizeof buf, "%lu,%02u", nt, nn)) {
		return strndup(buf, text_buffer_size.get(*state));
	}
	nn = nt;			// now also hours
	if (width >= snprintf(buf, sizeof buf, "%uh", nn)) {
		return strndup(buf, text_buffer_size.get(*state));
	}
	nn /= 24;			// now days
	if (width >= snprintf(buf, sizeof buf, "%ud", nn)) {
		return strndup(buf, text_buffer_size.get(*state));
	}
	nn /= 7;			// now weeks
	if (width >= snprintf(buf, sizeof buf, "%uw", nn)) {
		return strndup(buf, text_buffer_size.get(*state));
	}
	// well shoot, this outta' fit...
	return strndup("<inf>", text_buffer_size.get(*state));
}

struct top_data {
	struct process **list;
	int num;
	int was_parsed;
	char *s;
};

static conky::range_config_setting<unsigned int> top_name_width("top_name_width", 0,
										std::numeric_limits<unsigned int>::max(), 15, true);

static void print_top_name(struct text_object *obj, char *p, int p_max_size)
{
	struct top_data *td = (struct top_data *)obj->data.opaque;
	int width;

	if (!td || !td->list || !td->list[td->num])
		return;

	width = MIN(p_max_size, (int)top_name_width.get(*state) + 1);
	snprintf(p, width + 1, "%-*s", width, td->list[td->num]->name);
}

static void print_top_mem(struct text_object *obj, char *p, int p_max_size)
{
	struct top_data *td = (struct top_data *)obj->data.opaque;
	int width;

	if (!td || !td->list || !td->list[td->num])
		return;

	width = MIN(p_max_size, 7);
	snprintf(p, width, "%6.2f", (float) ((float)td->list[td->num]->rss / info.memmax) / 10);
}

static void print_top_time(struct text_object *obj, char *p, int p_max_size)
{
	struct top_data *td = (struct top_data *)obj->data.opaque;
	int width;
	char *timeval;

	if (!td || !td->list || !td->list[td->num])
		return;

	width = MIN(p_max_size, 10);
	timeval = format_time(td->list[td->num]->total_cpu_time, 9);
	snprintf(p, width, "%9s", timeval);
	free(timeval);
}

static void print_top_user(struct text_object *obj, char *p, int p_max_size)
{
	struct top_data *td = (struct top_data *)obj->data.opaque;

	if (!td || !td->list || !td->list[td->num])
		return;

	snprintf(p, p_max_size, "%.8s", getpwuid(td->list[td->num]->uid)->pw_name);
}

#define PRINT_TOP_GENERATOR(name, width, fmt, field) \
static void print_top_##name(struct text_object *obj, char *p, int p_max_size) \
{ \
	struct top_data *td = (struct top_data *)obj->data.opaque; \
	if (!td || !td->list || !td->list[td->num]) \
		return; \
	snprintf(p, MIN(p_max_size, width), fmt, td->list[td->num]->field); \
}

#define PRINT_TOP_HR_GENERATOR(name, field, denom) \
static void print_top_##name(struct text_object *obj, char *p, int p_max_size) \
{ \
	struct top_data *td = (struct top_data *)obj->data.opaque; \
	if (!td || !td->list || !td->list[td->num]) \
		return; \
	human_readable(td->list[td->num]->field / denom, p, p_max_size); \
}

PRINT_TOP_GENERATOR(cpu, 7, "%6.2f", amount)
PRINT_TOP_GENERATOR(pid, 6, "%5i", pid)
PRINT_TOP_GENERATOR(uid, 6, "%5i", uid)
PRINT_TOP_HR_GENERATOR(mem_res, rss, 1)
PRINT_TOP_HR_GENERATOR(mem_vsize, vsize, 1)
#ifdef BUILD_IOSTATS
PRINT_TOP_HR_GENERATOR(read_bytes, read_bytes, active_update_interval())
PRINT_TOP_HR_GENERATOR(write_bytes, write_bytes, active_update_interval())
PRINT_TOP_GENERATOR(io_perc, 7, "%6.2f", io_perc)
#endif /* BUILD_IOSTATS */

static void free_top(struct text_object *obj)
{
	struct top_data *td = (struct top_data *)obj->data.opaque;

	if (!td)
		return;
	free_and_zero(td->s);
	free_and_zero(obj->data.opaque);
}

int parse_top_args(const char *s, const char *arg, struct text_object *obj)
{
	struct top_data *td;
	char buf[64];
	int n;

	if (!arg) {
		NORM_ERR("top needs arguments");
		return 0;
	}

	obj->data.opaque = td = (struct top_data *)malloc(sizeof(struct top_data));
	memset(td, 0, sizeof(struct top_data));

	if (s[3] == 0) {
		td->list = info.cpu;
		top_cpu = 1;
	} else if (strcmp(&s[3], "_mem") == EQUAL) {
		td->list = info.memu;
		top_mem = 1;
	} else if (strcmp(&s[3], "_time") == EQUAL) {
		td->list = info.time;
		top_time = 1;
#ifdef BUILD_IOSTATS
	} else if (strcmp(&s[3], "_io") == EQUAL) {
		td->list = info.io;
		top_io = 1;
#endif /* BUILD_IOSTATS */
	} else {
#ifdef BUILD_IOSTATS
		NORM_ERR("Must be top, top_mem, top_time or top_io");
#else /* BUILD_IOSTATS */
		NORM_ERR("Must be top, top_mem or top_time");
#endif /* BUILD_IOSTATS */
		free_and_zero(obj->data.opaque);
		return 0;
	}

	td->s = strndup(arg, text_buffer_size.get(*state));

	if (sscanf(arg, "%63s %i", buf, &n) == 2) {
		if (strcmp(buf, "name") == EQUAL) {
			obj->callbacks.print = &print_top_name;
		} else if (strcmp(buf, "cpu") == EQUAL) {
			obj->callbacks.print = &print_top_cpu;
		} else if (strcmp(buf, "pid") == EQUAL) {
			obj->callbacks.print = &print_top_pid;
		} else if (strcmp(buf, "mem") == EQUAL) {
			obj->callbacks.print = &print_top_mem;
		} else if (strcmp(buf, "time") == EQUAL) {
			obj->callbacks.print = &print_top_time;
		} else if (strcmp(buf, "mem_res") == EQUAL) {
			obj->callbacks.print = &print_top_mem_res;
		} else if (strcmp(buf, "mem_vsize") == EQUAL) {
			obj->callbacks.print = &print_top_mem_vsize;
		} else if (strcmp(buf, "uid") == EQUAL) {
			obj->callbacks.print = &print_top_uid;
		} else if (strcmp(buf, "user") == EQUAL) {
			obj->callbacks.print = &print_top_user;
#ifdef BUILD_IOSTATS
		} else if (strcmp(buf, "io_read") == EQUAL) {
			obj->callbacks.print = &print_top_read_bytes;
		} else if (strcmp(buf, "io_write") == EQUAL) {
			obj->callbacks.print = &print_top_write_bytes;
		} else if (strcmp(buf, "io_perc") == EQUAL) {
			obj->callbacks.print = &print_top_io_perc;
#endif /* BUILD_IOSTATS */
		} else {
			NORM_ERR("invalid type arg for top");
#ifdef BUILD_IOSTATS
			NORM_ERR("must be one of: name, cpu, pid, mem, time, mem_res, mem_vsize, "
					"io_read, io_write, io_perc");
#else /* BUILD_IOSTATS */
			NORM_ERR("must be one of: name, cpu, pid, mem, time, mem_res, mem_vsize");
#endif /* BUILD_IOSTATS */
			free_and_zero(td->s);
			free_and_zero(obj->data.opaque);
			return 0;
		}
		if (n < 1 || n > 10) {
			NORM_ERR("invalid num arg for top. Must be between 1 and 10.");
			free_and_zero(td->s);
			free_and_zero(obj->data.opaque);
			return 0;
		} else {
			td->num = n - 1;
		}
	} else {
		NORM_ERR("invalid argument count for top");
		free_and_zero(td->s);
		free_and_zero(obj->data.opaque);
		return 0;
	}
	obj->callbacks.free = &free_top;
	return 1;
}
