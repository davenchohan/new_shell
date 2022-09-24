# Unix Command Interpreter

Created my own Unix Command Interpreter in C on a Linux machine.

## How to Use

Shell recognizes the following lines:
●  $<VAR>=<value>
●  <command> <arg0> <arg1> ... <argN>, where <command> is a name of built-in command.

Has the following custom commands:
● exit, the shell terminates on this Command
● log, the shell prints history of commands
● print, the shell prints argument given to this Command
● theme, the shell changes the color of output

## Script Mode

For script mode, it should start like ./cshell <filename>
The script is a file containing lines of commands