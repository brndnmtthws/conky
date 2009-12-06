/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
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
 * Copyright (c) 2005-2009 Brenden Matthews, Philip Kovacs, et. al.
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

static unsigned long g_time = 0;
static unsigned long long previous_total = 0;
static struct process *first_process = 0;

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
	phe = malloc(sizeof(struct proc_hash_entry));
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
		if (pr->name) {
			free(pr->name);
		}
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
	return 0;
}

/* Create a new process object and insert it into the process list */
static struct process *new_process(int p)
{
	struct process *process;
	process = (struct process *) malloc(sizeof(struct process));

	// clean up memory first
	memset(process, 0, sizeof(struct process));

	/* Do stitching necessary for doubly linked list */
	process->name = 0;
	process->previous = 0;
	process->next = first_process;
	if (process->next) {
		process->next->previous = process;
	}
	first_process = process;

	process->pid = p;
	process->time_stamp = 0;
	process->previous_user_time = ULONG_MAX;
	process->previous_kernel_time = ULONG_MAX;
#ifdef IOSTATS
	process->previous_read_bytes = ULLONG_MAX;
	process->previous_write_bytes = ULLONG_MAX;
#endif /* IOSTATS */
	process->counted = 1;

	/* process_find_name(process); */

	/* add the process to the hash table */
	hash_process(process);

	return process;
}

/******************************************
 * Functions							  *
 ******************************************/

/******************************************
 * Extract information from /proc		  *
 ******************************************/

/* These are the guts that extract information out of /proc.
 * Anyone hoping to port wmtop should look here first. */
static int process_parse_stat(struct process *process)
{
	char line[BUFFER_LEN] = { 0 }, filename[BUFFER_LEN], procname[BUFFER_LEN];
	char state[4];
	int ps;
	unsigned long user_time = 0;
	unsigned long kernel_time = 0;
	int rc;
	char *r, *q;
	int endl;
	int nice_val;
	char *lparen, *rparen;

	snprintf(filename, sizeof(filename), PROCFS_TEMPLATE, process->pid);

	ps = open(filename, O_RDONLY);
	if (ps < 0) {
		/* The process must have finished in the last few jiffies! */
		return 1;
	}

	/* Mark process as up-to-date. */
	process->time_stamp = g_time;

	rc = read(ps, line, sizeof(line));
	close(ps);
	if (rc < 0) {
		return 1;
	}

	/* Extract cpu times from data in /proc filesystem */
	lparen = strchr(line, '(');
	rparen = strrchr(line, ')');
	if(!lparen || !rparen || rparen < lparen)
		return 1; // this should not happen

	rc = MIN((unsigned)(rparen - lparen - 1), sizeof(procname) - 1);
	strncpy(procname, lparen + 1, rc);
	procname[rc] = '\0';
	rc = sscanf(rparen + 1, "%3s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu "
			"%lu %*s %*s %*s %d %*s %*s %*s %u %u", state, &process->user_time,
			&process->kernel_time, &nice_val, &process->vsize, &process->rss);
	if (rc < 6) {
		NORM_ERR("scaning data for %s failed, got only %d fields", procname, rc);
		return 1;
	}

	if(state[0]=='R')
		++ info.run_procs;

	/* remove any "kdeinit: " */
	if (procname == strstr(procname, "kdeinit")) {
		snprintf(filename, sizeof(filename), PROCFS_CMDLINE_TEMPLATE,
				process->pid);

		ps = open(filename, O_RDONLY);
		if (ps < 0) {
			/* The process must have finished in the last few jiffies! */
			return 1;
		}

		endl = read(ps, line, sizeof(line));
		close(ps);

		/* null terminate the input */
		line[endl] = 0;
		/* account for "kdeinit: " */
		if ((char *) line == strstr(line, "kdeinit: ")) {
			r = ((char *) line) + 9;
		} else {
			r = (char *) line;
		}

		q = procname;
		/* stop at space */
		while (*r && *r != ' ') {
			*q++ = *r++;
		}
		*q = 0;
	}

	if (process->name) {
		free(process->name);
	}
	process->name = strndup(procname, text_buffer_size);
	process->rss *= getpagesize();

	process->total_cpu_time = process->user_time + process->kernel_time;
	if (process->previous_user_time == ULONG_MAX) {
		process->previous_user_time = process->user_time;
	}
	if (process->previous_kernel_time == ULONG_MAX) {
		process->previous_kernel_time = process->kernel_time;
	}

	/* strangely, the values aren't monotonous */
	if (process->previous_user_time > process->user_time)
		process->previous_user_time = process->user_time;

	if (process->previous_kernel_time > process->kernel_time)
		process->previous_kernel_time = process->kernel_time;

	/* store the difference of the user_time */
	user_time = process->user_time - process->previous_user_time;
	kernel_time = process->kernel_time - process->previous_kernel_time;

	/* backup the process->user_time for next time around */
	process->previous_user_time = process->user_time;
	process->previous_kernel_time = process->kernel_time;

	/* store only the difference of the user_time here... */
	process->user_time = user_time;
	process->kernel_time = kernel_time;

	return 0;
}

