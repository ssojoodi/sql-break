#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_LENGTH (100)
#define SEARCH_STRING "-- Table structure for table"
#define SEARCH_STRING_LENGTH (strlen(SEARCH_STRING))

// Disable and Re-instate the internal Constraint enforcement of SQL.
#define SQL_FILE_START "/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;\n/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;\n\n"

#define SQL_FILE_END "\n\n/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;\n/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;\n"

/** Open a new SQL file and write the SQL_FILE_START to the file. */
FILE *start_new_sql_file(char *filename, int number)
{
    FILE *split_file = fopen(filename, "wb");
    if (split_file == NULL)
    {
        perror("Error creating split file");
        return NULL;
    }
    fprintf(split_file, "%s", SQL_FILE_START);
    return split_file;
}

/** Close the SQL file and write the SQL_FILE_END to the file. */
void finish_sql_file(FILE *file)
{
    fprintf(file, "%s", SQL_FILE_END);
    fclose(file);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        printf("This utility is designed to split a large MySQL Dump file into smaller files based on the presence of the phrase 'Table structure for table'.\n");
        printf("Each time this phrase is found, a new file is created starting from that point, allowing for easier management of large SQL files.\n");
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "rb");
    if (file == NULL)
    {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_LENGTH];
    size_t bytes_read;
    int split_file_number = 0;
    FILE *split_file = NULL;
    char search_buffer[SEARCH_STRING_LENGTH * 2] = {0}; // Buffer to hold potential matches across reads

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        // Prepend any leftover from the previous read
        memmove(search_buffer + SEARCH_STRING_LENGTH, buffer, SEARCH_STRING_LENGTH);

        for (size_t i = 0; i < bytes_read; i++)
        {
            // Shift the search buffer
            memmove(search_buffer, search_buffer + 1, SEARCH_STRING_LENGTH * 2 - 1);
            search_buffer[SEARCH_STRING_LENGTH * 2 - 1] = buffer[i];

            if (memcmp(search_buffer + SEARCH_STRING_LENGTH, SEARCH_STRING, SEARCH_STRING_LENGTH) == 0)
            {
                if (split_file != NULL)
                {
                    fwrite(buffer, 1, i, split_file);
                    finish_sql_file(split_file);
                }

                char split_file_name[255];
                snprintf(split_file_name, sizeof(split_file_name), "%s_%02d.sql", argv[1], split_file_number++);

                split_file = start_new_sql_file(split_file_name, split_file_number);
                if (split_file == NULL)
                {
                    perror("Error creating split file");
                    fclose(file);
                    return EXIT_FAILURE;
                }

                // Write the SEARCH_STRING to the new file
                fwrite(SEARCH_STRING, 1, SEARCH_STRING_LENGTH, split_file);
            }
        }

        if (split_file != NULL)
        {
            fwrite(buffer, 1, bytes_read, split_file);
        }
    }

    if (split_file != NULL)
    {
        finish_sql_file(split_file);
    }
    fclose(file);
    return EXIT_SUCCESS;
}