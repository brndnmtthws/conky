[NAME]
Conky
[DESCRIPTION]
.\" Add any additional description here
[EXAMPLES]
.B
conky -t '${time %D %H:%m}' -o -u 30
.PP
Start Conky in its own window with date and clock as text and 30 sec update interval.

.B
conky -a top_left -x 5 -y 500 -d
.PP
Start Conky to background at coordinates (5, 500).
[FILES]
~/.conkyrc default configuration file
