/******************************************************************************
***
* UNX511-Lab2
* I declare that this lab is my own work in accordance with Seneca Academic Policy.
* No part of this assignment has been copied manually or electronically from any other source
* (including web sites) or distributed to other students.
*
* Name: David Rivera Student ID: 137648226 Date: 25/01/26
*
*
******************************************************************************
**/
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <ctype.h>
#include <limits.h>

#include <stdio.h>



/* Couple of considerations:
 * OUTPUT 1:
 * - Scan every dir in /proc
 * - Get each /proc/status file, we care about the following fields:
 *   - PID
 *   - State 
 *   - VmSize
 *   - VmRSSS
 *
 *
 * OUTPUT 2:
 *
 *  - Out of the processes gathered in output 1
 *    - Print the top 5 and print its attributes shown above
 *
 */

#define PROC_DIR "/proc"


typedef struct {
  char name[64];
  char state; // see man7 proc
  uint32_t vmSize; // kB
  uint32_t PID;
  uint32_t VmRSSS; // kB
} ps_status_t;

static int parse_status(FILE* status_file, ps_status_t* out) {
  assert(status_file && out);
  memset(out, 0, sizeof(*out));
  out->state = '?';

  char* line_buffer = NULL;
  size_t line_len = 0;
  ssize_t read;

  while ((read = getline(&line_buffer, &line_len, status_file)) != -1) {
    if (!strncmp(line_buffer, "Name:", 5))
      sscanf(line_buffer + 5, "%63s", out->name);
    else if (!strncmp(line_buffer, "Pid:", 4))
      sscanf(line_buffer + 4, "%u", &out->PID);
    else if (!strncmp(line_buffer, "State:", 6))
      sscanf(line_buffer + 6, " %c", &out->state);
    else if (!strncmp(line_buffer, "VmSize:", 7))
      sscanf(line_buffer + 7, "%u", &out->vmSize);
    else if (!strncmp(line_buffer, "VmRSS:", 6))
      sscanf(line_buffer + 6, "%u", &out->VmRSSS);

    if (out->PID && out->state != '?' && out->vmSize && out->VmRSSS && out->name[0])
      break;
  }

  free(line_buffer);
  return out->PID != 0;
}


static int is_pid_dir(const char* name) {
  if (!name || !*name) return 0;
  for (const char* c = name; *c; ++c) if (!isdigit((unsigned char)*c)) return 0;
  return 1;
}

static int cmp_rss_desc(const void* a, const void* b) {
  const ps_status_t* pa = a; const ps_status_t* pb = b;
  if (pb->VmRSSS == pa->VmRSSS) return 0;
  return pb->VmRSSS > pa->VmRSSS ? 1 : -1;
}

int read_proc_ps(){
  DIR *d = opendir(PROC_DIR);
  if(!d){
    perror("Couldn't open /proc");
    return EXIT_FAILURE;
  }

  ps_status_t* list = NULL;
  size_t count = 0, cap = 0;

  struct dirent *de;
  while((de = readdir(d))){
    if(!is_pid_dir(de->d_name)) continue;

    char status_path[PATH_MAX];
    snprintf(status_path, sizeof(status_path), "%s/%s/status", PROC_DIR, de->d_name);

    FILE* f = fopen(status_path, "r");
    if(!f) continue; // EACCES or raced away

    ps_status_t ps;
    if(parse_status(f, &ps)){
      if(count == cap){
        cap = cap ? cap * 2 : 64;
        list = realloc(list, cap * sizeof(ps_status_t));
      }
      list[count++] = ps;
    }
    fclose(f);
  }

  closedir(d);

  // OUTPUT 1:
  puts("PID   S   VmSize(kB)   VmRSS(kB)   Name");
  for(size_t i = 0; i < count; ++i)
    printf("%5u %c %11u %12u   %s\n", list[i].PID, list[i].state, list[i].vmSize, list[i].VmRSSS, list[i].name);

  // OUTPUT 2
  qsort(list, count, sizeof(ps_status_t), cmp_rss_desc);

  puts("\nTop 5 by VmRSS");
  for(size_t i = 0; i < count && i < 5; ++i)
    printf("%s (%u) -> %u kB\n", list[i].name, list[i].PID, list[i].VmRSSS);

  free(list);
  return EXIT_SUCCESS;
}


int main(int argc, char *argv[])
{
  if(read_proc_ps() == EXIT_FAILURE){
    fprintf(stderr, "Error opening /proc\n");
    exit(EXIT_FAILURE);

  }
  

  return 0;
}

