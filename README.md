# kill-all-windows
Kills the possesses owning windows or closes windows

I wrote this program years ago not long after I started learning C and win32 API.

# Why?
More of a problem of the past.

Often if a full screen program such as a game crashed, there was no way to close that application (Alt-f4 was insufficient).

This lead you to either hit your power button and hope the computer shuts down normally or hit your reset button.

Alternatively, if you just have way to many windows open and want to get rid of them quickly

# Compiling
Character Set must be set to Muti-Byte not Unicode

# Options
#### ```--close```
Sends WM_CLOSE to all visible windows
#### ```--kill```
Kills all visible windows
####  ```--test```
Shows what would be killed without actually doing it.

# How To Use
There are many ways to execute this program when your screen is blocked.

Some keyboard/mouse drivers can execute programs with a key bind.

If you create a shortcut on your desktop to this program, you can set a Shortcut key in its properties.