#ifdef IOSTATS
static int process_parse_io(struct process *process)
{
	static const char *read_bytes_str="read_bytes:";
	static const char *write_bytes_str="write_bytes:";

	char line[BUFFER_LEN] = { 0 }, filename[BUFFER_LEN];
	int ps;
	int rc;
	char *pos, *endpos;
	unsigned long long read_bytes, write_bytes;

	snprintf(filename, sizeof(filename), PROCFS_TEMPLATE_IO, process->pid);

	ps = open(filename, O_RDONLY);
	if (ps < 0) {
		/* The process must have finished in the last few jiffies!
		 * Or, the kernel doesn't support I/O accounting.
		 */
		return 1;
	}

	rc = read(ps, line, sizeof(line));
	close(ps);
	if (rc < 0) {
		return 1;
	}

	pos = strstr(line, read_bytes_str);
	if (pos == NULL) {
		/* these should not happen (unless the format of the file changes) */
		return 1;
	}
	pos += strlen(read_bytes_str);
	process->read_bytes = strtoull(pos, &endpos, 10);
	if (endpos == pos) {
		return 1;
	}

	pos = strstr(line, write_bytes_str);
	if (pos == NULL) {
		return 1;
	}
	pos += strlen(write_bytes_str);
	process->write_bytes = strtoull(pos, &endpos, 10);
	if (endpos == pos) {
		return 1;
	}

	if (process->previous_read_bytes == ULLONG_MAX) {
		process->previous_read_bytes = process->read_bytes;
	}
	if (process->previous_write_bytes == ULLONG_MAX) {
		process->previous_write_bytes = process->write_bytes;
	}

	/* store the difference of the byte counts */
	read_bytes = process->read_bytes - process->previous_read_bytes;
	write_bytes = process->write_bytes - process->previous_write_bytes;

	/* backup the counts for next time around */
	process->previous_read_bytes = process->read_bytes;
	process->previous_write_bytes = process->write_bytes;

	/* store only the difference here... */
	process->read_bytes = read_bytes;
	process->write_bytes = write_bytes;

	return 0;
}
#endif /* IOSTATS */

/******************************************
 * Get process structure for process pid  *
 ******************************************/

/* This function seems to hog all of the CPU time.
 * I can't figure out why - it doesn't do much. */
static int calculate_stats(struct process *process)
{
	int rc;

	/* compute each process cpu usage by reading /proc/<proc#>/stat */
	rc = process_parse_stat(process);
	if (rc)	return 1;
	/* rc = process_parse_statm(process); if (rc) return 1; */

#ifdef IOSTATS
	rc = process_parse_io(process);
	if (rc) return 1;
#endif /* IOSTATS */

	/*
	 * Check name against the exclusion list
	 */
	/* if (process->counted && exclusion_expression &&
	 * !regexec(exclusion_expression, process->name, 0, 0, 0))
	 * process->counted = 0; */

	return 0;
}

/******************************************
 * Update process table					  *
 ******************************************/

