/*
 * Loader Implementation
 *
 * 2018, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "exec_parser.h"

static so_exec_t *exec;
static struct sigaction old_action;
int fd;

void *zeroing_len(void *addr, int page_size, void *start_zero_zone, void *end_zero_zone)
{
	/* se calculeaza lungimea zonei ce trebuie zeroizata in functie si de adresa paginii */
	if (addr < start_zero_zone && addr + page_size >= start_zero_zone)
		/* prima parte a adresei paginii se afla si in spatiul din fisier */

		return (void *)(addr + page_size - start_zero_zone);

	else if (addr >= start_zero_zone && (addr + page_size < end_zero_zone
						|| addr < end_zero_zone))
		/* adresa paginii se afla in interiorul zonei de zeroizat */

		return (void *)page_size;

	return NULL;
}

static void usr2_handler(int signum, siginfo_t *info, void *context)
{
	int page_idx, page_size, page_fault_addr, start_segm, end_segm;
	void *start_addr;
	char *mmap_addr;
	so_seg_t segment;

	/* se ia dimensiunea unei pagini */
	page_size = getpagesize();

	/* se ia adresa unde are loc page fault-ul */
	page_fault_addr = (int)info->si_addr;

	for (int i = 0; i < exec->segments_no; i++) {
		segment = exec->segments[i];

		/* iau adresa de inceput a segmentului */
		start_segm = exec->segments[i].vaddr;

		/* se calucleaza adresa la care se termina segmentul curent */
		end_segm = exec->segments[i].mem_size + exec->segments[i].vaddr;

		if (page_fault_addr >= start_segm && page_fault_addr <= end_segm) {
			/* page fault-ul a avut loc in acest segment */

			/* se caluculeaza index-ul paginii */
			page_idx = (page_fault_addr - start_segm) / page_size;

			if (((int *)exec->segments[i].data)[page_idx] == 1) {
				/* pagina a mai fost mapata in memorie */

				old_action.sa_sigaction(signum, info, context);
				return;
			}

			/* se marcheaza pagina ca mapata */
			((int *)exec->segments[i].data)[page_idx] = 1;

			int page = page_idx * page_size;

			/* se calculeaza adresa de start a paginii de memorie */
			start_addr = (void *) segment.vaddr + page;

			/*se face maparea */
			mmap_addr = mmap(start_addr,
							page_size,
							segment.perm,
							MAP_PRIVATE | MAP_FIXED,
							fd,
							segment.offset + page);

			if (mmap_addr == MAP_FAILED) {
				old_action.sa_sigaction(signum, info, context);
				return;
			}

			if (segment.mem_size > segment.file_size) {
				/* segmentul are o zona ce trebuie zeroizata */

				/* se calculeaza adresa de inceput si de final a zonei ce trebuie zeroizata */
				void *start_zero_zone = (void *)segment.vaddr + segment.file_size;
				void *end_zero_zone = (void *)segment.vaddr + segment.mem_size;

				/* se ia lungimea zonei ce trebuie zeroizata */
				int len = (int)zeroing_len(
								(void *)mmap_addr, page_size, start_zero_zone, end_zero_zone);

				/* se realizeaza zeroizarea zonei */
				memset(start_zero_zone, 0, (int)len);
			}

			return;

		}
	}

	/* adresa nu se gaseste in una din segmente */
	old_action.sa_sigaction(signum, info, context);
}

int so_init_loader(void)
{
	struct sigaction sa;
	int ret;

	// se configureaza handler-ul
	memset(&sa, 0, sizeof(sa));

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = usr2_handler;

	ret = sigaction(SIGSEGV, &sa, &old_action);
	if (ret < 0)
		exit(EXIT_FAILURE);

	return 0;
}

int so_execute(char *path, char *argv[])
{
	/* se deschide fisierul executabil si se iau datele de acolo */
	fd = open(path, O_RDONLY);

	if (fd < 0)
		return -1;

	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	int page_size, num_of_pages;

	page_size = getpagesize();

	for (int i = 0; i < exec->segments_no; i++) {
		so_seg_t *segm = &exec->segments[i];

		/* se calculeaza numarul de pagini din segmentul curent */
		num_of_pages = segm->mem_size / page_size + 1;

		/* se aloca memorie campului de data, folosit pentru a monitoriza paginile mapate */
		exec->segments[i].data = calloc(num_of_pages, sizeof(int));

		if (exec->segments[i].data == NULL)
			return -1;
	}

	so_start_exec(exec, argv);

	close(fd);

	return 0;
}
