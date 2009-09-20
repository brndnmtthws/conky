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

#include "top.h"
#include "logging.h"

static unsigned long g_time = 0;
static unsigned long long previous_total = 0;
static struct process *first_process = 0;

/* a simple hash table to speed up find_process() */
struct proc_hash_entry {
	struct proc_hash_entry *next;
	struct process *proc;
};
static struct proc_hash_entry proc_hash_table[256];

static void hash_process(struct process *p)
{
	struct proc_hash_entry *phe;
	static char first_run = 1;

	/* better make sure all next pointers are zero upon first access */
	if (first_run) {
		memset(proc_hash_table, 0, sizeof(struct proc_hash_entry) * 256);
		first_run = 0;
	}

	/* get the bucket head */
	phe = &proc_hash_table[p->pid % 256];

	/* find the bucket's end */
	while (phe->next)
		phe = phe->next;

	/* append process */
	phe->next = malloc(sizeof(struct proc_hash_entry));
	memset(phe->next, 0, sizeof(struct proc_hash_entry));
	phe->next->proc = p;
}

static void unhash_process(struct process *p)
{
	struct proc_hash_entry *phe, *tmp;

	/* get the bucket head */
	phe = &proc_hash_table[p->pid % 256];

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

	for (i = 0; i < 256; i++) {
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
		if (!strcmp(p->name, name))
			return p;
		p = p->next;
	}
	return 0;
}

static struct process *find_process(pid_t pid)
{
	struct proc_hash_entry *phe;

	phe = &proc_hash_table[pid % 256];
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
	struct information *cur = &info;
	char line[BUFFER_LEN] = { 0 }, filename[BUFFER_LEN], procname[BUFFER_LEN];
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
	rc = sscanf(rparen + 1, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu "
			"%lu %*s %*s %*s %d %*s %*s %*s %u %u", &process->user_time,
			&process->kernel_time, &nice_val, &process->vsize, &process->rss);
	if (rc < 5) {
		NORM_ERR("scaning data for %s failed, got only %d fields", procname, rc);
		return 1;
	}
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

	if (!cur->memmax) {
		update_total_processes();
	}

	process->total_cpu_time = process->user_time + process->kernel_time;
	process->totalmem = (float) (((float) process->rss / cur->memmax) / 10);
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

/* free a sp_process structure */
static void free_sp(struct sorted_process *sp)
{
	free(sp);
}

/* create a new sp_process structure */
static struct sorted_process *malloc_sp(struct process *proc)
{
	struct sorted_process *sp;
	sp = malloc(sizeof(struct sorted_process));
	memset(sp, 0, sizeof(struct sorted_process));
	sp->proc = proc;
	return sp;
}

/* cpu comparison function for insert_sp_element */
static int compare_cpu(struct process *a, struct process *b)
{
	if (a->amount < b->amount) {
		return 1;
	} else if (a->amount > b->amount) {
		return -1;
	} else {
		return 0;
	}
}

/* mem comparison function for insert_sp_element */
static int compare_mem(struct process *a, struct process *b)
{
	if (a->totalmem < b->totalmem) {
		return 1;
	} else if (a->totalmem > b->totalmem) {
		return -1;
	} else {
		return 0;
	}
}

/* CPU time comparision function for insert_sp_element */
static int compare_time(struct process *a, struct process *b)
{
	return b->total_cpu_time - a->total_cpu_time;
}

#ifdef IOSTATS
/* I/O comparision function for insert_sp_element */
static int compare_io(struct process *a, struct process *b)
{
	if (a->io_perc < b->io_perc) {
		return 1;
	} else if (a->io_perc > b->io_perc) {
		return -1;
	} else {
		return 0;
	}
}
#endif /* IOSTATS */

/* insert this process into the list in a sorted fashion,
 * or destroy it if it doesn't fit on the list */
static int insert_sp_element(struct sorted_process *sp_cur,
		struct sorted_process **p_sp_head, struct sorted_process **p_sp_tail,
		int max_elements, int compare_funct(struct process *, struct process *))
{

	struct sorted_process *sp_readthru = NULL, *sp_destroy = NULL;
	int did_insert = 0, x = 0;

	if (*p_sp_head == NULL) {
		*p_sp_head = sp_cur;
		*p_sp_tail = sp_cur;
		return 1;
	}
	for (sp_readthru = *p_sp_head, x = 0;
			sp_readthru != NULL && x < max_elements;
			sp_readthru = sp_readthru->less, x++) {
		if (compare_funct(sp_readthru->proc, sp_cur->proc) > 0 && !did_insert) {
			/* sp_cur is bigger than sp_readthru
			 * so insert it before sp_readthru */
			sp_cur->less = sp_readthru;
			if (sp_readthru == *p_sp_head) {
				/* insert as the new head of the list */
				*p_sp_head = sp_cur;
			} else {
				/* insert inside the list */
				sp_readthru->greater->less = sp_cur;
				sp_cur->greater = sp_readthru->greater;
			}
			sp_readthru->greater = sp_cur;
			/* element was inserted, so increase the counter */
			did_insert = ++x;
		}
	}
	if (x < max_elements && sp_readthru == NULL && !did_insert) {
		/* sp_cur is the smallest element and list isn't full,
		 * so insert at the end */
		(*p_sp_tail)->less = sp_cur;
		sp_cur->greater = *p_sp_tail;
		*p_sp_tail = sp_cur;
		did_insert = x;
	} else if (x >= max_elements) {
		/* We inserted an element and now the list is too big by one.
		 * Destroy the smallest element */
		sp_destroy = *p_sp_tail;
		*p_sp_tail = sp_destroy->greater;
		(*p_sp_tail)->less = NULL;
		free_sp(sp_destroy);
	}
	if (!did_insert) {
		/* sp_cur wasn't added to the sorted list, so destroy it */
		free_sp(sp_cur);
	}
	return did_insert;
}

/* copy the procs in the sorted list to the array, and destroy the list */
static void sp_acopy(struct sorted_process *sp_head, struct process **ar, int max_size)
{
	struct sorted_process *sp_cur, *sp_tmp;
	int x;

	sp_cur = sp_head;
	for (x = 0; x < max_size && sp_cur != NULL; x++) {
		ar[x] = sp_cur->proc;
		sp_tmp = sp_cur;
		sp_cur = sp_cur->less;
		free_sp(sp_tmp);
	}
}

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
	struct sorted_process *spc_head = NULL, *spc_tail = NULL, *spc_cur = NULL;
	struct sorted_process *spm_head = NULL, *spm_tail = NULL, *spm_cur = NULL;
	struct sorted_process *spt_head = NULL, *spt_tail = NULL, *spt_cur = NULL;
#ifdef IOSTATS
	struct sorted_process *spi_head = NULL, *spi_tail = NULL, *spi_cur = NULL;
#endif /* IOSTATS */
	struct process *cur_proc = NULL;
	unsigned long long total = 0;

