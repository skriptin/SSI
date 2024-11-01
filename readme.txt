ssi.c implmentations:

Basic functionality:
Contains a readme 
copiles without errors
is named ssi
prompt correctly dispalying username@hostname: /home/user > FORMAT

Foreground  execution:
Fully implemented should not fault even with bad IMPUT
Ctrl-C will not stop the program
Prints correct error nessage when dispalying error from bad imput

Changing Directoires:
fully implemented

Background Execution:
Programs sucessfuly execute in the Background
While programs are BG executing imput can still be put into the Foreground
bglist works
when a bg process finishes the terminating message is correctly printed
bglist is always accurate

PROBLEMS:
Sometimes strange characters are printed when a background task is finished executing
theese characters are printed as the arguments of a backgrond task, example:
liamcoady1@linux203: /home/liamcoady1/project1 >bg ping google.com -n 5
liamcoady1@linux203: /home/liamcoady1/project1 >bg cat foo.txt
liamcoady1@linux203: /home/liamcoady1/project1 >bg pwd
2271587: cat foo.txt �YU  has terminated.
liamcoady1@linux203: /home/liamcoady1/project1 >bg pwd
2271588: pwd  �YU  has terminated.
This isuue occurs when you improperly format the arguments to a program that you are trying to background execute.



