FROM alpine:latest

RUN apk update && apk add --no-cache \
    boost-dev \
    jsoncpp-dev \
    g++ \
    cmake \
    git \
    libc-dev \
    && rm -rf /var/cache/apk/*

WORKDIR /app

COPY . /app

EXPOSE 8088

RUN g++ -std=c++11 main.cpp -o server -lboost_system -ljsoncpp -I/usr/include/jsoncpp -I./libs

CMD ["./server"]