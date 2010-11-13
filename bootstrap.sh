#!/bin/sh

# On Mac OS X, this really seems to require:
# (These are not included with 10.4, install from source.)
#  autoconf 2.61 
#  libtool 1.5.24

# Set the shell to output what we are doing
set -x

(
libtoolize --force &&
aclocal &&
autoheader &&
autoconf &&
automake --add-missing --foreign
) ||
{
  set +x
  echo "Failure"
  exit 1
}

echo "On Mac OS X, the make step will fail. Copy the last line starting with './bin/sh ./libtool' and append '-framework libhid -framework libusb'. Now the library will be built. Finally, I don't know how to build a framework with autoconf, etc."
