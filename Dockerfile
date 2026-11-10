FROM public.ecr.aws/amazonlinux/amazonlinux:2023 AS builder

RUN dnf install -y gcc make libcurl-devel && dnf clean all
WORKDIR /src
COPY Makefile ./
COPY include ./include
COPY src ./src
RUN make build

FROM public.ecr.aws/lambda/provided:al2023
RUN dnf install -y libcurl && dnf clean all
COPY --from=builder /src/build/bootstrap /var/runtime/bootstrap
RUN chmod 0555 /var/runtime/bootstrap
CMD ["handler"]
