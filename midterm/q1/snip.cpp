#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int main() {

  // Opens (or creates) log.txt for writing only. O_TRUNC wipes any existing
  // content. Returns a file descriptor integer fd.
  int fd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

  // If open() failed it returns -1. perror prints the system error message,
  // then the program exits.
  if (fd < 0) { perror("open"); return 1; }

  // Writes the 19-byte string "Header Information\n" to the file.
  // The file offset is now at byte 19.
  char header[] = "Header Information\n";
  write(fd, header, strlen(header));

  // Moves the file offset to byte 20 from the start (SEEK_SET). Since only
  // 19 bytes were written, this creates a 1-byte gap (a "hole") filled with
  // a null byte \0.
  if (lseek(fd, 20, SEEK_SET) == -1) { perror("lseek"); return 1; }

  // Writes "Body Content\n" starting at offset 20, right after the gap.
  char body[] = "Body Content\n";
  write(fd, body, strlen(body));

  // Flushes and releases the file descriptor back to the kernel.
  close(fd);

  return 0;

}
