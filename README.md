## mysh - my shell for Operating Systems course

### Build 
```
make all
```

### Run
```
./mysh
```

### Commands:
 * `movetodir <dir>` - change directory
    * ex: `movetodir /home/mykola/projects/OS/mysh`
    * ex: `movetodir ../../projects/OS/homework/`
 * `history [-c]` - command history. `-c` to clear
    * ex: `history -c`
 * `run program [parameters]` - run a program in foreground
    * ex: `run /usr/bin/xterm -bg green`
    * ex: `run ls -l`
 * `background program [parameters]` - run a program in background
    * ex: `background /usr/bin/xterm -bg green`
 * `repeat n command` - repeat specified command n times
    * ex: `repeat 5 /usr/bin/xterm -ng red`
 * `exterminate <PID>` - kill process with PID
    * ex: `exterminate 16010`
 * `exterminateall` - kill all background processes
 * `whereami` - print current working directory
 * `byebye` - terminate the shell
 
 ### Information
 Written as a homework for COP4600. I like the topics covered in this class, 
 and this assignment gave an opportunity to do a good job at something more