FROM gitpod/workspace-full-vnc
                    
USER gitpod

# Install custom tools, runtime, etc. using apt-get
# For example, the command below would install "bastet" - a command line tetris clone:
#
# RUN sudo apt-get -q update && #     sudo apt-get install -yq bastet && #     sudo rm -rf /var/lib/apt/lists/*
#
# More information: https://www.gitpod.io/docs/config-docker/

RUN sudo apt-get update && \
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -yq \
        aptitude \
        ninja-build \
        libopengl-dev \
        libglew-dev \
        libglm-dev \
        python3-wxgtk4.0 \
        liboce-foundation-dev \
        liboce-ocaf-dev \
        libboost1.71-all-dev \
        libngspice0-dev \
        swig3.0 \
        libwxgtk3.0-gtk3-dev
