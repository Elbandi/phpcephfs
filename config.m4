PHP_ARG_ENABLE(cephfs, [Enable the cephfs extension],
    [  --enable-cephfs[=DIR]     Enable "cephfs" extension support], no)

if test $PHP_CEPHFS != "no"; then

    AC_MSG_CHECKING([for cephfs files (libcephfs.h)])
    for i in $PHP_CEPHFS /usr/local /usr; do
        if test -r $i/include/cephfs/libcephfs.h; then
            CEPHFS_DIR=$i
            AC_MSG_RESULT(found in $i)
            break
        fi
    done

    if test -z "$CEPHFS_DIR"; then
        AC_MSG_RESULT([not found])
        AC_MSG_ERROR([Please reinstall the cephfs library from http://www.ceph.com])
    else
        PHP_ADD_LIBRARY_WITH_PATH(cephfs, $CEPHFS_DIR/$PHP_LIBDIR, CEPHFS_SHARED_LIBADD)
        PHP_ADD_INCLUDE($CEPHFS_DIR/include)
    fi

    AC_MSG_CHECKING([for libSSL files (crypto.h)])
    for i in $PHP_CEPHFS /usr/local /usr; do
        if test -r $i/include/openssl/crypto.h; then
            CRYPTO_DIR=$i
            AC_MSG_RESULT(found in $i)
            break
         fi
    done

    if test -z "$CRYPTO_DIR"; then
        AC_MSG_RESULT([not found])
        AC_MSG_ERROR([Please reinstall the libssl library from http://www.openssl.org/])
    else
        PHP_ADD_LIBRARY_WITH_PATH(crypto, $CRYPTO_DIR/$PHP_LIBDIR, CEPHFS_SHARED_LIBADD)
    fi

    AC_DEFINE(_FILE_OFFSET_BITS,64,[ ])

    AC_CHECK_FUNCS(ceph_get_file_pool_name ceph_get_pool_id ceph_get_pool_replication ceph_get_stripe_unit_granularity)
    PHP_SUBST(CEPHFS_SHARED_LIBADD)
    PHP_NEW_EXTENSION(cephfs, [cephfs.c], $ext_shared)
fi