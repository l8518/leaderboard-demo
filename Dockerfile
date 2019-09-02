FROM gcc:9

# Reduce image size for now!
# COPY ./dataset-sf100 /opt/lsde/dataset-sf100
# COPY ./dataset-sf3000 /opt/lsde/dataset-sf3000

RUN apt-get update
RUN apt-get install -y vim
COPY . /opt/lsde/

WORKDIR /opt/lsde/

RUN make

CMD echo "gcc running" && tail -f /dev/null