FROM gcr.io/oss-fuzz-base/base-builder:v1
COPY . $SRC/memforge
WORKDIR $SRC/memforge
COPY .clusterfuzzlite/build.sh $SRC/build.sh
RUN chmod +x $SRC/build.sh