static int update_process_table(void)
{
	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir("/proc"))) {
		return 1;
	}

	info.run_procs = 0;
	++g_time;

	/* Get list of processes from /proc directory */
	while ((entry = readdir(dir))) {
		pid_t pid;

		if (!entry) {
			/* Problem reading list of processes */
			closedir(dir);
			return 1;
		}

		if (sscanf(entry->d_name, "%d", &pid) > 0) {
			struct process *p;

			p = find_process(pid);
			if (!p) {
				p = new_process(pid);
			}

			/* compute each process cpu usage */
			calculate_stats(p);
		}
	}

	closedir(dir);

	return 0;
}

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

	if (p->name) {
		free(p->name);
	}
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
 * Calculate cpu total					  *
 ******************************************/
#define TMPL_SHORTPROC "%*s %llu %llu %llu %llu"
#define TMPL_LONGPROC "%*s %llu %llu %llu %llu %llu %llu %llu %llu"

static unsigned long long calc_cpu_total(void)
{
	unsigned long long total = 0;
	unsigned long long t = 0;
	int rc;
	int ps;
	char line[BUFFER_LEN] = { 0 };
	unsigned long long cpu = 0;
	unsigned long long niceval = 0;
	unsigned long long systemval = 0;
	unsigned long long idle = 0;
	unsigned long long iowait = 0;
	unsigned long long irq = 0;
	unsigned long long softirq = 0;
	unsigned long long steal = 0;
	const char *template =
		KFLAG_ISSET(KFLAG_IS_LONGSTAT) ? TMPL_LONGPROC : TMPL_SHORTPROC;

	ps = open("/proc/stat", O_RDONLY);
	rc = read(ps, line, sizeof(line));
	close(ps);
	if (rc < 0) {
		return 0;
	}

	sscanf(line, template, &cpu, &niceval, &systemval, &idle, &iowait, &irq,
			&softirq, &steal);
	total = cpu + niceval + systemval + idle + iowait + irq + softirq + steal;

	t = total - previous_total;
	previous_total = total;

	return t;
}

/******************************************
 * Calculate each processes cpu			  *
 ******************************************/

inline static void calc_cpu_each(unsigned long long total)
{
	struct process *p = first_process;

	while (p) {
		p->amount = 100.0 * (cpu_separate ? info.cpu_count : 1) *
			(p->user_time + p->kernel_time) / (float) total;

		p = p->next;
	}
}

#ifdef IOSTATS
static void calc_io_each(void)
{
	struct process *p;
	unsigned long long sum = 0;

	for (p = first_process; p; p = p->next)
		sum += p->read_bytes + p->write_bytes;

	if(sum == 0)
		sum = 1; /* to avoid having NANs if no I/O occured */
	for (p = first_process; p; p = p->next)
		p->io_perc = 100.0 * (p->read_bytes + p->write_bytes) / (float) sum;
}
#endif /* IOSTATS */

/******************************************
 * Find the top processes				  *
 ******************************************/

