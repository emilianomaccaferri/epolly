FROM gcc:latest
COPY . /usr/src/epolly
WORKDIR /usr/src/epolly
RUN apt update
RUN apt install gdb -y
RUN mkdir -p bin
RUN make
CMD ["./bin/epolly"]

