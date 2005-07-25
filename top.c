#include "top.h"

static regex_t *exclusion_expression = 0;
static unsigned int g_time = 0;
static int previous_total = 0;
static struct process *first_process = 0;

static struct process *find_process(pid_t pid)
{
	struct process *p = first_process;
	while (p) {
		if (p->pid == pid)
			return p;
		p = p->next;
	}
	return 0;
}

/*
* Create a new process object and insert it into the process list
*/
static struct process *new_process(int p)
{
	struct process *process;
	process = malloc(sizeof(struct process));

	/*
	* Do stitching necessary for doubly linked list
	*/
	process->name = 0;
	process->previous = 0;
	process->next = first_process;
	if (process->next)
		process->next->previous = process;
	first_process = process;

	process->pid = p;
	process->time_stamp = 0;
	process->previous_user_time = INT_MAX;
	process->previous_kernel_time = INT_MAX;
	process->counted = 1;

	/*    process_find_name(process);*/

	return process;
}

/******************************************/
/* Functions                              */
/******************************************/

static int process_parse_stat(struct process *);
static int update_process_table(void);
static int calculate_cpu(struct process *);
static void process_cleanup(void);
static void delete_process(struct process *);
/*inline void draw_processes(void);*/
static int calc_cpu_total(void);
static void calc_cpu_each(int);


/******************************************/
/* Extract information from /proc         */
/******************************************/

/*
* These are the guts that extract information out of /proc.
* Anyone hoping to port wmtop should look here first.
*/
static int process_parse_stat(struct process *process)
{
	struct information *cur;
	cur = &info;
	char line[BUFFER_LEN], filename[BUFFER_LEN], procname[BUFFER_LEN];
	int ps;
	int user_time, kernel_time;
	int rc;
	char *r, *q;
	char deparenthesised_name[BUFFER_LEN];
	int endl;
	int nice_val;

	snprintf(filename, sizeof(filename), PROCFS_TEMPLATE,
		 process->pid);

	ps = open(filename, O_RDONLY);
	if (ps < 0)
		/*
		* The process must have finished in the last few jiffies!
		*/
		return 1;

	/*
	* Mark process as up-to-date.
	*/
	process->time_stamp = g_time;

	rc = read(ps, line, sizeof(line));
	close(ps);
	if (rc < 0)
		return 1;

	/*
	* Extract cpu times from data in /proc filesystem
	*/
	rc = sscanf(line,
		    "%*s %s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %d %d %*s %*s %*s %d %*s %*s %*s %d %d",
		    procname, &process->user_time, &process->kernel_time,
		    &nice_val, &process->vsize, &process->rss);
	if (rc < 5)
		return 1;
	/*
	* Remove parentheses from the process name stored in /proc/ under Linux...
	*/
	r = procname + 1;
	/* remove any "kdeinit: " */
	if (r == strstr(r, "kdeinit")) {
		snprintf(filename, sizeof(filename),
			 PROCFS_CMDLINE_TEMPLATE, process->pid);

		ps = open(filename, O_RDONLY);
		if (ps < 0)
			/*
			* The process must have finished in the last few jiffies!
			*/
			return 1;

		endl = read(ps, line, sizeof(line));
		close(ps);

		/* null terminate the input */
		line[endl] = 0;
		/* account for "kdeinit: " */
		if ((char *) line == strstr(line, "kdeinit: "))
			r = ((char *) line) + 9;
		else
			r = (char *) line;

		q = deparenthesised_name;
		/* stop at space */
		while (*r && *r != ' ')
			*q++ = *r++;
		*q = 0;
	} else {
		q = deparenthesised_name;
		while (*r && *r != ')')
			*q++ = *r++;
		*q = 0;
	}
	
	if (process->name)
		free(process->name);
	process->name = strdup(deparenthesised_name);
	process->rss *= getpagesize();
	
	if(!cur->memmax)
		update_total_processes();
	
	process->totalmem = ( (float) process->rss / cur->memmax ) / 10;

	if (process->previous_user_time == INT_MAX)
		process->previous_user_time = process->user_time;
	if (process->previous_kernel_time == INT_MAX)
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

/******************************************/
/* Update process table                   */
/******************************************/

static int update_process_table()
{
	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir("/proc")))
		return 1;

	++g_time;

	/*
	* Get list of processes from /proc directory
	*/
	while ((entry = readdir(dir))) {
		pid_t pid;

		if (!entry) {
			/*
			* Problem reading list of processes
			*/
			closedir(dir);
			return 1;
		}

		if (sscanf(entry->d_name, "%d", &pid) > 0) {
			struct process *p;
			p = find_process(pid);
			if (!p)
				p = new_process(pid);

			/* compute each process cpu usage */
			calculate_cpu(p);
		}
	}

	closedir(dir);

	return 0;
}

/******************************************/
/* Get process structure for process pid  */
/******************************************/

/*
* This function seems to hog all of the CPU time. I can't figure out why - it
* doesn't do much.
*/
static int calculate_cpu(struct process *process)
{
	int rc;

	/* compute each process cpu usage by reading /proc/<proc#>/stat */
	rc = process_parse_stat(process);
	if (rc)
		return 1;
	/*rc = process_parse_statm(process);
	if (rc)
	return 1;*/

	/*
	* Check name against the exclusion list
	*/
	if (process->counted && exclusion_expression
		   && !regexec(exclusion_expression, process->name, 0, 0, 0))
		process->counted = 0;

	return 0;
}

/******************************************/
/* Strip dead process entries             */
/******************************************/

