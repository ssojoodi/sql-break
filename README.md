# SQL Break

A utility to speed up ingesting MySQL Dumps

<img src="https://sojoodi.com/assets/SQL-break.png" alt="SQL Break Logo" width="200px" height="200px">

## Overview

SQL Break is a command-line utility written in C designed to split large MySQL dump files into smaller, more manageable files. It achieves this by identifying specific points in a large database dump file where a new table structure is defined. This allows for an easier and faster ingest of the data into a new DB via parallelism and selective updating of tables.

## Usage

The goal is to able to run this utility on a SQL dump as soon as the dump is created.

```sh
mysqldump -u dbusr -p dbname > dump.sql
./sql-break dump.sql
```

## Limitations

SQL Break is designed to work with MySQL dump files and may not function correctly with other types of SQL files. Additionally, it relies on the specific phrase "Table structure for table" to identify points for splitting the file. If this phrase is not present in the dump file, SQL Break will most likely break!

### Compatibility

SQL Break is designed to be compatible with MySQL dump files. Furthermore, the source code is developoed and compilied on MacOS however, it should work on any other UNIX-like and Windows system also.

## License

"SQL Break" is released under the MIT License.
