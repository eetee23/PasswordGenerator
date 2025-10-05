FROM gcc:latest

RUN apt-get update && \
    apt-get install -y sqlite3 libsqlite3-dev xclip && \
    apt-get clean

WORKDIR /app
COPY . .

RUN g++ main.cpp -o PasswordGenerator -lsqlite3

CMD ["./PasswordGenerator"]
