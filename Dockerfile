FROM ubuntu

MAINTAINER Christopher Dembia

# opensim-moco is a private repository on GitHub, so we need permission to
# access the repository within the Docker container. Create a Personal Access
# Token on the GitHub website:
# 1. Click user icon in the upper right corner.
# 2. Click Settings.
# 3. Click Developer settings.
# 4. Click Personal access tokens.
# 5. Click Generate new token.
# 6. Give a name to your token, e.g., "opensim-moco Docker"
# 7. Check the box next to "repo"
# 8. Click Generate token.
# 9. Copy the token to the clipboard.
# 10. Run Docker build as follows:
#
#       docker build --build-arg GITHUBTOKEN=<paste> .
#

# TODO: Remove when opensim-moco is public.
ARG GITHUBTOKEN

ARG MOCOBRANCH=preprint

# Avoid interactive timezone prompt when installing packages.
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
        git \
        build-essential libtool autoconf \
        cmake \
        gfortran \
        wget \
        pkg-config \
        libopenblas-dev \
        liblapack-dev \
        python3 python3-dev python3-numpy python3-matplotlib python3-setuptools \
        swig

# Must be careful to not embed the GitHub token in the image.
RUN git config --global url."https://$GITHUBTOKEN:@github.com/".insteadOf "https://github.com/" \
        && git clone https://github.com/stanfordnmbl/opensim-moco.git /opensim-moco \
        && cd /opensim-moco \
        && git checkout $MOCOBRANCH \
        && rm ~/.gitconfig

RUN cd /opensim-moco \
        && git submodule update --init \
        && mkdir ../moco_dependencies_build \
        && cd ../moco_dependencies_build \
        && cmake ../opensim-moco/dependencies -DOPENSIM_PYTHON_WRAPPING=on \
        && make --jobs 4 ipopt \
        && make --jobs 4 \
        && echo "/moco_dependencies_install/adol-c/lib64" >> /etc/ld.so.conf.d/moco.conf \
        && echo "/moco_dependencies_install/ipopt/lib" >> /etc/ld.so.conf.d/moco.conf \
        && ldconfig \
        && rm -r /moco_dependencies_build

RUN cd / \
        && mkdir build \
        && cd build \
        && cmake ../opensim-moco \
            -DMOCO_PYTHON_BINDINGS=on \
            -DBUILD_TESTING=off \
            -DBUILD_EXAMPLES=off \
        && make --jobs 4 install \
        && echo "/opensim-moco-install/sdk/lib" >> /etc/ld.so.conf.d/moco.conf \
        && echo "/opensim-moco-install/sdk/Simbody/lib" >> /etc/ld.so.conf.d/moco.conf \
        && ldconfig \
        && cd /opensim-moco-install/sdk/Python && python3 setup.py install \
        && rm -r /build

COPY . /mocopaper

RUN apt-get update && apt-get install -y python3-opencv

# Matplotlib's default backend requires a DISPLAY / Xserver.
RUN echo 'backend : Agg' >> /mocopaper/matplotlibrc && \
    echo 'font.sans-serif : Arial, Helvetica, sans-serif' >> /mocopaper/matplotlibrc

WORKDIR /mocopaper

ENTRYPOINT ["python3", "moco_article_results.py"]

# TODO: How do we get the output of the container back onto the host machine?
# https://stackoverflow.com/questions/31448821/how-to-write-data-to-host-file-system-from-docker-container