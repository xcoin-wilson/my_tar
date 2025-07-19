
/*

-c	Create new archive
-r	Append to archive (-f required)
-t	List archive contents
-u	Update archive if file is newer (-f required)
-x	Extract archive

*/

#define _POSIX_C_SOURCE 201312L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <utime.h>
#include <errno.h>
#include <stdint.h>
#define POSIX_C_SOURCE 200809L
#define BLOCK_SIZE 512
#define NAME_SIZE 100
#define PREFIX_SIZE 155

struct posix_header {
char name[100];
char mode[8];
char uid[8];
char gid[8];
char size[12];
char mtime[12];
char chksum[8];
char typeflag;
char linkname[100];
char magic[6];
char version[2];
char uname[32];
char gname[32];
char devmajor[8];
char devminor[8];
char prefix[155];
char pad[12];
};

//Function 1: print_error
static void print_error(const char *file, const char *msg) {
    dprintf(STDERR_FILENO, "my_tar: %s: %s\n", file, msg);
}

//Function 2: clear_header
static void clear_header(struct posix_header *header) {
    memset(header, 0, sizeof(struct posix_header));
}

//Function 3: octal_to_str
static void octal_to_str(char *str, size_t size, uint64_t val) {
    snprintf(str, size, "%0*lo", (int)size - 1, (unsigned long)val);
}

//Function 4: calculate_checksum
static unsigned int calculate_checksum(struct posix_header *header) {
    unsigned char *bytes = (unsigned char *)header;
    unsigned int sum = 0;
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
        if (i >= 148 && i < 156)
            sum += ' '; // checksum field treated as spaces
        else
            sum += bytes[i];
    }
    return sum;
}

//Function 5: write_header
static int write_header(int tar_fd, const char *filename, struct stat *st) {
    struct posix_header header;
    clear_header(&header);

    size_t filename_len = strlen(filename);
    if (filename_len > NAME_SIZE) {
        print_error(filename, "Filename too long");
        return 1;
    }
    strcpy(header.name, filename);
    octal_to_str(header.mode, sizeof(header.mode), (uint64_t)(st->st_mode & 0777));
    octal_to_str(header.uid, sizeof(header.uid), (uint64_t)st->st_uid);
    octal_to_str(header.gid, sizeof(header.gid), (uint64_t)st->st_gid);
    octal_to_str(header.size, sizeof(header.size), (uint64_t)st->st_size);
    octal_to_str(header.mtime, sizeof(header.mtime), (uint64_t)st->st_mtime);
    memset(header.chksum, ' ', sizeof(header.chksum));
    header.typeflag = '0';
    memcpy(header.magic, "ustar", 5);
    header.magic[5] = '\0';
    memcpy(header.version, "00", 2);

    unsigned int chksum = calculate_checksum(&header);
    snprintf(header.chksum, sizeof(header.chksum), "%06o", chksum);
    header.chksum[6] = '\0';
    header.chksum[7] = ' ';

    if (write(tar_fd, &header, sizeof(header)) != sizeof(header)) {
        print_error(filename, "Failed to write header");
        return 1;
    }
    return 0;
}


//Function 6: write_file_data
 static int write_file_data(int tar_fd, int fd, size_t size) {
 char buffer[BLOCK_SIZE];
 ssize_t bytes_read;
 size_t total = 0;

    while (total < size) {
    size_t to_read = BLOCK_SIZE;
    if (size - total < BLOCK_SIZE)
    to_read = size - total;

    bytes_read = read(fd, buffer, to_read);
    if (bytes_read < 0) return 1;
    if (bytes_read == 0) break;

    if (write(tar_fd, buffer, bytes_read) != bytes_read) return 1;
    total += bytes_read;
    }



    if (size % BLOCK_SIZE != 0) {
        size_t padding = BLOCK_SIZE - (size % BLOCK_SIZE);
        memset(buffer, 0, padding);
        if (write(tar_fd, buffer, padding) != (ssize_t)padding) return 1;
    }

    return 0;
}

//Function 7: write_file_to_tar
static int write_file_to_tar(int tar_fd, const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        print_error(filename, "Cannot stat: No such file or directory");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        print_error(filename, "Cannot stat file");
        close(fd);
        return 1;
    }

    if (S_ISDIR(st.st_mode)) {
        close(fd);
        return 0;
    }

    if (write_header(tar_fd, filename, &st) != 0) {
        close(fd);
        return 1;
    }

    if (write_file_data(tar_fd, fd, st.st_size) != 0) {
        print_error(filename, "Failed to write file data");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}

