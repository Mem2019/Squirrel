#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>


void *afl_custom_init(void *afl, unsigned int seed);
unsigned int afl_custom_fuzz_count(
	void *mutator, const unsigned char *buf, size_t buf_size);
uint8_t afl_custom_queue_new_entry(
	void *mutator, const char *filename_new, const char *filename_orig);

size_t read_file(const char* path, uint8_t** p_buf)
{
	FILE* f = fopen(path, "rb");
	if (f == NULL)
	{
		perror("fopen failed");
		exit(1);
	}
	int r = fseek(f, 0, SEEK_END);
	if (r != 0)
	{
		perror("fseek failed");
		exit(1);
	}
	long size = ftell(f);
	if (size < 0)
	{
		perror("ftell failed");
		exit(1);
	}
	r = fseek(f, 0, SEEK_SET);
	if (r != 0)
	{
		perror("fseek failed");
		exit(1);
	}
	*p_buf = (uint8_t*)malloc(size);
	if (fread(*p_buf, 1, size, f) != size)
	{
		perror("fread failed");
		exit(1);
	}
	fclose(f);
	return size;
}

int main(int argc, char const *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: ./xxx_filter data/fuzz_root/input/\n");
		return 1;
	}
	void* mutator = afl_custom_init(NULL, 0);

	struct dirent *entry; DIR *dir;
	dir = opendir(argv[1]);
	char path[PATH_MAX + 1];
	if (dir == NULL)
	{
		perror("Unable to open directory");
		return 1;
	}
	uint8_t* buf; size_t buf_size;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;
		snprintf(path, sizeof(path), "%s/%s", argv[1], entry->d_name);
		afl_custom_queue_new_entry(mutator, path, path);
		buf_size = read_file(path, &buf);
		if (afl_custom_fuzz_count(mutator, buf, buf_size) == 0)
		{
			if (remove(path) != 0)
			{
				perror("Cannot remove");
				return 1;
			}
		}
		free(buf);
	}
	closedir(dir);
	return 0;
}