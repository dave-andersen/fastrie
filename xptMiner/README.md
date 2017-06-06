## xptMiner
This is a Riecoin (RIC)-focused release built upon xptMiner. It contains only a
Riecoin miner. For the reference implementation of xptMiner, please see
https://github.com/jh000/xptMiner

This build supports Linux and Windows, the latter via the `Makefile.mingw`.

### Authors
xptMiner was written by jh00; this version is based upon the Unix port by
Clintar. The riecoin mining core was rewritten by dga based upon jh00's
version. Stratum support was added by Gatra.

## Prerequisites
### CentOS/RHEL
```sh
sudo yum groupinstall 'Development Tools' && sudo yum install \
  openssl openssl-devel openssh-clients gmp gmp-devel gmp-static git wget
```

You will also need to install primesieve. There is no official rpm package in
the official CentOS/RHEL/Fedora repositories, however it can be found on
rpmfind.net: http://rpmfind.net/linux/rpm2html/search.php?query=primesieve

Note: The primesieve rpm available at time of writing is for Mageia Cauldron,
not CentOS/RHEL/Fedora. Use at your own risk.

```sh
sudo yum install \
  'ftp://rpmfind.net/linux/mageia/distrib/cauldron/x86_64/media/core/release/primesieve-5.6.0-1.mga6.x86_64.rpm' \
  'ftp://rpmfind.net/linux/mageia/distrib/cauldron/x86_64/media/core/release/lib64primesieve6-5.6.0-1.mga6.x86_64.rpm'
```

### Ubuntu/Debian
```sh
sudo apt-get -y install \
  build-essential m4 openssl libssl-dev git libjson0 libjson0-dev \
  libcurl4-openssl-dev wget primesieve libjansson-dev libprimesieve-dev
```

## Building

```sh
# gmp library
wget http://mirrors.kernel.org/gnu/gmp/gmp-5.1.3.tar.bz2
tar xjvf gmp-5.1.3.tar.bz2
cd gmp-5.1.3
./configure --enable-cxx
make -j4 && sudo make install
cd ..

# xptMiner
git clone https://github.com/clintar/xptMiner.git
cd xptMiner
LD_LIBRARY_PATH=/usr/local/lib make -j4

# Run
./xptminer -u username.riecoinworkername -p workerpassword
```

If you get illegal instruction try this:

```sh
make clean
LD_LIBRARY_PATH=/usr/local/lib make -j4 -f Makefile.mtune
```

And run it again. If it still gets a segfault, let me know please and try this:

```sh
make clean
LD_LIBRARY_PATH=/usr/local/lib make -j4 -f Makefile.nomarch
```

This has a default 2% donation that can be set using the -d option (-d 2.5
would be 2.5% donation).
