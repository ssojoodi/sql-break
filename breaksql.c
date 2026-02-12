#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_BUFFER_SIZE (64 * 1024)
#define SEARCH_STRING "-- Table structure for table"
#define SEARCH_STRING_LENGTH (sizeof(SEARCH_STRING) - 1)

/* Disable and re-enable internal SQL constraint enforcement. */
#define SQL_FILE_START \
    "/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;\n" \
    "/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;\n\n"

#define SQL_FILE_END \
    "\n\n/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;\n" \
    "/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;\n"

/** Open a new SQL file and write the SQL_FILE_START header. */
FILE *start_new_sql_file(const char *base_filename, int number)
{
    char split_file_name[512];
    snprintf(split_file_name, sizeof(split_file_name), "%s_%03d.sql", base_filename, number);

    FILE *f = fopen(split_file_name, "w");
    if (f == NULL)
    {
        perror("Error creating split file");
        return NULL;
    }
    fprintf(f, "%s", SQL_FILE_START);
    return f;
}

/** Write the SQL_FILE_END footer and close the file. */
void finish_sql_file(FILE *f)
{
    if (f != NULL)
    {
        fprintf(f, "%s", SQL_FILE_END);
        fclose(f);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        fprintf(stderr, "Splits a large MySQL dump file into smaller files at table boundaries.\n");
        fprintf(stderr, "Each output file contains one table's structure and data.\n");
        return EXIT_FAILURE;
    }

    FILE *input = fopen(argv[1], "r");
    if (input == NULL)
    {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    char *line = malloc(LINE_BUFFER_SIZE);
    if (line == NULL)
    {
        perror("Error allocating line buffer");
        fclose(input);
        return EXIT_FAILURE;
    }

    int split_number = 0;
    FILE *output = NULL;

    while (fgets(line, LINE_BUFFER_SIZE, input) != NULL)
    {
        /* Start a new output file when a table structure marker is found.
           Using strncmp ensures we only match at the start of a line,
           avoiding false positives from data containing the marker text. */
        if (strncmp(line, SEARCH_STRING, SEARCH_STRING_LENGTH) == 0)
        {
            finish_sql_file(output);
            output = start_new_sql_file(argv[1], split_number++);
            if (output == NULL)
            {
                fclose(input);
                free(line);
                return EXIT_FAILURE;
            }
        }

        /* Create an initial output file for preamble content (server
           settings, character sets, etc.) that appears before the first
           table marker. This prevents silent data loss. */
        if (output == NULL)
        {
            output = start_new_sql_file(argv[1], split_number++);
            if (output == NULL)
            {
                fclose(input);
                free(line);
                return EXIT_FAILURE;
            }
        }

        fputs(line, output);
    }

    if (ferror(input))
    {
        perror("Error reading input file");
        finish_sql_file(output);
        fclose(input);
        free(line);
        return EXIT_FAILURE;
    }

    finish_sql_file(output);
    fclose(input);
    free(line);
    return EXIT_SUCCESS;
}
