FROM debian:latest AS env

RUN apt-get update -y
RUN apt-get install -y \
    build-essential \
    protobuf-compiler \
    cmake \
    pkg-config \
    librdkafka-dev \
    libprotobuf-dev \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswresample-dev \
    libswscale-dev \
    libpqxx-dev \
    libpq-dev

RUN rm -rf /var/lib/apt/lists/*

FROM env

COPY . /app
WORKDIR /app
RUN cmake .
RUN make

CMD [ "./archive_manager"]