	if (!top_cpu && !top_mem && !top_time
#ifdef IOSTATS
			&& !top_io
#endif /* IOSTATS */
			&& !top_running
	   ) {
		return;
	}

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
			spc_cur = malloc_sp(cur_proc);
			insert_sp_element(spc_cur, &spc_head, &spc_tail, MAX_SP,
					&compare_cpu);
		}
		if (top_mem) {
			spm_cur = malloc_sp(cur_proc);
			insert_sp_element(spm_cur, &spm_head, &spm_tail, MAX_SP,
					&compare_mem);
		}
		if (top_time) {
			spt_cur = malloc_sp(cur_proc);
			insert_sp_element(spt_cur, &spt_head, &spt_tail, MAX_SP,
					&compare_time);
		}
#ifdef IOSTATS
		if (top_io) {
			spi_cur = malloc_sp(cur_proc);
			insert_sp_element(spi_cur, &spi_head, &spi_tail, MAX_SP,
					&compare_io);
		}
#endif /* IOSTATS */
		cur_proc = cur_proc->next;
	}

	if (top_cpu)	sp_acopy(spc_head, cpu,		MAX_SP);
	if (top_mem)	sp_acopy(spm_head, mem,		MAX_SP);
	if (top_time)	sp_acopy(spt_head, ptime,	MAX_SP);
#ifdef IOSTATS
	if (top_io)		sp_acopy(spi_head, io,		MAX_SP);
#endif /* IOSTATS */
}

int parse_top_args(const char *s, const char *arg, struct text_object *obj)
{
	char buf[64];
	int n;

	if (obj->data.top.was_parsed) {
		return 1;
	}
	obj->data.top.was_parsed = 1;

	if (arg && !obj->data.top.s) {
		obj->data.top.s = strndup(arg, text_buffer_size);
	}

	if (s[3] == 0) {
		obj->type = OBJ_top;
		top_cpu = 1;
	} else if (strcmp(&s[3], "_mem") == EQUAL) {
		obj->type = OBJ_top_mem;
		top_mem = 1;
	} else if (strcmp(&s[3], "_time") == EQUAL) {
		obj->type = OBJ_top_time;
		top_time = 1;
#ifdef IOSTATS
	} else if (strcmp(&s[3], "_io") == EQUAL) {
		obj->type = OBJ_top_io;
		top_io = 1;
#endif /* IOSTATS */
	} else {
#ifdef IOSTATS
		NORM_ERR("Must be top, top_mem, top_time or top_io");
#else /* IOSTATS */
		NORM_ERR("Must be top, top_mem or top_time");
#endif /* IOSTATS */
		return 0;
	}

	if (!arg) {
		NORM_ERR("top needs arguments");
		return 0;
	}

	if (sscanf(arg, "%63s %i", buf, &n) == 2) {
		if (strcmp(buf, "name") == EQUAL) {
			obj->data.top.type = TOP_NAME;
		} else if (strcmp(buf, "cpu") == EQUAL) {
			obj->data.top.type = TOP_CPU;
		} else if (strcmp(buf, "pid") == EQUAL) {
			obj->data.top.type = TOP_PID;
		} else if (strcmp(buf, "mem") == EQUAL) {
			obj->data.top.type = TOP_MEM;
		} else if (strcmp(buf, "time") == EQUAL) {
			obj->data.top.type = TOP_TIME;
		} else if (strcmp(buf, "mem_res") == EQUAL) {
			obj->data.top.type = TOP_MEM_RES;
		} else if (strcmp(buf, "mem_vsize") == EQUAL) {
			obj->data.top.type = TOP_MEM_VSIZE;
#ifdef IOSTATS
		} else if (strcmp(buf, "io_read") == EQUAL) {
			obj->data.top.type = TOP_READ_BYTES;
		} else if (strcmp(buf, "io_write") == EQUAL) {
			obj->data.top.type = TOP_WRITE_BYTES;
		} else if (strcmp(buf, "io_perc") == EQUAL) {
			obj->data.top.type = TOP_IO_PERC;
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
			obj->data.top.num = n - 1;
		}
	} else {
		NORM_ERR("invalid argument count for top");
		return 0;
	}
	return 1;
}


