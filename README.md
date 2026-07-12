<h4 align="center">
    <img src="https://img.shields.io/github/languages/top/LinArcX/ush.svg"/>  <img src="https://img.shields.io/github/repo-size/LinArcX/ush.svg"/>  <img src="https://img.shields.io/github/tag/LinArcX/ush.svg?colorB=green"/>
</h4>

# Motivations and goals
1. The most important reason that i started this project is to satisfy my `curiosity` and `Eager for a deeper understanding` senses.
2. If you came from UNIX world, working with window ecosystem is a frustrating experience. CMD as a shell is awful. Even i started [winmagics](https://github.com/LinArcX/winmagics) to hack it.
	But very soon i realized that it's better to create my own world on top of windows and live inside it.
3. I always appreciate people that encourage other people to learn deeper and more. So i invite you to hack `ush` and make your own version of it :)
4. Another goal is to use the same shell on both linux and windows. (This is why i named this project: unified shell)

# Build
You can use different build-systems(gnu-make or visual-studio 2005) with different configurations.(x86,64 | debug,release).
But before compiling you need some tools:

## Build dependencies
- git
- clang(gcc or msvc)
- gnu-make(or visual-studio)

# TODO
- [ ] ignore white spaces and don't put them into chars and args array.
- [ ] Ctrl+Right -> go to next word.
- [ ] Ctrl+Left -> go to previous word.
- [ ] Ctrl+Shift+Right -> select next word.
- [ ] Ctrl+Shift+Left -> select previous word.
- [ ] Ctrl+Shift+c -> Copy selected text or current character and put it in clipboard.
- [ ] Ctrl+Shift+v -> Paste from clipboard to current location.
- [ ] implement [ELN](https://github.com/leo-arch/clifm/wiki/Common-Operations#elns) inspired by clifm.
- [ ] put history in: $HOME/.ush_history
- [ ] search through history with Up/Down keys.
- [ ] Ctrl+r -> search recursively in history.
- [ ] press tab offer auto-completion menu.
- [ ] just typipng any character, will show as a hint the aviable command or avaiable history.
- [ ] implement ush language. (which will be use in ush scripts and config file)
  - [ ] implement config file in: $HOME/.ushrc
    - [ ] aiblity to change prompt.
      - [ ] add git status to prompt.

# Sources of inspiration
1. [write-a-shell-in-c](https://brennan.io/2015/01/16/write-a-shell-in-c/)

# License
![License](https://img.shields.io/github/license/LinArcX/ush.svg)
