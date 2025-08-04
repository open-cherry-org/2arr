#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define BUFFER_SIZE 4096

void pack_file(FILE *archive, const char *path);
void unpack_archive(const char *archive_name);
void list_archive(const char *archive_name);
void show_help();
int is_directory(const char *path);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        show_help();
        return 1;
    }

    if (strcmp(argv[1], "-p") == 0 && argc >= 4) {
        FILE *archive = fopen(argv[2], "wb");
        if (!archive) {
            perror("Error creating archive");
            return 1;
        }
        for (int i = 3; i < argc; i++) {
            pack_file(archive, argv[i]);
        }
        fclose(archive);
        printf("./%s\n", argv[2]);
    } else if (strcmp(argv[1], "-u") == 0) {
        unpack_archive(argv[2]);
    } else if (strcmp(argv[1], "-l") == 0) {
        list_archive(argv[2]);
    } else if (strcmp(argv[1], "-h") == 0) {
        show_help();
    } else {
        show_help();
        return 1;
    }

    return 0;
}

void pack_file(FILE *archive, const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        perror("Error getting file info");
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        fprintf(archive, "D%s\n", path);
        DIR *dir = opendir(path);
        if (!dir) {
            perror("Error opening directory");
            return;
        }
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            char new_path[1024];
            snprintf(new_path, sizeof(new_path), "%s/%s", path, entry->d_name);
            pack_file(archive, new_path);
        }
        closedir(dir);
    } else {
        FILE *file = fopen(path, "rb");
        if (!file) {
            perror("Error opening file");
            return;
        }
        fprintf(archive, "F%s\n%ld\n", path, st.st_size);
        char buffer[BUFFER_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            fwrite(buffer, 1, bytes_read, archive);
        }
        fclose(file);
    }
}

void unpack_archive(const char *archive_name) {
    FILE *archive = fopen(archive_name, "rb");
    if (!archive) {
        perror("Error opening archive");
        return;
    }

    char line[1024];
    printf("Unpacking %s...\n", archive_name);
    
    char output_dir[1024];
    snprintf(output_dir, sizeof(output_dir), "%.*s", (int)(strrchr(archive_name, '.') - archive_name), archive_name);
    mkdir(output_dir, 0755);

    while (fgets(line, sizeof(line), archive)) {
        line[strcspn(line, "\n")] = 0;
        if (line[0] == 'D') {
            char dirpath[1024];
            snprintf(dirpath, sizeof(dirpath), "%s/%s", output_dir, line + 1);
            mkdir(dirpath, 0755);
        } else if (line[0] == 'F') {
            char filepath[1024];
            snprintf(filepath, sizeof(filepath), "%s/%s", output_dir, line + 1);
            
            fgets(line, sizeof(line), archive);
            long filesize = atol(line);
            
            FILE *file = fopen(filepath, "wb");
            if (!file) {
                perror("Error creating file");
                continue;
            }
            
            char buffer[BUFFER_SIZE];
            long remaining = filesize;
            while (remaining > 0) {
                size_t to_read = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;
                size_t bytes_read = fread(buffer, 1, to_read, archive);
                fwrite(buffer, 1, bytes_read, file);
                remaining -= bytes_read;
            }
            fclose(file);
        }
    }
    fclose(archive);
    printf("Unpacked to %s/ successfully.\n", output_dir);
}

void list_archive(const char *archive_name) {
    FILE *archive = fopen(archive_name, "rb");
    if (!archive) {
        perror("Error opening archive");
        return;
    }

    printf("LIST OF %s\n", archive_name);
    char line[1024];
    while (fgets(line, sizeof(line), archive)) {
        line[strcspn(line, "\n")] = 0;
        if (line[0] == 'D') {
            printf("  %s/\n", line + 1);
        } else if (line[0] == 'F') {
            printf("  %s\n", line + 1);
            fgets(line, sizeof(line), archive);
        }
    }
    fclose(archive);
    printf("==\n");
}

void show_help() {
    printf("Usage:\n");
    printf("  2arr -l archive-name.2arr\n");
    printf("  2arr -u archive-name.2arr\n");
    printf("  2arr -p archive-name.2arr file1.txt file2.c file3.sh folder/\n");
    printf("Options:\n");
    printf("  -p - pack to archive\n");
    printf("  -u - unpack\n");
    printf("  -l - list of files and dirs in archive\n");
    printf("  -h - help for usage\n");
}

int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}