static void process_cleanup()
{

	struct process *p = first_process;
	while (p) {
		struct process *current = p;

#if defined(PARANOID)
		assert(p->id == 0x0badfeed);
#endif				/* defined(PARANOID) */

		p = p->next;
		/*
		* Delete processes that have died
		*/
		if (current->time_stamp != g_time)
			delete_process(current);
	}
}

/******************************************/
/* Destroy and remove a process           */
/******************************************/

static void delete_process(struct process *p)
{
#if defined(PARANOID)
	assert(p->id == 0x0badfeed);

	/*
	* Ensure that deleted processes aren't reused.
	*/
	p->id = 0x007babe;
#endif				/* defined(PARANOID) */

	/*
	* Maintain doubly linked list.
	*/
	if (p->next)
		p->next->previous = p->previous;
	if (p->previous)
		p->previous->next = p->next;
	else
		first_process = p->next;

	if (p->name)
		free(p->name);
	free(p);
}

/******************************************/
/* Calculate cpu total                    */
/******************************************/

static int calc_cpu_total()
{
	int total, t;
	int rc;
	int ps;
	char line[BUFFER_LEN];
	int cpu, nice, system, idle;

	ps = open("/proc/stat", O_RDONLY);
	rc = read(ps, line, sizeof(line));
	close(ps);
	if (rc < 0)
		return 0;
	sscanf(line, "%*s %d %d %d %d", &cpu, &nice, &system, &idle);
	total = cpu + nice + system + idle;

	t = total - previous_total;
	previous_total = total;

	if (t < 0)
		t = 0;

	return t;
}

/******************************************/
/* Calculate each processes cpu           */
/******************************************/

inline static void calc_cpu_each(int total)
{
	struct process *p = first_process;
	while (p) {
		/*p->amount = total ?
		(100.0 * (float) (p->user_time + p->kernel_time) /
		total) : 0;*/
		p->amount = (100.0 * (p->user_time + p->kernel_time) / total);

/*		if (p->amount > 100)
		p->amount = 0;*/
		p = p->next;
	}
}

/******************************************/
/* Find the top processes                 */
/******************************************/

/*
* Result is stored in decreasing order in best[0-9].
*/

static struct process **sorttmp;
static size_t sorttmp_size = 10;

inline void process_find_top(struct process **cpu, struct process **mem)
{
	struct process *pr;
	if (sorttmp == NULL) {
		sorttmp = malloc(sizeof(struct process)*sorttmp_size);
		assert(sorttmp != NULL);
	}
	int total;
	unsigned int i, max;

	total = calc_cpu_total();	/* calculate the total of the processor */

	update_process_table();	/* update the table with process list */
	calc_cpu_each(total);	/* and then the percentage for each task */
	process_cleanup();	/* cleanup list from exited processes */

	/*
	* this is really ugly,
	* not to mention probably not too efficient.
	* the main problem is that there could be any number of processes,
	* however we have to use a fixed size for the "best" array.
	* right now i can't think of a better way to do this,
	* although i'm sure there is one.
	* Perhaps just using a linked list would be more effecient?
	* I'm too fucking lazy to do that right now.
	*/
	if(top_cpu) {
		pr = first_process;
		i = 0;
		while(pr) {
			if(i<sorttmp_size && pr->counted) {
				sorttmp[i] = pr;
				i++;
			}
			else if (i == sorttmp_size && pr->counted) {
				sorttmp_size++;
				sorttmp = realloc(sorttmp, sizeof(struct process)*sorttmp_size);
				sorttmp[i] = pr;
				i++;
			}
			pr = pr->next;
		}
		if (i+1 < sorttmp_size) {
			sorttmp = realloc(sorttmp, sizeof(struct process)*sorttmp_size);
		}
		max = i;
		for(i=0;i<max-1;i++)
		{
			while (sorttmp[i+1]->amount > sorttmp[i]->amount)
			{
				pr = sorttmp[i];
				sorttmp[i] = sorttmp[i+1];
				sorttmp[i+1] = pr;
				if (i>0)
					i--;
				else
					break;
			}

		}
		for(i=max;i>1;i--);
		{
			while (sorttmp[i]->amount > sorttmp[i-1]->amount)
			{
				pr = sorttmp[i];
				sorttmp[i] = sorttmp[i-1];
				sorttmp[i-1] = pr;
				if (i<max)
					i++;
				else
					break;
			}
		}
		for(i=0;i<10;i++)
		{
			cpu[i] = sorttmp[i];

		}
	}
	if (top_mem) {
		pr = first_process;
		i = 0;
		while(pr) {
			if(i<sorttmp_size && pr->counted) {
				sorttmp[i] = pr;
				i++;
			}
			else if (i == sorttmp_size && pr->counted) {
				sorttmp_size++;
				sorttmp = realloc(sorttmp, sizeof(struct process)*sorttmp_size);
				sorttmp[i] = pr;
				i++;
			}
			pr = pr->next;
		}
		if (i+1 < sorttmp_size) {
			sorttmp = realloc(sorttmp, sizeof(struct process)*sorttmp_size);
		}		max = i;
		for(i=0;i<max-1;i++)
		{
			while (sorttmp[i+1]->totalmem > sorttmp[i]->totalmem)
			{
				pr = sorttmp[i];
				sorttmp[i] = sorttmp[i+1];
				sorttmp[i+1] = pr;
				if (i>0)
					i--;
				else
					break;
			}

		}
		for(i=max;i>1;i--);
		{
			while (sorttmp[i]->totalmem > sorttmp[i-1]->totalmem)
			{
				pr = sorttmp[i];
				sorttmp[i] = sorttmp[i-1];
				sorttmp[i-1] = pr;
				if (i<max)
					i++;
				else
					break;
			}
		}
		for(i=0;i<10;i++)
		{
			mem[i] = sorttmp[i];

		}
	}
}
