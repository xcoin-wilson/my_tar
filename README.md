# Welcome to My Tar
***

## Task
The problem is that the my_tar program fails when trying to add or archive files that do not exist, causing error messages,
the challenge is to correctly handle these missing files gracefully while implementing tar-like functionality without using forbidden functions or global variables.

## Description
I solved the problem by checking if each file exists before adding it to the archive, 
printing appropriate error messages when files are missing, and ensuring the program continues running without crashing.
I also carefully managed file operations and adhered to the specified system call restrictions and no global variables.

## Installation
To install the project, simply run make in the project directory to compile the my_tar executable.

## Usage
The program manipulates tar archives by creating, listing, appending, updating, or extracting files based on the given mode (-c, -t, -r, -u, or -x). 
It reads or writes a tar archive file, processes file headers and contents in 512-byte blocks, and handles errors if files don’t exist or cannot be accessed

./my_project argument1 argument2


### The Core Team


<span><i>Made at <a href='https://qwasar.io'>Qwasar SV -- Software Engineering School</a></i></span>
<span><img alt='Qwasar SV -- Software Engineering School's Logo' src='https://storage.googleapis.com/qwasar-public/qwasar-logo_50x50.png' width='20px' /></span>
