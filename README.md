# ush
a universall shell with these features:
- an easy syntax to write shell scripts.
- works on both linux and windows.
- [ELN](https://github.com/leo-arch/clifm/wiki/Common-Operations#elns) inspired by clifm

# Build
## Linux
`./p`

## Build dependencies
- git
- a c++ compiler (clang, gcc or msvc)

# TODO
- [x] ignore white spaces and don't put them into chars and args array.
- [x] Ctrl+Right -> go to next word.
- [x] Ctrl+Left -> go to previous word.
- [x] SPACE -> to move forwards all characters in right.

- [ ] put command history in: $HOME/.config/ush/history/commands
  - [ ] UP -> go to next command in history.
  - [ ] Down -> go to previous command in history
  - [ ] Ctrl+r -> search recursively in history.

- [ ] put directory history in: $HOME/.config/ush/history/dirs
  - [ ] Ctrl+j -> Go to next directory in history
  - [ ] Ctrl+k -> Go to previous directory in history

- [ ] implement [ELN](https://github.com/leo-arch/clifm/wiki/Common-Operations#elns) inspired by clifm.

- [ ] Ctrl+Shift+Right -> select next word.
- [ ] Ctrl+Shift+Left -> select previous word.
- [ ] Ctrl+Shift+c -> Copy selected text or current character and put it in clipboard.
- [ ] Ctrl+Shift+v -> Paste from clipboard to current location.

- [ ] press tab offer auto-completion menu.
- [ ] just typipng any character, will show as a hint the aviable command or avaiable history.

- [ ] implement ush language. (which will be use in ush scripts and config file)
  - [ ] implement config file in: $HOME/.ushrc
    - [ ] aiblity to change prompt.
      - [ ] add git status to prompt.

# Sources of inspiration
1. [write-a-shell-in-c](https://brennan.io/2015/01/16/write-a-shell-in-c/)
