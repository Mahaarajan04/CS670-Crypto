FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    g++ cmake make python3 \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN g++ -std=c++20 gen_queries.cpp -o gen_queries \
 && g++ -std=c++20 dealer.cpp -o dealer \
 && g++ -std=c++20 -DROLE_p0 pB.cpp -o p0 \
 && g++ -std=c++20 -DROLE_p1 pB.cpp -o p1 \
 && g++ -std=c++20 -DROLE_p2 pB.cpp -o p2
