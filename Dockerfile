# Environment for HackRF HIL testing with Jenkins CI
FROM ubuntu:22.04
USER root

# Copy usb hub script from Jenkins' container
COPY --from=gsg-jenkins /startup/hubs.py /startup/hubs.py
COPY --from=gsg-jenkins /startup/.hubs /startup/.hubs
RUN ln -s /startup/hubs.py /usr/local/bin/hubs

# Override interactive installations and install software dependencies
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    curl \
    dfu-util \
    gcc-arm-none-eabi \
    git \
    libfftw3-dev \
    libusb-1.0-0-dev \
    pkg-config \
    python3 \
    python3-pip \
    python3-yaml \
    usbutils \
    && rm -rf /var/lib/apt/lists/*

# Install USB hub PPPS dependencies
RUN pip3 install python-dotenv git+https://github.com/CapableRobot/CapableRobot_USBHub_Driver --upgrade
RUN curl -L https://github.com/mvp/uhubctl/archive/refs/tags/v2.5.0.tar.gz > uhubctl-2.5.0.tar.gz \
    && mkdir uhubctl-2.5.0 \
    && tar -xvzf uhubctl-2.5.0.tar.gz -C uhubctl-2.5.0 --strip-components 1 \
    && rm uhubctl-2.5.0.tar.gz \
    && cd uhubctl-2.5.0 \
    && make \
    && make install

# Inform Docker that the container is listening on port 8080 at runtime
EXPOSE 8080
