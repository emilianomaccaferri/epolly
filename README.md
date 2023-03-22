# epolly
A very minimal webserver built on top of epoll 

# inspo
This project has been made for the OS Design exam at Unimore and is based on [this one](http://www.0x04.net/doc/posix/Multi-Threaded%20Programming%20with%20POSIX%20Threads%20-%20Linux%20Systems%20Programming.pdf#%5B%7B%22num%22%3A132%2C%22gen%22%3A0%7D%2C%7B%22name%22%3A%22XYZ%22%7D%2C0%2C792%2Cnull%5D), which is  a multithreaded web server done the "classic" way (each thread handles a connection). I wanted to revisit this simple project with non-blocking I/O, so here you have `epolly`!<br>
This is a project I'd been wanting to do for a very long time, simply because I've always been fascinated by epoll and non-blocking I/O.<br>
# running
```
git clone https://github.com/emilianomaccaferri/epolly/
cd epolly
```
serach for the variable `WWW_PATH` inside the `lib/http_request.c` file (line 9) and set this variable to the path you want to serve via HTTP (e.g. if your index.html is inside `/var/www/html`, just set `WWW_PATH "/var/www/html"` (sorry, it sucks, but I didn't have much time).
```
mkdir bin
make
./bin/epolly
```
you can change some parameters (port, number of threads...) inside `main.c`.
# benchmarks
Tests have been performed on my 6-core AMD Ryzen 5600x with [wrk](https://github.com/wg/wrk).<br>
The results are quite satisfying since I didn't have time to optimize many things:
```

Running 30s test @ http://127.0.0.1:8080/index.html
  12 threads and 400 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    13.08ms   17.20ms 300.37ms   86.14%
    Req/Sec     1.56k     1.69k   10.17k    76.52%
  519453 requests in 30.08s, 201.64MB read
  Socket errors: connect 0, read 519453, write 0, timeout 0
Requests/sec:  17268.68
Transfer/sec:      6.70MB

```
Good job, epolly!
