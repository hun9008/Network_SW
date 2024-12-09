#!/bin/bash

SERVER_IP="127.0.0.1"
SERVER_PORT="9000"

# 0부터 9까지 반복하여 0.0 ~ 0.9 생성
for i in $(seq 1 1 9); do
    decimal_value=$(printf "0.%d" "$i")  # 정수를 소수로 변환
    echo "Starting server with arg: $decimal_value"
    
    # 서버를 포그라운드에서 실행
    ./server_test $SERVER_IP $SERVER_PORT $decimal_value &
    SERVER_PID=$!  # 서버 프로세스 ID 저장

    echo "Server started with PID: $SERVER_PID"

    # 클라이언트를 5번 실행
    for j in {1..10}; do
        echo "Running client_test ($j/10)..."
        ./client_test $SERVER_IP $SERVER_PORT
    done

    # 서버 종료
    echo "Stopping server with PID: $SERVER_PID"
    kill $SERVER_PID  # 서버 프로세스 종료
    wait $SERVER_PID 2>/dev/null  # 서버가 완전히 종료될 때까지 대기

    echo "Server with arg $decimal_value stopped."
done
