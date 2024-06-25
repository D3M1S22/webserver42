FROM alpine:latest

COPY --from=golang:1.13-alpine /usr/local/go/ /usr/local/go/

RUN apk update && \
    apk add --no-cache wget || { echo 'Failed to install wget'; exit 1; } && \
    apk add --no-cache python3 || { echo 'Failed to install wget'; exit 1; } && \
    apk add --no-cache make || { echo 'Failed to install make'; exit 1; } && \
    apk add --no-cache g++ || { echo 'Failed to install c++'; exit 1; }



ENV PATH="/usr/local/go/bin:${PATH}"

COPY . ./app

WORKDIR /app

# RUN make all || { echo 'Failed to compile'; exit 1; }
# RUN c++ aa

RUN make re

EXPOSE 8080
EXPOSE 8081

CMD ["./webserver", "config/test.conf"]
