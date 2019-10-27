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

ISDIAG_PLATFORM=0

#Create Array
DIAG_PLATFORM0='x86_64-cel_silverstone-r0'
DIAG_PLATFORM1='x86_64-dellemc_z9332f_d1508-r0'

#Onie-sysinfo is read from /etc/machine.conf (onie_platform attribute)
CURRENT_PLATFORM=$(onie-sysinfo)

for index in 0 1 ; do
  eval assign="\$DIAG_PLATFORM$index"
  if [ $assign == $CURRENT_PLATFORM ]; then
    ISDIAG_PLATFORM=`expr $ISDIAG_PLATFORM + 1`
  fi
done

#If the platfrom isn't diag installation require then exit.
if [ $ISDIAG_PLATFORM -eq 0 ]; then
	exit 0
fi

### Change parition name with -DIAG, The uninstall operation must not modify or remove this partiion.
### clear GPT system partition attribute bit (bit 0)
if [ ! -z $(sgdisk -p /dev/sda | grep "ONL-BOOT-DIAG" | awk '{print $1}') ]; then
    sgdisk --change-name=$(sgdisk -p /dev/sda | grep "ONL-BOOT-DIAG" | awk '{print $1}'):"ONL-BOOT" /dev/sda
    sgdisk -A $(sgdisk -p /dev/sda | grep "ONL-BOOT" | awk '{print $1}'):clear:0 /dev/sda
fi
if [ ! -z $(sgdisk -p /dev/sda | grep "ONL-CONFIG-DIAG" | awk '{print $1}') ]; then
    sgdisk --change-name=$(sgdisk -p /dev/sda | grep "ONL-CONFIG-DIAG" | awk '{print $1}'):"ONL-CONFIG" /dev/sda
    sgdisk -A $(sgdisk -p /dev/sda | grep "ONL-CONFIG" | awk '{print $1}'):clear:0 /dev/sda
fi
if [ ! -z $(sgdisk -p /dev/sda | grep "ONL-IMAGES-DIAG" | awk '{print $1}') ]; then
    sgdisk --change-name=$(sgdisk -p /dev/sda | grep "ONL-IMAGES-DIAG" | awk ' {print $1}'):"ONL-IMAGES" /dev/sda
    sgdisk -A $(sgdisk -p /dev/sda | grep "ONL-IMAGES" | awk '{print $1}'):clear:0 /dev/sda
fi
if [ ! -z $(sgdisk -p /dev/sda | grep "ONL-DATA-DIAG" | awk '{print $1}') ]; then
    sgdisk --change-name=$(sgdisk -p /dev/sda | grep "ONL-DATA-DIAG" | awk '{print $1}'):"ONL-DATA" /dev/sda
    sgdisk -A $(sgdisk -p /dev/sda | grep "ONL-DATA" | awk '{print $1}'):clear:0 /dev/sda
fi

## Remove Dummy partition CLS-DIAG if exist
if [ ! -z $(sgdisk -p /dev/sda | grep CLS-DIAG | awk '{print $1}') ]; then
    DUMMY_PARTITION_NUMBER=$(sgdisk -p /dev/sda | grep CLS-DIAG | awk '{print $1}')
    parted /dev/sda rm $DUMMY_PARTITION_NUMBER
fi

## Move back the ONL efi partition for protect the conflict between install process.
if [ -d /boot/efi/EFI/ONL-DIAG ]; then
    mv /boot/efi/EFI/ONL-DIAG /boot/efi/EFI/ONL
fi


exit 0
