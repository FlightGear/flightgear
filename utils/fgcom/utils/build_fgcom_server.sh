#!/bin/bash


PHONEBOOK_SCRIPT="http://clemaez.fr/flightgear/gen_phonebook.pl.txt"
APTNAV_DATA="http://dev.x-plane.com/update/data/AptNav201304XP1000.zip"
DAHDI_SRC="http://downloads.asterisk.org/pub/telephony/dahdi-linux-complete/dahdi-linux-complete-current.tar.gz"
LOGSEP="###########################################"
GETMETAR_SCRIPT="#!/bin/bash
#curl https://tgftp.nws.noaa.gov/data/observations/metar/stations/$1.TXT
echo \"Hello World !\""

ROOT=$PWD


echo $LOGSEP
echo "Install Asterisk & Festival & dependances"
echo ""
sudo apt-get install asterisk asterisk-dahdi festival gzip unzip perl libfile-slurp-perl libdatetime-perl
echo ""
echo ""
wget $DAHDI_SRC -O dahdi_src.tar.gz
tar -zxvf  dahdi_src.tar.gz
mv dahdi*/ dahdi_src/
cd dahdi_src
make
sudo make install
sudo make config
echo ""
echo ""
echo "$GETMETAR_SCRIPT" > getmetar
chmod +x getmetar
sudo mv getmetar /usr/bin/getmetar
cd $ROOT

echo $LOGSEP
echo "Download APT & NAV data"
echo ""
wget $APTNAV_DATA -O aptnav_data.zip
mkdir aptnav && cd aptnav
unzip ../aptnav_data.zip
mv earth_nav.dat nav.dat
gzip apt.dat && mv apt.dat.gz ../apt.dat.gz
gzip nav.dat && mv nav.dat.gz ../nav.dat.gz
cd .. && rm -rf aptnav/
rm aptnav_data.zip
cd $ROOT


echo $LOGSEP
echo "Download PhoneBook script"
echo ""
wget $PHONEBOOK_SCRIPT -O gen_phonebook.pl


echo $LOGSEP
echo "Run PhoneBook script"
echo ""
chmod +x gen_phonebook.pl
./gen_phonebook.pl



echo $LOGSEP
echo "Configure Asterisk"
echo ""

echo "[general]
static=yes
writeprotect=yes
;
[default]
#include \"fgcom.conf\"
include => fgcom" > extensions.conf

cd /etc/asterisk

if [ -f iax.conf.bak ]
then
  sudo rm iax.conf
  sudo mv iax.conf.bak iax.conf
fi
sudo cp iax.conf iax.conf.bak
sudo sed -i '/callerid="Guest IAX User"/a requirecalltoken=no' iax.conf
sudo sed -i '/^;calltokenoptional=/c calltokenoptional=0.0.0.0/0.0.0.0' iax.conf
sudo sed -i '/^;authdebug=no/c authdebug=no' iax.conf

if [ ! -f extensions.conf.bak ]
then
  sudo cp extensions.conf extensions.conf.bak
fi
sudo rm extensions.conf
sudo mv $ROOT/extensions.conf extensions.conf
sudo mv $ROOT/fgcom.conf fgcom.conf

sudo chown asterisk:asterisk extensions.conf
sudo chown asterisk:asterisk extensions.conf.bak
sudo chown asterisk:asterisk iax.conf
sudo chown asterisk:asterisk iax.conf.bak
sudo chown asterisk:asterisk fgcom.conf
sudo mkdir /var/fgcom-server
sudo chown asterisk:asterisk /var/fgcom-server
sudo mkdir /var/fgcom-server/atis
sudo chown asterisk:asterisk /var/fgcom-server/atis
cd $ROOT


echo $LOGSEP
echo "Restart Asterisk"
echo ""
sudo service asterisk restart


echo $LOGSEP
echo "Run Festival"
echo ""
nohup festival --server > /dev/null 2> /dev/null < /dev/null &


echo $LOGSEP
echo "Enter in Asterisk CLI"
echo ""
sudo asterisk -r -vvvvvvvvvv
