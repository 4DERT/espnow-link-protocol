FROM mcr.microsoft.com/devcontainers/base:noble
RUN userdel -r ubuntu

RUN apt-get update && apt-get install -y \
    git \
    wget \
    flex \
    bison \
    gperf \
    python3 \
    python3-pip \
    python3-venv \
    cmake \
    ninja-build \
    ccache \
    libffi-dev \
    libssl-dev \
    libusb-1.0-0 \
    dfu-util \
    socat \ 
    esptool

RUN git clone -b v5.2.2 --recursive https://github.com/espressif/esp-idf.git /opt/esp-idf

#RUN sudo apt install udev -y
#RUN echo 'SUBSYSTEM=="tty", ATTRS{idVendor}=="[303a]", ATTRS{idProduct}=="[0002]", SYMLINK+="ttyUSB0", GROUP="dialout", MODE="0666"' > /etc/udev/rules.d/99-usb-serial.rules

USER vscode

RUN /bin/bash -c "/opt/esp-idf/install.sh"

RUN echo "source /opt/esp-idf/export.sh" >> /home/vscode/.bashrc

RUN echo "alias idf=\"idf.py\"" >> /home/vscode/.bashrc


