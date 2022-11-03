# Sandbox test environment for HackRF
FROM ubuntu:20.04
CMD ["/bin/bash"]

# Override interactive installations and install prerequisites
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  dfu-util \
  gcc-arm-none-eabi \
  git \
  libfftw3-dev \
  libusb-1.0-0-dev \
  pkg-config \
  python3 \
  python3-pip \
  python-is-python3 \
  && rm -rf /var/lib/apt/lists/*
RUN pip3 install git+https://github.com/CapableRobot/CapableRobot_USBHub_Driver --upgrade

# Serial numbers for EUT and TESTER devices connected to the test server
ENV EUT=RunningFromRAM
ENV TESTER=0000000000000000325866e629a25623

# Inform Docker that the container is listening on port 8080 at runtime
EXPOSE 8080
