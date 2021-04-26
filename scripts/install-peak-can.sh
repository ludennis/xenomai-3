sudo apt-get install libpopt-dev

cd ~
git clone http://mmslswdev.itri.org.tw/bitbucket/scm/~a70572/peak-linux-driver-8.9.0.git
cd peak-linux-driver-8.9.0
make all
sudo make install

echo 'Ensure "pcan.ko" kernel module is installed in your system. Add and recompile kernel'
