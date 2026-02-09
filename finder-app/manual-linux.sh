#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
    #echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper

    #defconfig to setup our default config files
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

    # Build a kernel image for booting with QEMU
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

    #Build any kernel modules
    #make ARCH=${ARCH} CROSS_COMPILE=aarch64-none-linux-gnu-modules

    #Build Device tree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi
mkdir -p ${OUTDIR}/rootfs
cd rootfs
# TODO: Create necessary base directories
sudo mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
sudo mkdir -p usr/bin usr/lib usr/sbin
sudo mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    
else
    cd busybox
fi

# TODO: Make and install busybox
make distclean
make defconfig
sed -i 's/^CONFIG_STATIC=y/# CONFIG_STATIC is not set/' .config
sed -i 's/^CONFIG_TC=y/CONFIG_TC=n/' .config
make oldconfig
#make menuconfig
sudo make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
sudo make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
#${CROSS_COMPILE}readelf -a /bin/busybox | grep "program interpreter"
#${CROSS_COMPILE}readelf -a /bin/busybox | grep "Shared library"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
LIBSRC=/usr/aarch64-linux-gnu/lib
LIBDST=${OUTDIR}/rootfs/lib64

cp ${LIBSRC}/ld-linux-aarch64.so.1 ${LIBDST}/
cp ${LIBSRC}/libc.so.6 ${LIBDST}/
cp ${LIBSRC}/libm.so.6 ${LIBDST}/
cp ${LIBSRC}/libresolv.so.2 ${LIBDST}/

mkdir -p ${OUTDIR}/rootfs/lib/aarch64-linux-gnu

cp ${LIBSRC}/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/
cp ${LIBSRC}/libc.so.6 ${OUTDIR}/rootfs/lib/aarch64-linux-gnu/
cp ${LIBSRC}/libm.so.6 ${OUTDIR}/rootfs/lib/aarch64-linux-gnu/
cp ${LIBSRC}/libresolv.so.2 ${OUTDIR}/rootfs/lib/aarch64-linux-gnu/



# TODO: Make device nodes
echo "Making device nodes"
cd ${OUTDIR}/rootfs
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE} all


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
mkdir ${OUTDIR}/rootfs/home/conf
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/../conf/username.txt ${OUTDIR}/rootfs/home/conf
cp ${FINDER_APP_DIR}/../conf/assignment.txt ${OUTDIR}/rootfs/home/conf
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home

cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home/
chmod +x ${OUTDIR}/rootfs/home/autorun-qemu.sh

# TODO: Chown the root directory
#mkdir -p ${OUTDIR}/rootfs/sbin
#ln -s /bin/busybox ${OUTDIR}/rootfs/sbin/init

# TODO: Create initramfs.cpio.gz
cd "$OUTDIR/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio
cp ${OUTDIR}/linux-stable/arch/arm64/boot/Image ${OUTDIR}/

