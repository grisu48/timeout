# timeout
Small tool to execute programs with a given timeout

## Usage

timeout [OPTIONS] TIMEOUT PROGRAM [ARGUMENTS]

Synopsis

OPTIONS:
  -v   --verbose      Turn verbosity on (including runtime)
  -9   --kill         Send SIGKILL on timeout instead of SIGTERM
  -h   --help         Display help message
  
TIMEOUT is given in seconds
PROGRAM is the program that is executed with the given timeout
ARGUMENTS are optional arguments passed to the program

## Compilation

* make && sudo make install

## License

This work is licensed under the MIT license (http://opensource.org/licenses/MIT)

2015, Felix Niederwanger, http://feldspaten.org
