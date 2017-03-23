xptMiner
========

This is a Riecoin (RIC)-focused release built upon xptMiner.  It
contains only a Riecoin miner - for the reference implementation
of xptMiner, please see https://github.com/jh000/xptMiner

This build supports Linux and Windows, the latter via the Makefile.mingw.

Authors:  xptMiner was written by jh00;
          This version is based upon the Unix port by Clintar
          The riecoin mining core was rewritten by dga based upon jh00's version.
	  Stratum support was added by Gatra.

Some instructions to get started


PREREQUISITES 
=============
CentOS:
sudo yum groupinstall "Development Tools"
sudo yum install openssl openssl-devel openssh-clients gmp gmp-devel gmp-static git wget
# Install primesieve - is there a yum package for it?

Ubuntu:
sudo apt-get -y install build-essential m4 openssl libssl-dev git libjson0 libjson0-dev libcurl4-openssl-dev wget primesieve

Install Primesieve

you'll also need libjansson-dev if you are building for linux

BUILDING
========

wget http://mirrors.kernel.org/gnu/gmp/gmp-5.1.3.tar.bz2
tar xjvf gmp-5.1.3.tar.bz2
cd gmp-5.1.3
./configure --enable-cxx
make -j4 && sudo make install
cd
git clone https://github.com/clintar/xptMiner.git
cd xptMiner
LD_LIBRARY_PATH=/usr/local/lib make -j4
./xptminer -u username.riecoinworkername -p workerpassword


if you get illegal instruction try this

make clean
LD_LIBRARY_PATH=/usr/local/lib make -j4 -f Makefile.mtune

and run it again. If it still gets a segfault, (let me know, please) and try this

make clean
LD_LIBRARY_PATH=/usr/local/lib make -j4 -f Makefile.nomarch


This has a default 2% donation that can be set using the -d option (-d 2.5 would be 2.5% donation)
