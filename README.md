# Falsetto - A systems level lisp [WIP]
A strongly-typed Lisp that compiles down to C

## Why?
I decided to start working on this project as I love the simplicity and low level nature of C, however the lack of powerful macros and the at times confusing syntax often gets in the way of productivity.

### NOTE THAT THIS IS NOT USABLE YET!!
It has yet to even come close to the feature set that would be required to implement even the most trivial of applications.

## Building
You will need Meson, Ninja and either GCC or Clang. To install these on Alpine Linux:
```sh
apk add meson gcc # replace with clang if you prefer
```

## Goals
	At the moment, the only goal is to become self hosting, and everything added to the project is workign towards that goal.
