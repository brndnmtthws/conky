**Issue**

Verify the issue you want to report is not a duplicate. If not, please explain
your bug or issue here. [DELETE ME]

**Information**

Do we require your OS and/or Conky version? If not, delete this part. Otherwise,
please report them here. [DELETE ME]

Do we require your config? If no, delete this part. Otherwise, place your
config inside `lua` triple backticks. [DELETE ME]


```lua
conky.config = {
    out_to_x = false,
    out_to_console = true,
};

conky.text = [[
Hello, World!
]];
```

If you want to report a crash, please try again with `gdb`.
```bash
# Start 'conky'
$ gdb conky

# Run 'conky' with a config.
(gdb) run -c ~/.your_conky.conf

# Wait for a crash to occur, then run this.
(gdb) bt full

# Backtrace.
```
We want that backtrace inside `gdb` triple backticks.
```gdb
$ gdb conky
GNU gdb (GDB) 8.2
Copyright (C) 2018 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
...
```

Please delete all descriptions, unused headers, notes, sections, etc too. Thank you.
