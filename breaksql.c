#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_LENGTH (100)
#define SEARCH_STRING "Table structure for table"
#define SEARCH_STRING_LENGTH (strlen(SEARCH_STRING))

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        printf("This utility is designed to split a large MySQL Dump file into smaller files based on the presence of the phrase 'Table structure for table'.\n");
        printf("Each time this phrase is found, a new file is created starting from that point, allowing for easier management of large SQL files.\n");
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_LENGTH];
    size_t line_number = 1;
    size_t buffer_size = 0;
    size_t split_file_start_index = 0;
    int split_file_number = 0;
    FILE *split_file = NULL;

    while ((buffer_size = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < buffer_size; i++) {
            if (buffer[i] == '\n') {
                line_number++;
            }
            if (i >= SEARCH_STRING_LENGTH && memcmp(buffer + i - SEARCH_STRING_LENGTH, SEARCH_STRING, SEARCH_STRING_LENGTH) == 0) {
                if (split_file != NULL) {
                    fwrite(buffer + split_file_start_index, 1, i - split_file_start_index, split_file);
                    fclose(split_file);
                }
                char split_file_name[255];
                snprintf(split_file_name, sizeof(split_file_name), "%s_%d.sql", argv[1], split_file_number++);
                printf("Creating split file: %s for line number starting at %zu\n", split_file_name, line_number);
                split_file = fopen(split_file_name, "wb");
                if (split_file == NULL) {
                    perror("Error creating split file");
                    fclose(file);
                    return EXIT_FAILURE;
                }

                // Start the new file from the "Table structure for table" statement
                split_file_start_index = i - SEARCH_STRING_LENGTH;
            }
        }
        if (split_file != NULL) {
            fwrite(buffer + split_file_start_index, 1, buffer_size - split_file_start_index, split_file);

            // Reset start index for the next buffer read
            split_file_start_index = 0;
        }
    }

    if (split_file != NULL) {
        fclose(split_file);
    }
    fclose(file);
    return EXIT_SUCCESS;
}
