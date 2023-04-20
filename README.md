# mgrep

*grep*-like tool with nice colors.

## Usage
```console
./mgrep -f <files> -t <terms> [options]
```

Prints lines from `<files>` containing any of the `<terms>`.
If no files are specified, `stdin` is used instead.

## Options
- `-c`: Case insensitive search
- `-p`: Print file and line number
- `-a`: Line must contain all words (not implemented)
- `-m`: Enable multi-threading      (not implemented)

## Looks like this:
![](screenshot.png)