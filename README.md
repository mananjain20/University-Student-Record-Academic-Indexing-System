# University Student Record Academic Indexing System

**Author:** Manan Jain  
**Roll No.:** 150096725166

A C++ command-line application for managing student records and comparing AVL
tree and B+ tree indexing. Records are indexed by student ID, and the selected
tree can be switched while the application is running.

## Features

- Add, delete, and search for student records by ID
- Search for records within an ID range
- Display records sorted by ID
- Switch between AVL tree and B+ tree indexes
- List students with a CGPA above 9.0
- Benchmark insert, search, range-search, and delete performance for both trees
- Load and save records using `students.csv`
- Write benchmark measurements to `results.csv`

## Requirements

- A C++ compiler with C++17 support

## Build and run

From the project directory:

```sh
g++ -std=c++17 -O2 -o student_records main.cpp
./student_records
```

The program reads `students.csv` from the current working directory. If no
records are loaded, it starts with the sample records in the source and saves
them to `students.csv`. Changes are saved as records are added or deleted and
when the program exits.

Choose **7. Run benchmark** from the menu to run the AVL/B+ tree comparison.
The benchmark overwrites `results.csv` in the current working directory.

## CSV format

`students.csv` uses this header and column order:

```csv
id,name,dept,semester,cgpa,city
```

Student IDs must be positive and unique. Semester values must be from 1 to 8,
and CGPA values must be from 0 to 10.