/* cpu comparison function for prio queue */
static int compare_cpu(void *va, void *vb)
{
	struct process *a = va, *b = vb;

	if (a->amount < b->amount) {
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
	struct process *a = va, *b = vb;

	if (a->rss < b->rss) {
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
	struct process *a = va, *b = vb;

	return b->total_cpu_time - a->total_cpu_time;
}

#ifdef IOSTATS
/* I/O comparision function for prio queue */
static int compare_io(void *va, void *vb)
{
	struct process *a = va, *b = vb;

	if (a->io_perc < b->io_perc) {
		return 1;
	} else if (a->io_perc > b->io_perc) {
		return -1;
	} else {
		return 0;
	}
}
#endif /* IOSTATS */

/* ****************************************************************** *
 * Get a sorted list of the top cpu hogs and top mem hogs.			  *
 * Results are stored in the cpu,mem arrays in decreasing order[0-9]. *
 * ****************************************************************** */

void process_find_top(struct process **cpu, struct process **mem,
		struct process **ptime
#ifdef IOSTATS
		, struct process **io
#endif /* IOSTATS */
		)
{
	prio_queue_t cpu_queue, mem_queue, time_queue
#ifdef IOSTATS
		, io_queue
#endif
		;
	struct process *cur_proc = NULL;
	unsigned long long total = 0;
	int i;

	if (!top_cpu && !top_mem && !top_time
#ifdef IOSTATS
			&& !top_io
#endif /* IOSTATS */
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

#ifdef IOSTATS
	io_queue = init_prio_queue();
	pq_set_compare(io_queue, &compare_io);
	pq_set_max_size(io_queue, MAX_SP);
#endif

	total = calc_cpu_total();	/* calculate the total of the processor */
	update_process_table();		/* update the table with process list */
	calc_cpu_each(total);		/* and then the percentage for each task */
	process_cleanup();			/* cleanup list from exited processes */
#ifdef IOSTATS
	calc_io_each();			/* percentage of I/O for each task */
#endif /* IOSTATS */

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
#ifdef IOSTATS
		if (top_io) {
			insert_prio_elem(io_queue, cur_proc);
		}
#endif /* IOSTATS */
		cur_proc = cur_proc->next;
	}

	for (i = 0; i < MAX_SP; i++) {
		if (top_cpu)
			cpu[i] = pop_prio_elem(cpu_queue);
		if (top_mem)
			mem[i] = pop_prio_elem(mem_queue);
		if (top_time)
			ptime[i] = pop_prio_elem(time_queue);
#ifdef IOSTATS
		if (top_io)
			io[i] = pop_prio_elem(io_queue);
#endif /* IOSTATS */
	}
	free_prio_queue(cpu_queue);
	free_prio_queue(mem_queue);
	free_prio_queue(time_queue);
#ifdef IOSTATS
	free_prio_queue(io_queue);
#endif /* IOSTATS */
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
		return strndup(buf, text_buffer_size);
	}
	if (width >= snprintf(buf, sizeof buf, "%lu:%02u", nt, nn)) {
		return strndup(buf, text_buffer_size);
	}
	nn = nt % 60;		// minutes past the hour
	nt /= 60;			// total hours
	if (width >= snprintf(buf, sizeof buf, "%lu,%02u", nt, nn)) {
		return strndup(buf, text_buffer_size);
	}
	nn = nt;			// now also hours
	if (width >= snprintf(buf, sizeof buf, "%uh", nn)) {
		return strndup(buf, text_buffer_size);
	}
	nn /= 24;			// now days
	if (width >= snprintf(buf, sizeof buf, "%ud", nn)) {
		return strndup(buf, text_buffer_size);
	}
	nn /= 7;			// now weeks
	if (width >= snprintf(buf, sizeof buf, "%uw", nn)) {
		return strndup(buf, text_buffer_size);
	}
	// well shoot, this outta' fit...
	return strndup("<inf>", text_buffer_size);
}

struct top_data {
	struct process **list;
	int num;
	int was_parsed;
	char *s;
};

static unsigned int top_name_width = 15;

/* return zero on success, non-zero otherwise */
int set_top_name_width(const char *s)
{
	if (!s)
		return 0;
	return !(sscanf(s, "%u", &top_name_width) == 1);
}

static void print_top_name(struct text_object *obj, char *p, int p_max_size)
{
	struct top_data *td = obj->data.opaque;
	int width;

	if (!td || !td->list || !td->list[td->num])
		return;

	width = MIN(p_max_size, (int)top_name_width + 1);
	snprintf(p, width + 1, "%-*s", width, td->list[td->num]->name);
}

static void print_top_mem(struct text_object *obj, char *p, int p_max_size)
{
	struct top_data *td = obj->data.opaque;
	int width;

	if (!td || !td->list || !td->list[td->num])
		return;

	width = MIN(p_max_size, 7);
	snprintf(p, width, "%6.2f", (float) ((float)td->list[td->num]->rss / info.memmax) / 10);
}

static void print_top_time(struct text_object *obj, char *p, int p_max_size)
{
	struct top_data *td = obj->data.opaque;
	int width;
	char *timeval;

	if (!td || !td->list || !td->list[td->num])
		return;

	width = MIN(p_max_size, 10);
	timeval = format_time(td->list[td->num]->total_cpu_time, 9);
	snprintf(p, width, "%9s", timeval);
	free(timeval);
}

#define PRINT_TOP_GENERATOR(name, width, fmt, field) \
static void print_top_##name(struct text_object *obj, char *p, int p_max_size) \
{ \
	struct top_data *td = obj->data.opaque; \
	if (!td || !td->list || !td->list[td->num]) \
		return; \
	snprintf(p, MIN(p_max_size, width), fmt, td->list[td->num]->field); \
}

#define PRINT_TOP_HR_GENERATOR(name, field, denom) \
static void print_top_##name(struct text_object *obj, char *p, int p_max_size) \
{ \
	struct top_data *td = obj->data.opaque; \
	if (!td || !td->list || !td->list[td->num]) \
		return; \
	human_readable(td->list[td->num]->field / denom, p, p_max_size); \
}

PRINT_TOP_GENERATOR(cpu, 7, "%6.2f", amount)
PRINT_TOP_GENERATOR(pid, 6, "%5i", pid)
PRINT_TOP_HR_GENERATOR(mem_res, rss, 1)
PRINT_TOP_HR_GENERATOR(mem_vsize, vsize, 1)
#ifdef IOSTATS
PRINT_TOP_HR_GENERATOR(read_bytes, read_bytes, update_interval)
PRINT_TOP_HR_GENERATOR(write_bytes, write_bytes, update_interval)
PRINT_TOP_GENERATOR(io_perc, 7, "%6.2f", io_perc)
#endif /* IOSTATS */

static void free_top(struct text_object *obj)
{
	struct top_data *td = obj->data.opaque;

	if (!td)
		return;
	if (td->s)
		free(td->s);
	free(obj->data.opaque);
	obj->data.opaque = NULL;
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

	obj->data.opaque = td = malloc(sizeof(struct top_data));
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
#ifdef IOSTATS
	} else if (strcmp(&s[3], "_io") == EQUAL) {
		td->list = info.io;
		top_io = 1;
#endif /* IOSTATS */
	} else {
#ifdef IOSTATS
		NORM_ERR("Must be top, top_mem, top_time or top_io");
#else /* IOSTATS */
		NORM_ERR("Must be top, top_mem or top_time");
#endif /* IOSTATS */
		free(obj->data.opaque);
		obj->data.opaque = 0;
		return 0;
	}

	td->s = strndup(arg, text_buffer_size);

	if (sscanf(arg, "%63s %i", buf, &n) == 2) {
		if (strcmp(buf, "name") == EQUAL) {
			obj->callbacks.print = &print_top_name;
		} else if (strcmp(buf, "cpu") == EQUAL) {
			obj->callbacks.print = &print_top_cpu;
		} else if (strcmp(buf, "pid") == EQUAL) {
			obj->callbacks.print = &print_top_pid;
		} else if (strcmp(buf, "mem") == EQUAL) {
			obj->callbacks.print = &print_top_mem;
			add_update_callback(&update_meminfo);
		} else if (strcmp(buf, "time") == EQUAL) {
			obj->callbacks.print = &print_top_time;
		} else if (strcmp(buf, "mem_res") == EQUAL) {
			obj->callbacks.print = &print_top_mem_res;
		} else if (strcmp(buf, "mem_vsize") == EQUAL) {
			obj->callbacks.print = &print_top_mem_vsize;
#ifdef IOSTATS
		} else if (strcmp(buf, "io_read") == EQUAL) {
			obj->callbacks.print = &print_top_read_bytes;
		} else if (strcmp(buf, "io_write") == EQUAL) {
			obj->callbacks.print = &print_top_write_bytes;
		} else if (strcmp(buf, "io_perc") == EQUAL) {
			obj->callbacks.print = &print_top_io_perc;
#endif /* IOSTATS */
		} else {
			NORM_ERR("invalid type arg for top");
#ifdef IOSTATS
			NORM_ERR("must be one of: name, cpu, pid, mem, time, mem_res, mem_vsize, "
					"io_read, io_write, io_perc");
#else /* IOSTATS */
			NORM_ERR("must be one of: name, cpu, pid, mem, time, mem_res, mem_vsize");
#endif /* IOSTATS */
			return 0;
		}
		if (n < 1 || n > 10) {
			NORM_ERR("invalid num arg for top. Must be between 1 and 10.");
			return 0;
		} else {
			td->num = n - 1;
		}
	} else {
		NORM_ERR("invalid argument count for top");
		return 0;
	}
	obj->callbacks.free = &free_top;
	return 1;
}
