/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005 Adi Zaimi, Dan Piponi <dan@tanelorn.demon.co.uk>,
 *					  Dave Clark <clarkd@skynet.ca>
 * Copyright (c) 2005-2007 Brenden Matthews, Philip Kovacs, et. al.
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
 * $Id$ */

#include "top.h"

static unsigned long g_time = 0;
static unsigned long long previous_total = 0;
static struct process *first_process = 0;

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
}

static struct process *find_process(pid_t pid)
{
	struct process *p = first_process;

	while (p) {
		if (p->pid == pid) {
			return p;
		}
		p = p->next;
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
	process->counted = 1;

	/* process_find_name(process); */

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
	struct information *cur;

	cur = &info;
	char line[BUFFER_LEN] = { 0 }, filename[BUFFER_LEN], procname[BUFFER_LEN];
	int ps;
	unsigned long user_time = 0;
	unsigned long kernel_time = 0;
	int rc;
	char *r, *q;
	char deparenthesised_name[BUFFER_LEN];
	int endl;
	int nice_val;

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
	rc = sscanf(line, "%*s %s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu "
		"%lu %*s %*s %*s %d %*s %*s %*s %d %d", procname, &process->user_time,
		&process->kernel_time, &nice_val, &process->vsize, &process->rss);
	if (rc < 5) {
		return 1;
	}
	/* Remove parentheses from the process name stored in /proc/ under Linux */
	r = procname + 1;
	/* remove any "kdeinit: " */
	if (r == strstr(r, "kdeinit")) {
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

		q = deparenthesised_name;
		/* stop at space */
		while (*r && *r != ' ') {
			*q++ = *r++;
		}
		*q = 0;
	} else {
		q = deparenthesised_name;
		while (*r && *r != ')') {
			*q++ = *r++;
		}
		*q = 0;
	}

	if (process->name) {
		free(process->name);
	}
	process->name = strdup(deparenthesised_name);
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

/******************************************
 * Get process structure for process pid  *
 ******************************************/

/* This function seems to hog all of the CPU time.
 * I can't figure out why - it doesn't do much. */
static int calculate_cpu(struct process *process)
{
	int rc;

	/* compute each process cpu usage by reading /proc/<proc#>/stat */
	rc = process_parse_stat(process);
	if (rc)
		return 1;
	/* rc = process_parse_statm(process); if (rc) return 1; */

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
			calculate_cpu(p);
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
	unsigned long long nice = 0;
	unsigned long long system = 0;
	unsigned long long idle = 0;
	unsigned long long iowait = 0;
	unsigned long long irq = 0;
	unsigned long long softirq = 0;
	unsigned long long steal = 0;
	char *template =
		KFLAG_ISSET(KFLAG_IS_LONGSTAT) ? TMPL_LONGPROC : TMPL_SHORTPROC;

	ps = open("/proc/stat", O_RDONLY);
	rc = read(ps, line, sizeof(line));
	close(ps);
	if (rc < 0) {
		return 0;
	}

	sscanf(line, template, &cpu, &nice, &system, &idle, &iowait, &irq,
		&softirq, &steal);
	total = cpu + nice + system + idle + iowait + irq + softirq + steal;

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

/******************************************
 * Find the top processes				  *
 ******************************************/

/* free a sp_process structure */
void free_sp(struct sorted_process *sp)
{
	free(sp);
}

/* create a new sp_process structure */
struct sorted_process *malloc_sp(struct process *proc)
{
	struct sorted_process *sp;
	sp = malloc(sizeof(struct sorted_process));
	sp->greater = NULL;
	sp->less = NULL;
	sp->proc = proc;
	return sp;
}

/* cpu comparison function for insert_sp_element */
int compare_cpu(struct process *a, struct process *b)
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
int compare_mem(struct process *a, struct process *b)
{
	if (a->totalmem < b->totalmem) {
		return 1;
	} else if (a->totalmem > b->totalmem) {
		return -1;
	} else {
		return 0;
	}
}

/* insert this process into the list in a sorted fashion,
 * or destroy it if it doesn't fit on the list */
int insert_sp_element(struct sorted_process *sp_cur,
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
void sp_acopy(struct sorted_process *sp_head, struct process **ar, int max_size)
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

inline void process_find_top(struct process **cpu, struct process **mem)
{
	struct sorted_process *spc_head = NULL, *spc_tail = NULL, *spc_cur = NULL;
	struct sorted_process *spm_head = NULL, *spm_tail = NULL, *spm_cur = NULL;
	struct process *cur_proc = NULL;
	unsigned long long total = 0;

	if (!top_cpu && !top_mem) {
		return;
	}

	total = calc_cpu_total();	/* calculate the total of the processor */
	update_process_table();		/* update the table with process list */
	calc_cpu_each(total);		/* and then the percentage for each task */
	process_cleanup();			/* cleanup list from exited processes */

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
		cur_proc = cur_proc->next;
	}
	sp_acopy(spc_head, cpu, MAX_SP);
	sp_acopy(spm_head, mem, MAX_SP);
}
