#!/bin/bash
set -e

# Update repository
echo "===== UPDATING LINUX REPOSITORY FOR ARCHITECTURE $2 ====== "
cd /var/telemetryjet-downloads/builds/cli/linux/$2/repo
cp "../telemetryjet-cli-linux_$2_$1.deb" $2
dpkg-sig -k telemetryjet --sign repo "amd64/telemetryjet-cli-linux_$2_$1.deb"
apt-ftparchive packages $2 > Packages
gzip -k -f Packages
apt-ftparchive release . > Release
rm -fr Release.gpg;
gpg --default-key "TelemetryJet" -abs -o Release.gpg Release
rm -fr InRelease;
gpg --default-key "TelemetryJet" --clearsign -o InRelease Release
sudo chown -R www-data:www-data *
sudo chmod 664 KEY.gpg
sudo chmod 664 Packages
sudo chmod 664 Packages.gz
sudo chmod 664 Release
sudo chmod 664 Release.gpg