//Function 8: list_tar
static int list_tar(int tar_fd) {
    struct posix_header header;
    ssize_t bytes_read;

    while ((bytes_read = read(tar_fd, &header, sizeof(header))) == sizeof(header)) {
        if (header.name[0] == '\0') break;

        size_t size = strtol(header.size, NULL, 8);
        write(STDOUT_FILENO, header.name, strlen(header.name));
        write(STDOUT_FILENO, "\n", 1);

        off_t skip = ((size + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
        if (lseek(tar_fd, skip, SEEK_CUR) == (off_t)-1) return 1;
    }
    return 0;
}

//Function 9: extract_tar
static int extract_tar(int tar_fd) {
    struct posix_header header;
    ssize_t bytes_read;

    while ((bytes_read = read(tar_fd, &header, sizeof(header))) == sizeof(header)) {
        if (header.name[0] == '\0') break;

        size_t size = strtol(header.size, NULL, 8);

        int out_fd = open(header.name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) {
            print_error(header.name, "Failed to create file");
            off_t skip = ((size + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
            if (lseek(tar_fd, skip, SEEK_CUR) == (off_t)-1) return 1;
            continue;
        }

    char buffer[BLOCK_SIZE];
    size_t remaining = size;
    while (remaining > 0) {
    size_t to_read = remaining > BLOCK_SIZE ? BLOCK_SIZE : remaining;
    ssize_t r = read(tar_fd, buffer, to_read);
    if (r <= 0) break;
    if (write(out_fd, buffer, r) != r) break;
    remaining -= r;
    }
    close(out_fd);

        off_t skip = ((size + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE - size;
        if (skip > 0) {
            if (lseek(tar_fd, skip, SEEK_CUR) == (off_t)-1) return 1;
        }

        time_t mtime = strtol(header.mtime, NULL, 8);
        struct utimbuf times = {mtime, mtime};
        utime(header.name, &times);
    }
    return 0;
}

//Function 10: append_files
static int append_files(const char *tar_filename, int argc, char *argv[], int start_index) {
    int orig_fd = open(tar_filename, O_RDONLY);
    if (orig_fd < 0) {
        print_error(tar_filename, "Cannot open tarball");
        return 1;
    }

    char tmp_filename[256];
    snprintf(tmp_filename, sizeof(tmp_filename), "%s.tmp", tar_filename);

    int tmp_fd = open(tmp_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (tmp_fd < 0) {
        print_error(tmp_filename, "Cannot create temp file");
        close(orig_fd);
        return 1;
    }

    int status = 0;

    // Write new files first
    for (int i = start_index; i < argc; i++) {
        if (write_file_to_tar(tmp_fd, argv[i]) != 0)
            status = 1;
    }

    // Copy old archive entries
    while (1) {
        struct posix_header header;
        ssize_t r = read(orig_fd, &header, sizeof(header));
        if (r == 0) break;
        if (r != sizeof(header)) {
            print_error(tar_filename, "Corrupted archive header");
            status = 1;
            break;
        }

        if (header.name[0] == '\0') break;

        if (write(tmp_fd, &header, sizeof(header)) != sizeof(header)) {
            print_error(tmp_filename, "Failed to write header");
            status = 1;
            break;
        }



        size_t size = strtol(header.size, NULL, 8);
        size_t blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
        ssize_t to_copy = blocks * BLOCK_SIZE;

        char buffer[BLOCK_SIZE];
        while (to_copy > 0) {
            ssize_t chunk = to_copy > (ssize_t)BLOCK_SIZE ? BLOCK_SIZE : to_copy;
            ssize_t rr = read(orig_fd, buffer, chunk);
            if (rr <= 0) {
                print_error(tar_filename, "Unexpected EOF in archive");
                status = 1;
                break;
            }
            if (write(tmp_fd, buffer, rr) != rr) {
                print_error(tmp_filename, "Failed to write data");
                status = 1;
                break;
            }
            to_copy -= rr;
        }

        if (status) break;
    }


    // Write two zero blocks to mark end
    {
     char zero_block[BLOCK_SIZE] = {0};
     write(tmp_fd, zero_block, BLOCK_SIZE);
    write(tmp_fd, zero_block, BLOCK_SIZE);
    }



    close(orig_fd);
    close(tmp_fd);

    if (status) {
        unlink(tmp_filename);
        return 1;
    }

    // Replace original tarball
    if (rename(tmp_filename, tar_filename) < 0) {
        print_error(tar_filename, "Failed to rename temp archive");
        unlink(tmp_filename);
        return 1;
    }

    return 0;
}

//Function 11: main the entry point of the program
int main(int argc, char *argv[]) {
if (argc < 3) {
dprintf(STDERR_FILENO, "Usage: ./my_tar -<mode>f archive.tar [files...]\n");
return 1;
}

if (argv[1][0] != '-' || strlen(argv[1]) < 3 || argv[1][2] != 'f') {
dprintf(STDERR_FILENO, "my_tar: Invalid usage\n");
return 1;
}
    char mode = argv[1][1];
    char *tar_name = argv[2];
    int tar_fd, ret;

                switch (mode) {
                    case 'c':
            tar_fd = open(tar_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (tar_fd < 0) {
                print_error(tar_name, "Cannot open tarball");
                return 1;
            }
            for (int i = 3; i < argc; i++) {
                if (write_file_to_tar(tar_fd, argv[i]) != 0) {
                    close(tar_fd);
                    return 1;
                }
            }
            {
                char zero_block[BLOCK_SIZE] = {0};
                write(tar_fd, zero_block, BLOCK_SIZE);
                write(tar_fd, zero_block, BLOCK_SIZE);
            }
            close(tar_fd);
            break;

        case 't':
            tar_fd = open(tar_name, O_RDONLY);
            if (tar_fd < 0) {
                print_error(tar_name, "Cannot open tarball");
                return 1;
            }
            ret = list_tar(tar_fd);
            close(tar_fd);
            return ret;

        case 'x':
            tar_fd = open(tar_name, O_RDONLY);
            if (tar_fd < 0) {
                print_error(tar_name, "Cannot open tarball");
                return 1;
            }
            ret = extract_tar(tar_fd);
            close(tar_fd);
            return ret;

        case 'r':
            if (argc < 4) {
                dprintf(STDERR_FILENO, "my_tar: No files specified for append\n");
                return 1;
            }
            ret = append_files(tar_name, argc, argv, 3);
            return ret;

        default:
            dprintf(STDERR_FILENO, "my_tar: Unsupported mode '%c'\n", mode);
            return 1;
    }

    return 0;
}

