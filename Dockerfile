FROM ruby:3.3.1-alpine3.20

RUN apk --no-cache add build-base ragel

COPY . .
RUN gem install bundler:2.4.1 && \
  bundle install -j$(nproc)

WORKDIR /app
