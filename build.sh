rm -rf build
mkdir build
sudo apt install -y software-properties-common apt-transport-https ca-certificates gnupg curl  wget unzip libxss-dev libssl-dev  cmake g++ libboost1.74-dev libboost-filesystem1.74-dev libboost-system1.74-dev libboost-program-options1.74-dev libboost-thread1.74-dev libgtkmm-3.0-dev
wget -qO - http://mirrors.mit.edu/ubuntu-ports/project/ubuntu-archive-keyring.gpg | sudo apt-key add -
sudo echo "deb http://mirrors.mit.edu/ubuntu-ports/ bionic main universe" >> /etc/apt/sources.list
sudo apt-get update
sudo apt-get install -y libwebkitgtk-3.0-dev
sudo apt install -y libgtkmm-3.0-dev libglibmm-2.4-dev libzmq3-dev libspdlog-dev libgtest-dev libgmock-dev libgstreamer-plugins-good1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer1.0-dev
sudo apt install -y libglu1-mesa freeglut3 libzmq5 libgtkmm-3.0-1v5 libcanberra-gtk3-module libgpm2 libslang2 gstreamer1.0-plugins-good gstreamer1.0-plugins-base gstreamer1.0-gl gstreamer1.0-libav gstreamer1.0-gtk3 libspdlog1.10
./rebuild.sh
