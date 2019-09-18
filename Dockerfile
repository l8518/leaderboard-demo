FROM ubuntu:18.04

# Reduce image size for now!
# COPY ./dataset-sf100 /opt/lsde/dataset-sf100
# COPY ./dataset-sf3000 /opt/lsde/dataset-sf3000

RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install build-essential -y
RUN apt-get install g++ valgrind -y
RUN apt-get install -y vim
COPY . /opt/lsde/

WORKDIR /opt/lsde/

RUN make

CMD echo "gcc running" && tail -f /dev/null