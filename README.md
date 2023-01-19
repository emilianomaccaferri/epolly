# epolly
A very minimal webserver built on top of epoll (WIP)

# inspo
This project is made for the OS Design exam at Unimore and is based on [this one](http://www.0x04.net/doc/posix/Multi-Threaded%20Programming%20with%20POSIX%20Threads%20-%20Linux%20Systems%20Programming.pdf#%5B%7B%22num%22%3A132%2C%22gen%22%3A0%7D%2C%7B%22name%22%3A%22XYZ%22%7D%2C0%2C792%2Cnull%5D), which is  a multithreaded web server done the "classic" way (each thread handles a connection). I wanted to revisit this simple project with non-blocking I/O, so here you have `epolly`!<br>
This is a project I've been wanting to do for a very long time, simply because I've always been fascinated by epoll and non-blocking I/O.<br>
