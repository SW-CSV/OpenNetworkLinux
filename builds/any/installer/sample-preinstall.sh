#!/bin/sh
#
######################################################################
#
# sample-preinstall.sh
#
# Example script for pre-install hooks.
#
# Add this as a preinstall hook to your installer via
# the 'mkinstaller.py' command line:
#
# $ mkinstaller.py ... --preinstall-script sample-preinstall.sh ...
#
# At install time, this script will
#
# 1. be extracted into the working directory with the other installer
#    collateral
# 2. have the execute bit set
# 3. run in-place with the installer chroot directory passed
#    as the first command line parameter
#
# If the script fails (returns a non-zero exit code) then
# the install is aborted.
#
# This script is executed using the ONIE runtime (outside the chroot),
# before the actual installer (chrooted Python script)
#
# At the time the script is run, the installer environment (chroot)
# has been fully prepared, including filesystem mount-points.
#
######################################################################

rootdir=$1; shift

echo "Hello from preinstall"
echo "Chroot is $rootdir"


diskpartition=$(df | grep "onie-boot" | awk '{print $1}')
diskname=${diskpartition:0:8}
echo "diskname:"$diskname


### Change parition name with -DIAG, The uninstall operation must not modify or remove this partiion.
### clear GPT system partition attribute bit (bit 0)
if [ ! -z $(sgdisk -p $diskname | grep "ONL-BOOT-DIAG" | awk '{print $1}') ]; then
    sgdisk --change-name=$(sgdisk -p $diskname | grep "ONL-BOOT-DIAG" | awk '{print $1}'):"ONL-BOOT" $diskname
    sgdisk -A $(sgdisk -p $diskname | grep "ONL-BOOT" | awk '{print $1}'):clear:0 $diskname
fi
if [ ! -z $(sgdisk -p $diskname | grep "ONL-CONFIG-DIAG" | awk '{print $1}') ]; then
    sgdisk --change-name=$(sgdisk -p $diskname | grep "ONL-CONFIG-DIAG" | awk '{print $1}'):"ONL-CONFIG" $diskname
    sgdisk -A $(sgdisk -p $diskname | grep "ONL-CONFIG" | awk '{print $1}'):clear:0 $diskname
fi
if [ ! -z $(sgdisk -p $diskname | grep "ONL-IMAGES-DIAG" | awk '{print $1}') ]; then
    sgdisk --change-name=$(sgdisk -p $diskname | grep "ONL-IMAGES-DIAG" | awk ' {print $1}'):"ONL-IMAGES" $diskname
    sgdisk -A $(sgdisk -p $diskname | grep "ONL-IMAGES" | awk '{print $1}'):clear:0 $diskname
fi
if [ ! -z $(sgdisk -p $diskname | grep "ONL-DATA-DIAG" | awk '{print $1}') ]; then
    sgdisk --change-name=$(sgdisk -p $diskname | grep "ONL-DATA-DIAG" | awk '{print $1}'):"ONL-DATA" $diskname
    sgdisk -A $(sgdisk -p $diskname | grep "ONL-DATA" | awk '{print $1}'):clear:0 $diskname
fi

## Remove Dummy partition ONL-ROOTFS-DIAG if exist
if [ ! -z $(sgdisk -p $diskname | grep "ONL-ROOTFS-DIAG" | awk '{print $1}') ]; then
    DUMMY_PARTITION_NUMBER=$(sgdisk -p $diskname | grep "ONL-ROOTFS-DIAG" | awk '{print $1}')
	parted $diskname rm $DUMMY_PARTITION_NUMBER
fi

## Move back the ONL efi partition for protect the conflict between install process.
if [ -d /boot/efi/EFI/ONL-DIAG ]; then
    mv /boot/efi/EFI/ONL-DIAG /boot/efi/EFI/ONL
fi



exit 0

