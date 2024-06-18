#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define STB_INCLUDE_IMPLEMENTATION
#include "stb_include.h"

struct source {
	char* path;
	struct timespec modtime_recursive;
};

static struct source* source_arr;

static char* copy_string(const char* src)
{
	const size_t n = strlen(src)+1;
	char* dst = malloc(n);
	memcpy(dst, src, n);
	return dst;
}

static void get_file_modtime(const char* path, struct timespec* out_modtime)
{
	struct stat st;
	int e = stat(path, &st);
	if (e == -1) {
		fprintf(stderr, "%s: %s\n", path, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (out_modtime) *out_modtime = st.st_mtim;
}

static int timespec_compar(struct timespec a, struct timespec b)
{
	const int d0 = (a.tv_sec > b.tv_sec) - (a.tv_sec < b.tv_sec);
	if (d0 != 0) return d0;
	return (a.tv_nsec > b.tv_nsec) - (a.tv_nsec < b.tv_nsec);
}

static void get_source_modtime_rec(const char* path, struct timespec* out_modtime)
{
	for (int i = 0; i < arrlen(source_arr); i++) {
		struct source* f = &source_arr[i];
		if (strcmp(f->path, path) == 0) {
			*out_modtime = f->modtime_recursive;
			return;
		}
	}

	// inserting early to prevent infinite recursion
	const int si = arrlen(source_arr);
	arrput(source_arr, ((struct source) {
		.path = copy_string(path),
	}));

	FILE* f = fopen(path, "r");
	if (f == NULL) {
		fprintf(stderr, "%s: %s\n", path, strerror(errno));
		exit(EXIT_FAILURE);
	}

	struct stat st;
	if (fstat(fileno(f), &st) == -1) {
		fclose(f);
		fprintf(stderr, "%s: %s\n", path, strerror(errno));
		exit(EXIT_FAILURE);
	}

	struct timespec modtime = st.st_mtim;

	if (fseek(f, 0, SEEK_END) == -1) {
		fclose(f);
		fprintf(stderr, "%s: %s\n", path, strerror(errno));
		exit(EXIT_FAILURE);
	}

	const long sz = ftell(f);

	if (fseek(f, 0, SEEK_SET) == -1) {
		fclose(f);
		fprintf(stderr, "%s: %s\n", path, strerror(errno));
		exit(EXIT_FAILURE);
	}

	char* data = malloc(sz+1);
	if (fread(data, 1, sz, f) != sz) {
		free(data);
		fclose(f);
		fprintf(stderr, "%s: %s\n", path, strerror(errno));
		exit(EXIT_FAILURE);
	}
	data[sz] = 0;
	fclose(f);

	include_info* inc_list = NULL;
	const int num = stb_include_find_includes(data, &inc_list);
	for (int i = 0; i < num; i++) {
		struct timespec other_modtime = {0};
		get_source_modtime_rec(inc_list[i].filename, &other_modtime);
		if (timespec_compar(other_modtime, modtime) > 0) {
			modtime = other_modtime;
		}
	}
	stb_include_free_includes(inc_list, num);

	free(data);

	source_arr[si].modtime_recursive = modtime;

	if (out_modtime) *out_modtime = modtime;
}

static inline const char* get_ext(const char* path)
{
	const size_t n = strlen(path);
	const char* p;
	for (p = (path+n-1); p >= path && *p != '.'; p--) {}
	return *p == '.' ? p : path+n;
}

static inline int has_ext(const char* path, const char* ext)
{
	return strcmp(get_ext(path), ext) == 0;
}

static int base_compar(const char* a, const char* b)
{
	const char* ea = get_ext(a);
	const char* eb = get_ext(b);
	const size_t na = ea-a;
	const size_t nb = eb-b;
	const size_t n = na<nb ? na : nb;
	int d = memcmp(a,b,n);
	if (d != 0) return d;
	if (na == nb) return 0;
	return na-nb;
}

static int str_compar(const void* va, const void* vb)
{
	const char* a = *(const char**)va;
	const char* b = *(const char**)vb;
	const int d = strcmp(a, b);
	return d;
}

static char* prg;

static void usage(void)
{
	fprintf(stderr, "Usage: %s [-x]\n", prg);
	fprintf(stderr, "Without -x it lists object files to delete.\n");
	fprintf(stderr, "With -x it deletes them.\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
	prg = argv[0];
	int do_delete;
	if (argc == 1) {
		do_delete = 0;
	} else if (argc == 2) {
		if (strcmp(argv[1], "-x") == 0) {
			do_delete = 1;
		} else {
			usage();
		}
	} else {
		usage();
	}
	DIR* dir = opendir(".");
	if (dir == NULL) {
		fprintf(stderr, "opendir: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	char** src_arr = NULL;
	char** o_arr = NULL;
	for (;;) {
		struct dirent* ent = readdir(dir);
		if (ent == NULL) break;
		char* name = ent->d_name;
		if (has_ext(name, ".c") || has_ext(name, ".cc") || has_ext(name, ".cpp")) {
			char* nc = copy_string(name);
			arrput(src_arr, nc);
		} else if (has_ext(name, ".o")) {
			char* nc = copy_string(name);
			arrput(o_arr, nc);
		}
	}

	if (closedir(dir) == -1) {
		fprintf(stderr, "closedir: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	qsort(src_arr, arrlen(src_arr), sizeof src_arr[0], str_compar);
	qsort(o_arr,   arrlen(o_arr),   sizeof o_arr[0],   str_compar);

	const int no = arrlen(o_arr);
	const int nsrc = arrlen(src_arr);
	int i1 = 0;
	int n = 0;
	for (int i0 = 0; i0 < no; i0++) {
		const char* o = o_arr[i0];
		for (; i1 < nsrc; i1++) {
			const char* src = src_arr[i1];
			const int d = base_compar(src, o);
			if (d < 0) continue;
			if (d > 0) break;
			struct timespec o_modtime, src_modtime;
			get_file_modtime(o, &o_modtime);
			get_source_modtime_rec(src, &src_modtime);
			if (timespec_compar(src_modtime, o_modtime) > 0) {
				n++;
				if (do_delete) unlink(o);
				printf("%s%s\n", do_delete ? "removed " : "", o);
			}
			break;
		}
	}
	if (n > 0 && !do_delete) printf("Run with `%s -x` to delete these files.\n", argv[0]);

	return EXIT_SUCCESS;
}
