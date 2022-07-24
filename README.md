# ptrace-jail

This is simple ptrace jail by using netlink to monitor all the process and check if it is match the jail rules

The usage is simple

```toml
[[rules]]
name="python3"
filename="/etc/passwd"
```

This rules means that this tool will jail all the process that has python3 in the filename to read the `/etc/passwd`


Of course you can define multiple rules at the same time

```toml
[[rules]]
name="python3"
filename="/etc/passwd"

[[rules]]
name="java"
filename="/etc/passwd"
```

Finally, you can run the jail by following the command

```bash
pjail -c $CONFIG_FILE
```

Currently Struts
[img.png](img.png)