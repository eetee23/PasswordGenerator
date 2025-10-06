# How to run docker file inside windows computer
docker build -t password_generator .
docker run -it -v "$(pwd -W)/database:/app/database" password_generator