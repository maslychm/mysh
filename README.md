## mysh - my shell for Operating Systems course

#### Build 
```
make
```

#### Run
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
 Written as a homework for COP4600. I spent a little too much time organizing
 the system architecture and planning for extensibility before realized that 
 at this rate the code will be three thousand lines long. So I went for a more
 limited, but stable implementation.
 
 I utilized something that resembles 
 the Command pattern from [This amazing Game design patterns book](https://gameprogrammingpatterns.com/command.html).