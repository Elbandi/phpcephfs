/*
 * phpcephfs - A PHP5 extension for using libcephfs
 *
 * Copyright (C) 2011 Wido den Hollander <wido@widodh.nl>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if PHP_WIN32
#include "config.w32.h"
#else
#include <php_config.h>
#endif

#include <arpa/inet.h>
#include <sys/un.h>
#include <grp.h>

#include "php.h"
#include "php_ini.h"
#include "zend_exceptions.h"
#include "ext/standard/info.h"
#include "ext/standard/php_filestat.h"

#include "php_cephfs.h"

#define PHP_SCANDIR_SORT_ASCENDING 0
#define PHP_SCANDIR_SORT_DESCENDING 1
#define PHP_SCANDIR_SORT_NONE 2

static int le_cephfs_mount;
static int le_cephfs_dirp;
static int le_cephfs_file;

static int major = 0, minor = 0, extra = 0;

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO(arginfo_cephfs_create, 0)
    ZEND_ARG_INFO(0, id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_shutdown, 0)
    ZEND_ARG_INFO(0, mount)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_mount, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, root)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_unmount, 0)
    ZEND_ARG_INFO(0, mount)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_is_mounted, 0)
    ZEND_ARG_INFO(0, mount)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_conf_read_file, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_conf_set, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, option)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_conf_get, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, option)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_statfs, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_sync_fs, 0)
    ZEND_ARG_INFO(0, mount)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_getcwd, 0)
    ZEND_ARG_INFO(0, mount)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_chdir, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_cephfs_mkdir, 0, 0, 2)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_rmdir, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_link, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, target)
    ZEND_ARG_INFO(0, link)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_readlink, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_symlink, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, target)
    ZEND_ARG_INFO(0, link)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_unlink, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_rename, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, oldname)
    ZEND_ARG_INFO(0, newname)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_stat, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_lstat, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_chmod, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_chown, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, user)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_lchown, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, user)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_chgrp, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, group)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_lchgrp, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, group)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_cephfs_touch, 0, 0, 2)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, time)
    ZEND_ARG_INFO(0, atime)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_truncate, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, size)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_cephfs_mknod, 0, 0, 3)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, mode)
    ZEND_ARG_INFO(0, major)
    ZEND_ARG_INFO(0, minor)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_opendir, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_closedir, 0)
    ZEND_ARG_INFO(0, dir_handle)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_rewinddir, 0)
    ZEND_ARG_INFO(0, dir_handle)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_readdir, 0)
    ZEND_ARG_INFO(0, dir_handle)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_telldir, 0)
    ZEND_ARG_INFO(0, dir_handle)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_seekdir, 0)
    ZEND_ARG_INFO(0, dir_handle)
    ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_cephfs_scandir, 0, 0, 2)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, sorting_order)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_cephfs_fopen, 0, 0, 3)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, mode)
    ZEND_ARG_INFO(0, access_mode)
    ZEND_ARG_INFO(0, stripe_unit)
    ZEND_ARG_INFO(0, stripe_count)
    ZEND_ARG_INFO(0, object_size)
    ZEND_ARG_INFO(0, data_pool)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_fclose, 0)
    ZEND_ARG_INFO(0, file_handle)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_cephfs_fseek, 0, 0, 2)
    ZEND_ARG_INFO(0, file_handle)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, whence)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_feof, 0)
    ZEND_ARG_INFO(0, file_handle)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_cephfs_fread, 0, 0, 2)
    ZEND_ARG_INFO(0, file_handle)
    ZEND_ARG_INFO(0, length)
    ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_fwrite, 0)
    ZEND_ARG_INFO(0, file_handle)
    ZEND_ARG_INFO(0, str)
    ZEND_ARG_INFO(0, length)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_fchmod, 0)
    ZEND_ARG_INFO(0, file_handle)
    ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_fchown, 0)
    ZEND_ARG_INFO(0, file_handle)
    ZEND_ARG_INFO(0, user)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_fchgrp, 0)
    ZEND_ARG_INFO(0, file_handle)
    ZEND_ARG_INFO(0, group)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_ftruncate, 0)
    ZEND_ARG_INFO(0, file_handle)
    ZEND_ARG_INFO(0, size)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_fsync, 0)
    ZEND_ARG_INFO(0, file_handle)
    ZEND_ARG_INFO(0, dataonly)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_fstat, 0)
    ZEND_ARG_INFO(0, file_handle)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_get_file_stripe_unit, 0)
    ZEND_ARG_INFO(0, file_handle)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_get_file_pool, 0)
    ZEND_ARG_INFO(0, file_handle)
ZEND_END_ARG_INFO()

#ifdef HAVE_CEPH_GET_FILE_POOL_NAME
ZEND_BEGIN_ARG_INFO(arginfo_cephfs_get_file_pool_name, 0)
    ZEND_ARG_INFO(0, file_handle)
ZEND_END_ARG_INFO()
#endif

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_get_file_replication, 0)
    ZEND_ARG_INFO(0, file_handle)
ZEND_END_ARG_INFO()

#ifdef HAVE_CEPH_GET_POOL_ID
ZEND_BEGIN_ARG_INFO(arginfo_cephfs_get_pool_id, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, pool_name)
ZEND_END_ARG_INFO()
#endif

#ifdef HAVE_CEPH_GET_POOL_REPLICATION
ZEND_BEGIN_ARG_INFO(arginfo_cephfs_get_pool_replication, 0)
    ZEND_ARG_INFO(0, mount)
    ZEND_ARG_INFO(0, pool_id)
ZEND_END_ARG_INFO()
#endif

ZEND_BEGIN_ARG_INFO(arginfo_cephfs_get_file_stripe_address, 0)
    ZEND_ARG_INFO(0, file_handle)
    ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

#ifdef HAVE_CEPH_GET_STRIPE_UNIT_GRANULARITY
ZEND_BEGIN_ARG_INFO(arginfo_cephfs_get_stripe_unit_granularity, 0)
    ZEND_ARG_INFO(0, mount)
ZEND_END_ARG_INFO()
#endif
/* }}} */

/* {{{ cephfs_functions[]
 */
const zend_function_entry cephfs_functions[] = {
    PHP_FE(cephfs_create, arginfo_cephfs_create)
    PHP_FE(cephfs_shutdown, arginfo_cephfs_shutdown)
    PHP_FE(cephfs_mount, arginfo_cephfs_mount)
    PHP_FE(cephfs_unmount, arginfo_cephfs_unmount)
    PHP_FE(cephfs_is_mounted, arginfo_cephfs_is_mounted)
    PHP_FE(cephfs_conf_read_file, arginfo_cephfs_conf_read_file)
    PHP_FE(cephfs_conf_set, arginfo_cephfs_conf_set)
    PHP_FE(cephfs_conf_get, arginfo_cephfs_conf_get)
    PHP_FE(cephfs_statfs, arginfo_cephfs_statfs)
    PHP_FE(cephfs_sync_fs, arginfo_cephfs_sync_fs)
    PHP_FE(cephfs_getcwd, arginfo_cephfs_getcwd)
    PHP_FE(cephfs_chdir, arginfo_cephfs_chdir)
    PHP_FE(cephfs_mkdir, arginfo_cephfs_mkdir)
    PHP_FE(cephfs_rmdir, arginfo_cephfs_rmdir)
    PHP_FE(cephfs_unlink, arginfo_cephfs_unlink)
    PHP_FE(cephfs_rename, arginfo_cephfs_rename)
    PHP_FE(cephfs_link, arginfo_cephfs_link)
    PHP_FE(cephfs_readlink, arginfo_cephfs_readlink)
    PHP_FE(cephfs_symlink, arginfo_cephfs_symlink)
    PHP_FE(cephfs_stat, arginfo_cephfs_stat)
    PHP_FE(cephfs_lstat, arginfo_cephfs_lstat)
    PHP_FE(cephfs_chmod, arginfo_cephfs_chmod)
    PHP_FE(cephfs_chown, arginfo_cephfs_chown)
    PHP_FE(cephfs_lchown, arginfo_cephfs_lchown)
    PHP_FE(cephfs_chgrp, arginfo_cephfs_chgrp)
    PHP_FE(cephfs_lchgrp, arginfo_cephfs_lchgrp)
    PHP_FE(cephfs_touch, arginfo_cephfs_touch)
    PHP_FE(cephfs_truncate, arginfo_cephfs_truncate)
    PHP_FE(cephfs_mknod, arginfo_cephfs_mknod)

    PHP_FE(cephfs_opendir, arginfo_cephfs_opendir)
    PHP_FE(cephfs_closedir, arginfo_cephfs_closedir)
    PHP_FE(cephfs_rewinddir, arginfo_cephfs_rewinddir)
    PHP_FE(cephfs_readdir, arginfo_cephfs_readdir)
    PHP_FE(cephfs_telldir, arginfo_cephfs_telldir)
    PHP_FE(cephfs_seekdir, arginfo_cephfs_seekdir)
    PHP_FE(cephfs_scandir, arginfo_cephfs_scandir)

    PHP_FE(cephfs_fopen, arginfo_cephfs_fopen)
    PHP_FE(cephfs_fclose, arginfo_cephfs_fclose)
    PHP_FE(cephfs_fseek, arginfo_cephfs_fseek)
    PHP_FE(cephfs_feof, arginfo_cephfs_feof)
    PHP_FE(cephfs_fread, arginfo_cephfs_fread)
    PHP_FE(cephfs_fwrite, arginfo_cephfs_fwrite)
    PHP_FE(cephfs_fchmod, arginfo_cephfs_fchmod)
    PHP_FE(cephfs_fchown, arginfo_cephfs_fchown)
    PHP_FE(cephfs_fchgrp, arginfo_cephfs_fchgrp)
    PHP_FE(cephfs_ftruncate, arginfo_cephfs_ftruncate)
    PHP_FE(cephfs_fsync, arginfo_cephfs_fsync)
    PHP_FE(cephfs_fstat, arginfo_cephfs_fstat)

    PHP_FE(cephfs_get_file_stripe_unit, arginfo_cephfs_get_file_stripe_unit)
    PHP_FE(cephfs_get_file_pool, arginfo_cephfs_get_file_pool)
#ifdef HAVE_CEPH_GET_FILE_POOL_NAME
    PHP_FE(cephfs_get_file_pool_name, arginfo_cephfs_get_file_pool_name)
#endif
    PHP_FE(cephfs_get_file_replication, arginfo_cephfs_get_file_replication)
#ifdef HAVE_CEPH_GET_POOL_ID
    PHP_FE(cephfs_get_pool_id, arginfo_cephfs_get_pool_id)
#endif
#ifdef HAVE_CEPH_GET_POOL_REPLICATION
    PHP_FE(cephfs_get_pool_replication, arginfo_cephfs_get_pool_replication)
#endif
    PHP_FE(cephfs_get_file_stripe_address, arginfo_cephfs_get_file_stripe_address)
#ifdef HAVE_CEPH_GET_STRIPE_UNIT_GRANULARITY
    PHP_FE(cephfs_get_stripe_unit_granularity, arginfo_cephfs_get_stripe_unit_granularity)
#endif
    {NULL, NULL, NULL}
};
/* }}} */

/* libcephfs C API */

/* {{{ proto resource cephfs_create([string name])
   Create a cephfs */
PHP_FUNCTION(cephfs_create)
{
    php_cephfs_mount *mount_r;
    struct ceph_mount_info *mount;
    char *id = NULL;
    int id_len = 0;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &id, &id_len) == FAILURE) {
        RETURN_FALSE;
    }

    ret = ceph_create(&mount, id);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    mount_r = (php_cephfs_mount *)emalloc(sizeof(php_cephfs_mount));
    mount_r->mount = mount;
    ZEND_REGISTER_RESOURCE(return_value, mount_r, le_cephfs_mount);
}
/* }}} */

/* {{{ proto bool cephfs_shutdown(resource handle)
   Shuts down a cephfs */
PHP_FUNCTION(cephfs_shutdown)
{
    php_cephfs_mount *mount_r;
    zval *zmount;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zmount) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ceph_shutdown(mount_r->mount);

    RETURN_BOOL(zend_list_delete(Z_RESVAL_P(zmount)) == SUCCESS);
}
/* }}} */

/* {{{ proto bool cephfs_mount(resource handle, string root)
   Mount a cephfs within the root. */
PHP_FUNCTION(cephfs_mount)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *root;
    int root_len;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zmount, &root, &root_len) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_mount(mount_r->mount, root);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_unmount(resource handle)
   Unmount a mounted cephfs */
PHP_FUNCTION(cephfs_unmount)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zmount) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_unmount(mount_r->mount);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_is_mounted(resource handle)
   Is this a mounted cephfs */
PHP_FUNCTION(cephfs_is_mounted)
{
    php_cephfs_mount *mount_r;
    zval *zmount;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zmount) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    if (ceph_is_mounted(mount_r->mount) == 0) {
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_conf_read_file(resource handle, string path)
   Load the ceph configuration */
PHP_FUNCTION(cephfs_conf_read_file)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *path;
    int path_len;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zmount, &path, &path_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_conf_read_file(mount_r->mount, path);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_conf_set(resource handle, string option, string value)
   Sets a configuration value */
PHP_FUNCTION(cephfs_conf_set)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *option, *value;
    int option_len, value_len;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &zmount, &option, &option_len, &value, &value_len) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_conf_set(mount_r->mount, option, value);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_conf_get(resource handle, string option)
   Gets the configuration value */
PHP_FUNCTION(cephfs_conf_get)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *option;
    int option_len;
    char value[256];
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zmount, &option, &option_len) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_conf_get(mount_r->mount, option, value, sizeof(value));
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_STRINGL(value, strlen(value), 1);
}
/* }}} */

/* {{{ proto bool cephfs_statfs(resource handle, string path)
   Load the ceph configuration */
PHP_FUNCTION(cephfs_statfs)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *path;
    int path_len;
    struct statvfs stbuf;
    zval *capacity, *used, *remaining;
    char *names[3] = {
        "capacity", "used", "remaining"
    };
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zmount, &path, &path_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_statfs(mount_r->mount, path, &stbuf);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    array_init(return_value);

    MAKE_LONG_ZVAL_INCREF(capacity, (long)stbuf.f_blocks*stbuf.f_bsize/1024);
    MAKE_LONG_ZVAL_INCREF(used, (long)(stbuf.f_blocks-stbuf.f_bavail)*stbuf.f_bsize/1024);
    MAKE_LONG_ZVAL_INCREF(remaining, (long)stbuf.f_bavail*stbuf.f_bsize/1024);

    /* Store numeric indexes in propper order */
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&capacity, sizeof(zval *), NULL);
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&used, sizeof(zval *), NULL);
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&remaining, sizeof(zval *), NULL);

    /* Store string indexes referencing the same zval*/
    zend_hash_update(HASH_OF(return_value), names[0], strlen(names[0])+1, (void *) &capacity, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), names[1], strlen(names[1])+1, (void *) &used, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), names[2], strlen(names[2])+1, (void *) &remaining, sizeof(zval *), NULL);
}
/* }}} */

/* {{{ proto bool cephfs_sync_fs(resource handle)
   Synchronizes filesystem */
PHP_FUNCTION(cephfs_sync_fs)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zmount) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_sync_fs(mount_r->mount);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto mixed cephfs_getcwd(resource handle)
   Gets the current directory */
PHP_FUNCTION(cephfs_getcwd)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    const char *result;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zmount) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    if (!(result = ceph_getcwd(mount_r->mount))) {
        RETURN_FALSE;
    }
    RETVAL_STRING((char*) result, 1);
}
/* }}} */

/* {{{ proto bool cephfs_chdir(resource handle, string path)
   Change the current directory */
PHP_FUNCTION(cephfs_chdir)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *path;
    int path_len;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zmount, &path, &path_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_chdir(mount_r->mount, path);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_mkdir(resource handle, string pathname [, int mode [, bool recursive]])
   Create a directory */
PHP_FUNCTION(cephfs_mkdir)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *path;
    int path_len;
    long mode = 0777;
    zend_bool recursive = 0;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|lb", &zmount, &path, &path_len, &mode, &recursive) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    if (recursive) {
        ret = ceph_mkdirs(mount_r->mount, path, (mode_t)mode);
    } else {
        ret = ceph_mkdir(mount_r->mount, path, (mode_t)mode);
    }

    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_rmdir(resource handle, string path)
   Remove a directory */
PHP_FUNCTION(cephfs_rmdir)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *path;
    int path_len;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zmount, &path, &path_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_rmdir(mount_r->mount, path);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto int cephfs_link(resource handle, string target, string link)
   Create a hard link */
PHP_FUNCTION(cephfs_link)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *topath, *frompath;
    int topath_len, frompath_len;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &zmount, &topath, &topath_len, &frompath, &frompath_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(topath) != topath_len) {
        RETURN_FALSE;
    }

    if (strlen(frompath) != frompath_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_link(mount_r->mount, frompath, topath);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto string cephfs_readlink(resource handle, string path)
   Return the target of a symbolic link */
PHP_FUNCTION(cephfs_readlink)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *path;
    int path_len;
    char buff[MAXPATHLEN];
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zmount, &path, &path_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_readlink(mount_r->mount, path, buff, MAXPATHLEN-1);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }
    /* Append NULL to the end of the string */
    buff[ret] = '\0';

    RETURN_STRING(buff, 1);
}
/* }}} */

/* {{{ proto int cephfs_symlink(resource handle, string target, string link)
   Create a symbolic link */
PHP_FUNCTION(cephfs_symlink)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *topath, *frompath;
    int topath_len, frompath_len;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &zmount, &topath, &topath_len, &frompath, &frompath_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(topath) != topath_len) {
        RETURN_FALSE;
    }

    if (strlen(frompath) != frompath_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_symlink(mount_r->mount, frompath, topath);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_unlink(resource handle, string filename)
   Delete a file */
PHP_FUNCTION(cephfs_unlink)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *path;
    int path_len;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zmount, &path, &path_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_unlink(mount_r->mount, path);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_rename(resource handle, string old_name, string new_name)
   Rename a file */
PHP_FUNCTION(cephfs_rename)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *new_name, *old_name;
    int new_name_len, old_name_len;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &zmount, &old_name, &old_name_len, &new_name, &new_name_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(old_name) != old_name_len) {
        RETURN_FALSE;
    }

    if (strlen(new_name) != new_name_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_rename(mount_r->mount, old_name, new_name);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

static void php_cephfs_stat_return(struct stat stat_sb, zval *return_value TSRMLS_DC) /* {{{ */
{
    zval *stat_dev, *stat_ino, *stat_mode, *stat_nlink, *stat_uid, *stat_gid, *stat_rdev,
         *stat_size, *stat_atime, *stat_mtime, *stat_ctime, *stat_blksize, *stat_blocks;
    char *stat_sb_names[13] = {
        "dev", "ino", "mode", "nlink", "uid", "gid", "rdev",
        "size", "atime", "mtime", "ctime", "blksize", "blocks"
    };

    array_init(return_value);

    MAKE_LONG_ZVAL_INCREF(stat_dev, stat_sb.st_dev);
    MAKE_LONG_ZVAL_INCREF(stat_ino, stat_sb.st_ino);
    MAKE_LONG_ZVAL_INCREF(stat_mode, stat_sb.st_mode);
    MAKE_LONG_ZVAL_INCREF(stat_nlink, stat_sb.st_nlink);
    MAKE_LONG_ZVAL_INCREF(stat_uid, stat_sb.st_uid);
    MAKE_LONG_ZVAL_INCREF(stat_gid, stat_sb.st_gid);
    MAKE_LONG_ZVAL_INCREF(stat_rdev, stat_sb.st_rdev);
    MAKE_LONG_ZVAL_INCREF(stat_size, stat_sb.st_size);
    MAKE_LONG_ZVAL_INCREF(stat_atime, stat_sb.st_atime);
    MAKE_LONG_ZVAL_INCREF(stat_mtime, stat_sb.st_mtime);
    MAKE_LONG_ZVAL_INCREF(stat_ctime, stat_sb.st_ctime);
    MAKE_LONG_ZVAL_INCREF(stat_blksize, stat_sb.st_blksize);
    MAKE_LONG_ZVAL_INCREF(stat_blocks, stat_sb.st_blocks);

    /* Store numeric indexes in propper order */
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_dev, sizeof(zval *), NULL);
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_ino, sizeof(zval *), NULL);
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_mode, sizeof(zval *), NULL);
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_nlink, sizeof(zval *), NULL);
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_uid, sizeof(zval *), NULL);
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_gid, sizeof(zval *), NULL);

    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_rdev, sizeof(zval *), NULL);
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_size, sizeof(zval *), NULL);
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_atime, sizeof(zval *), NULL);
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_mtime, sizeof(zval *), NULL);
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_ctime, sizeof(zval *), NULL);
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_blksize, sizeof(zval *), NULL);
    zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_blocks, sizeof(zval *), NULL);

    /* Store string indexes referencing the same zval*/
    zend_hash_update(HASH_OF(return_value), stat_sb_names[0], strlen(stat_sb_names[0])+1, (void *) &stat_dev, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), stat_sb_names[1], strlen(stat_sb_names[1])+1, (void *) &stat_ino, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), stat_sb_names[2], strlen(stat_sb_names[2])+1, (void *) &stat_mode, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), stat_sb_names[3], strlen(stat_sb_names[3])+1, (void *) &stat_nlink, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), stat_sb_names[4], strlen(stat_sb_names[4])+1, (void *) &stat_uid, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), stat_sb_names[5], strlen(stat_sb_names[5])+1, (void *) &stat_gid, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), stat_sb_names[6], strlen(stat_sb_names[6])+1, (void *) &stat_rdev, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), stat_sb_names[7], strlen(stat_sb_names[7])+1, (void *) &stat_size, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), stat_sb_names[8], strlen(stat_sb_names[8])+1, (void *) &stat_atime, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), stat_sb_names[9], strlen(stat_sb_names[9])+1, (void *) &stat_mtime, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), stat_sb_names[10], strlen(stat_sb_names[10])+1, (void *) &stat_ctime, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), stat_sb_names[11], strlen(stat_sb_names[11])+1, (void *) &stat_blksize, sizeof(zval *), NULL);
    zend_hash_update(HASH_OF(return_value), stat_sb_names[12], strlen(stat_sb_names[12])+1, (void *) &stat_blocks, sizeof(zval *), NULL);
}
/* }}} */

static void php_do_cephfs_stat(INTERNAL_FUNCTION_PARAMETERS, int do_lstat) /* {{{ */
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *path;
    int path_len;
    int ret;
    struct stat stat_sb;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zmount, &path, &path_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    if (do_lstat) {
        ret = ceph_lstat(mount_r->mount, path, &stat_sb);
    } else {
        ret = ceph_stat(mount_r->mount, path, &stat_sb);
    }

    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }
    php_cephfs_stat_return(stat_sb, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto array cephfs_stat(resource handle, string path)
   Give information about a file */
PHP_FUNCTION(cephfs_stat)
{
    php_do_cephfs_stat(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto array cephfs_lstat(resource handle, string path)
   Give information about a file or symbolic link */
PHP_FUNCTION(cephfs_lstat)
{
    php_do_cephfs_stat(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto bool cephfs_chmod(resource handle, string path, int mode)
   Change file mode */
PHP_FUNCTION(cephfs_chmod)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *path;
    int path_len;
    long mode;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsl", &zmount, &path, &path_len, &mode) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_chmod(mount_r->mount, path, (mode_t) mode);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

#if PHP_VERSION_ID < 50400
PHPAPI uid_t php_get_uid_by_name(const char *name, uid_t *uid TSRMLS_DC)
{
#if defined(ZTS) && defined(_SC_GETPW_R_SIZE_MAX) && defined(HAVE_GETPWNAM_R)
    struct passwd pw;
    struct passwd *retpwptr = NULL;
    long pwbuflen = sysconf(_SC_GETPW_R_SIZE_MAX);
    char *pwbuf;

    if (pwbuflen < 1) {
        return FAILURE;
    }

    pwbuf = emalloc(pwbuflen);
    if (getpwnam_r(name, &pw, pwbuf, pwbuflen, &retpwptr) != 0 || retpwptr == NULL) {
        efree(pwbuf);
        return FAILURE;
    }
    efree(pwbuf);
    *uid = pw.pw_uid;
#else
    struct passwd *pw = getpwnam(name);

    if (!pw) {
        return FAILURE;
    }
    *uid = pw->pw_uid;
#endif
    return SUCCESS;
}
#endif

#define CEPHFS_HANDLE 0
#define CEPHFS_FILE 1
#define CEPHFS_LINK 2

static void php_do_cephfs_chown(INTERNAL_FUNCTION_PARAMETERS, int do_type) /* {{{ */
{
    php_cephfs_mount *mount_r = NULL;
    php_cephfs_file *file_r = NULL;
    zval *zresource;
    char *path;
    int path_len;
    zval *user;
    uid_t uid;
    int ret;

    if (do_type) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsz/", &zresource, &path, &path_len, &user) == FAILURE) {
            RETURN_FALSE;
        }

        if (strlen(path) != path_len) {
            RETURN_FALSE;
        }
    } else {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz/", &zresource, &user) == FAILURE) {
            RETURN_FALSE;
        }
    }

    if (Z_TYPE_P(user) == IS_LONG) {
        uid = (uid_t)Z_LVAL_P(user);
    } else if (Z_TYPE_P(user) == IS_STRING) {
        if(php_get_uid_by_name(Z_STRVAL_P(user), &uid TSRMLS_CC) != SUCCESS) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to find uid for %s", Z_STRVAL_P(user));
            RETURN_FALSE;
        }
    } else {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "parameter %d should be string or integer, %s given", do_type == CEPHFS_HANDLE ? 2 : 3, zend_zval_type_name(user));
        RETURN_FALSE;
    }

    if (do_type != CEPHFS_HANDLE) {
        ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zresource, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);
    } else {
        ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zresource, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);
    }

    if (do_type == CEPHFS_LINK) {
        ret = ceph_lchown(mount_r->mount, path, uid, -1);
    } else if (do_type == CEPHFS_FILE) {
        ret = ceph_chown(mount_r->mount, path, uid, -1);
    } else {
        ret = ceph_fchown(file_r->mount, file_r->fd, uid, -1);
    }

    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_chown(resource handle, string path, mixed user)
   Change file owner */
PHP_FUNCTION(cephfs_chown)
{
    php_do_cephfs_chown(INTERNAL_FUNCTION_PARAM_PASSTHRU, CEPHFS_FILE);
}
/* }}} */

/* {{{ proto bool cephfs_lchown(resource handle, string path, mixed user)
   Change symlink owner */
PHP_FUNCTION(cephfs_lchown)
{
    php_do_cephfs_chown(INTERNAL_FUNCTION_PARAM_PASSTHRU, CEPHFS_LINK);
}
/* }}} */

#if PHP_VERSION_ID < 50400
PHPAPI int php_get_gid_by_name(const char *name, gid_t *gid TSRMLS_DC)
{
#if defined(ZTS) && defined(HAVE_GETGRNAM_R) && defined(_SC_GETGR_R_SIZE_MAX)
    struct group gr;
    struct group *retgrptr;
    long grbuflen = sysconf(_SC_GETGR_R_SIZE_MAX);
    char *grbuf;

    if (grbuflen < 1) {
        return FAILURE;
    }

    grbuf = emalloc(grbuflen);
    if (getgrnam_r(name, &gr, grbuf, grbuflen, &retgrptr) != 0 || retgrptr == NULL) {
        efree(grbuf);
        return FAILURE;
    }
    efree(grbuf);
    *gid = gr.gr_gid;
#else
    struct group *gr = getgrnam(name);

    if (!gr) {
        return FAILURE;
    }
    *gid = gr->gr_gid;
#endif
    return SUCCESS;
}
#endif

static void php_do_cephfs_chgrp(INTERNAL_FUNCTION_PARAMETERS, int do_type) /* {{{ */
{
    php_cephfs_mount *mount_r = NULL;
    php_cephfs_file *file_r = NULL;
    zval *zresource;
    char *path;
    int path_len;
    zval *group;
    gid_t gid;
    int ret;

    if (do_type) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsz/", &zresource, &path, &path_len, &group) == FAILURE) {
            RETURN_FALSE;
        }

        if (strlen(path) != path_len) {
            RETURN_FALSE;
        }
    } else {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz/", &zresource, &group) == FAILURE) {
            RETURN_FALSE;
        }
    }

    if (Z_TYPE_P(group) == IS_LONG) {
        gid = (gid_t)Z_LVAL_P(group);
    } else if (Z_TYPE_P(group) == IS_STRING) {
        if(php_get_gid_by_name(Z_STRVAL_P(group), &gid TSRMLS_CC) != SUCCESS) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to find gid for %s", Z_STRVAL_P(group));
            RETURN_FALSE;
        }
    } else {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "parameter %d should be string or integer, %s given", do_type == CEPHFS_HANDLE ? 2 : 3, zend_zval_type_name(group));
        RETURN_FALSE;
    }

    if (do_type != CEPHFS_HANDLE) {
        ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zresource, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);
    } else {
        ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zresource, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);
    }

    if (do_type == CEPHFS_LINK) {
        ret = ceph_lchown(mount_r->mount, path, -1, gid);
    } else if (do_type == CEPHFS_FILE) {
        ret = ceph_chown(mount_r->mount, path, -1, gid);
    } else {
        ret = ceph_fchown(file_r->mount, file_r->fd, -1, gid);
    }

    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_chgrp(resource handle, string path, mixed group)
   Change file group */
PHP_FUNCTION(cephfs_chgrp)
{
    php_do_cephfs_chgrp(INTERNAL_FUNCTION_PARAM_PASSTHRU, CEPHFS_FILE);
}
/* }}} */

/* {{{ proto bool cephfs_lchgrp(resource handle, string path, mixed group)
   Change symlink group */
PHP_FUNCTION(cephfs_lchgrp)
{
    php_do_cephfs_chgrp(INTERNAL_FUNCTION_PARAM_PASSTHRU, CEPHFS_LINK);
}
/* }}} */

/* {{{ proto bool cephfs_touch(resource handle, string path[, int time [, int atime]])
   Set modification time of file */
PHP_FUNCTION(cephfs_touch)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    int argc = ZEND_NUM_ARGS();
    char *path;
    int path_len;
    long filetime = 0, fileatime = 0;
    struct utimbuf newtime;
    int ret;

    if (zend_parse_parameters(argc TSRMLS_CC, "rs|ll", &zmount, &path, &path_len, &filetime, &fileatime) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    switch (argc) {
        case 2:
            newtime.modtime = newtime.actime = time(NULL);
            break;
        case 3:
            newtime.modtime = newtime.actime = filetime;
            break;
        case 4:
            newtime.modtime = filetime;
            newtime.actime = fileatime;
            break;
        default:
            /* Never reached */
            WRONG_PARAM_COUNT;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    /* TODO: create the file if it doesn't exist already */

    ret = ceph_utime(mount_r->mount, path, &newtime);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_truncate(resource handle, string path, int size)
   Truncate file to 'size' length */
PHP_FUNCTION(cephfs_truncate)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *path;
    int path_len;
    long size;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsl", &zmount, &path, &path_len, &size) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_truncate(mount_r->mount, path, size);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_mknod(resource handle, string path, int mode [, int major [, int minor]])
   Make a special or ordinary file */
PHP_FUNCTION(cephfs_mknod)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *path;
    int path_len;
    long mode;
    long major = 0, minor = 0;
    dev_t php_dev = 0;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsl", &zmount, &path, &path_len, &mode, &major, &minor) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    if ((mode & S_IFCHR) || (mode & S_IFBLK)) {
        if (ZEND_NUM_ARGS() == 3) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "For S_IFCHR and S_IFBLK you need to pass a major device kernel identifier");
            RETURN_FALSE;
        }
        if (major == 0) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "Expects argument 4 to be non-zero for POSIX_S_IFCHR and POSIX_S_IFBLK");
            RETURN_FALSE;
        } else {
#if defined(HAVE_MAKEDEV) || defined(makedev)
            php_dev = makedev(major, minor);
#else
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot create a block or character device, creating a normal file instead");
#endif
        }
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_mknod(mount_r->mount, path, mode, php_dev);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto mixed cephfs_opendir(resource handle, string path)
   Open a directory and return a dir_handle */
PHP_FUNCTION(cephfs_opendir)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    php_cephfs_dir_result *dir_result_r;
    struct ceph_dir_result *dir_result;
    char *path;
    int path_len;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zmount, &path, &path_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_opendir(mount_r->mount, path, &dir_result);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    dir_result_r = (php_cephfs_dir_result *)emalloc(sizeof(php_cephfs_dir_result));
    dir_result_r->mount = mount_r->mount;
    dir_result_r->dirp = dir_result;
    ZEND_REGISTER_RESOURCE(return_value, dir_result_r, le_cephfs_dirp);
}
/* }}} */

/* {{{ proto bool cephfs_closedir(resource dir_handle)
   Close directory connection identified by the dir_handle */
PHP_FUNCTION(cephfs_closedir)
{
    php_cephfs_dir_result *dir_result_r;
    zval *zdirp;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zdirp) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(dir_result_r, php_cephfs_dir_result*, &zdirp, -1, PHP_CEPHFS_DIR_RES_NAME, le_cephfs_dirp);

    ret = ceph_closedir(dir_result_r->mount, dir_result_r->dirp);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_BOOL(zend_list_delete(Z_RESVAL_P(zdirp)) == SUCCESS);
}
/* }}} */

/* {{{ proto string cephfs_readdir(resource dir_handle)
   Read directory entry from dir_handle */
PHP_FUNCTION(cephfs_readdir)
{
    php_cephfs_dir_result *dir_result_r;
    zval *zdirp;
    struct dirent entry;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zdirp) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(dir_result_r, php_cephfs_dir_result*, &zdirp, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_dirp);

    ret = ceph_readdir_r(dir_result_r->mount, dir_result_r->dirp, &entry);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    if (ret == 0) { //no more entry
        RETURN_FALSE;
    }
    RETURN_STRINGL(entry.d_name, strlen(entry.d_name), 1);
}
/* }}} */

/* {{{ proto void cephfs_rewinddir(resource dir_handle)
   Rewind dir_handle back to the start */
PHP_FUNCTION(cephfs_rewinddir)
{
    php_cephfs_dir_result *dir_result_r;
    zval *zdirp;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zdirp) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(dir_result_r, php_cephfs_dir_result*, &zdirp, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_dirp);

    ceph_rewinddir(dir_result_r->mount, dir_result_r->dirp);
}
/* }}} */

/* {{{ proto int cephfs_telldir(resource dir_handle)
   Get dir position pointer */
PHP_FUNCTION(cephfs_telldir)
{
    php_cephfs_dir_result *dir_result_r;
    zval *zdirp;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zdirp) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(dir_result_r, php_cephfs_dir_result*, &zdirp, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_dirp);

    RETURN_LONG(ceph_telldir(dir_result_r->mount, dir_result_r->dirp));
}
/* }}} */

/* {{{ proto bool cephfs_seekdir(resource dir_handle, int offset)
   Seek on a dir pointer */
PHP_FUNCTION(cephfs_seekdir)
{
    php_cephfs_dir_result *dir_result_r;
    zval *zdirp;
    long offset;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zdirp, &offset) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(dir_result_r, php_cephfs_dir_result*, &zdirp, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_dirp);

    ceph_seekdir(dir_result_r->mount, dir_result_r->dirp, offset);

    RETURN_TRUE;
}
/* }}} */

/* {{{ php_cephfs_scandir
 */
static int php_cephfs_scandir(struct ceph_mount_info *cmount, char *dirname, char **namelist[],
              int (*compare) (const char **a, const char **b) TSRMLS_DC)
{
    struct ceph_dir_result *dir_result;
    struct dirent entry;
    char **vector = NULL;
    unsigned int vector_size = 0;
    unsigned int nfiles = 0;
    int ret;

    if (!namelist) {
        return FAILURE;
    }

    ret = ceph_opendir(cmount, dirname, &dir_result);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        return FAILURE;
    }

    while ((ret = ceph_readdir_r(cmount, dir_result, &entry)) != 0) {
        if (ret < 0) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
            /* we are in error state, dont care the closedir result */
            ceph_closedir(cmount, dir_result);
            return FAILURE;
        }
        if (nfiles == vector_size) {
            if (vector_size == 0) {
                vector_size = 10;
            } else {
                if(vector_size*2 < vector_size) {
                    /* overflow */
                    efree(vector);
                    return FAILURE;
                }
                vector_size *= 2;
            }
            vector = (char **) safe_erealloc(vector, vector_size, sizeof(char *), 0);
        }

        vector[nfiles] = estrdup(entry.d_name);

        nfiles++;
        if(vector_size < 10 || nfiles == 0) {
            /* overflow */
            efree(vector);
            return FAILURE;
        }
    }

    ret = ceph_closedir(cmount, dir_result);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        return FAILURE;
    }

    *namelist = vector;

    if (compare) {
        qsort(*namelist, nfiles, sizeof(char *), (int(*)(const void *, const void *))compare);
    }
    return nfiles;
}
/* }}} */

/* {{{ proto array cephfs_scandir(resource handle, string dir [, int sorting_order])
   List files & directories inside the specified path */
PHP_FUNCTION(cephfs_scandir)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *path;
    int path_len;
    long flags = 0;
    char **namelist;
    int n, i;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|l", &zmount, &path, &path_len, &flags) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    if (path_len < 1) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Directory name cannot be empty");
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    if (flags == PHP_SCANDIR_SORT_ASCENDING) {
        n = php_cephfs_scandir(mount_r->mount, path, &namelist, (void *) php_stream_dirent_alphasort);
    } else if (flags == PHP_SCANDIR_SORT_NONE) {
        n = php_cephfs_scandir(mount_r->mount, path, &namelist, NULL);
    } else {
        n = php_cephfs_scandir(mount_r->mount, path, &namelist, (void *) php_stream_dirent_alphasortr);
    }
    if (n < 0) {
        RETURN_FALSE;
    }

    array_init(return_value);

    for (i = 0; i < n; i++) {
        add_next_index_string(return_value, namelist[i], 1);
    }

    if (n) {
        efree(namelist);
    }
}
/* }}} */

/* {{{ proto resource cephfs_fopen(resource handle, string path, string mode [, int access_mode[, int stripe_unit[, int stripe_count[, int object_size[, string data_pool]]]]])
   Open a file and return a file pointer */
PHP_FUNCTION(cephfs_fopen)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    php_cephfs_file *file_r;
    char *path, *mode;
    int path_len, mode_len;
    int open_flags;
    long amode = 0777;
    int stripe_unit = 0;
    int stripe_count = 0;
    int object_size = 0;
    char *data_pool = NULL;
    int data_pool_len = 0;
    struct stat stbuf;
    int fd, ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss|lllls", &zmount, &path, &path_len, &mode, &mode_len,
            &amode, &stripe_unit, &stripe_count, &object_size, &data_pool, &data_pool_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (strlen(path) != path_len) {
        RETURN_FALSE;
    }

    if (FAILURE == php_stream_parse_fopen_modes(mode, &open_flags)) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    fd = ceph_open_layout(mount_r->mount, path, open_flags, (mode_t)amode, stripe_unit, stripe_count, object_size, data_pool);
    if (fd < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-fd));
        RETURN_FALSE;
    }

    ret = ceph_fstat(mount_r->mount, fd, &stbuf);
    if (ret < 0) {
        ceph_close(mount_r->mount, fd);
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    file_r = (php_cephfs_file *)emalloc(sizeof(php_cephfs_file));
    file_r->mount = mount_r->mount;
    file_r->fd = fd;
    file_r->eof = 0;
    file_r->size = stbuf.st_size;
    ZEND_REGISTER_RESOURCE(return_value, file_r, le_cephfs_file);
}
/* }}} */

/* {{{ proto bool cephfs_fclose(resource fp)
   Close an open file pointer */
PHP_FUNCTION(cephfs_fclose)
{
    php_cephfs_file *file_r;
    zval *zfile;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zfile) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    ret = ceph_close(file_r->mount, file_r->fd);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_BOOL(zend_list_delete(Z_RESVAL_P(zfile)) == SUCCESS);
}
/* }}} */

/* {{{ proto int cephfs_fseek(resource fp, int offset [, int whence])
   Seek on a file pointer */
PHP_FUNCTION(cephfs_fseek)
{
    php_cephfs_file *file_r;
    zval *zfile;
    long offset, whence = SEEK_SET;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl|l", &zfile, &offset, &whence) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    ret = ceph_lseek(file_r->mount, file_r->fd, offset, whence);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    file_r->eof = file_r->size == ret;
    RETURN_LONG(ret);
}
/* }}} */

/* {{{ proto bool cephfs_feof(resource fp)
   Test for end-of-file on a file pointer */
PHP_FUNCTION(cephfs_feof)
{
    php_cephfs_file *file_r;
    zval *zfile;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zfile) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    RETURN_BOOL(file_r->eof);
}
/* }}} */

/* {{{ proto string cephfs_fread(resource fp, int length [, int offset])
   Binary-safe file read */
PHP_FUNCTION(cephfs_fread)
{
    php_cephfs_file *file_r;
    zval *zfile;
    long len;
    long offset = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl|l", &zfile, &len, &offset) == FAILURE) {
        RETURN_FALSE;
    }

    if (len <= 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Length parameter must be greater than 0");
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    Z_STRVAL_P(return_value) = emalloc(len + 1);
    Z_STRLEN_P(return_value) = ceph_read(file_r->mount, file_r->fd, Z_STRVAL_P(return_value), len, offset);

    if (Z_STRLEN_P(return_value) < 0) {
//        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-Z_STRVAL_P(return_value)));
        efree(Z_STRVAL_P(return_value));
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "could not read valid data from ceph");
        RETURN_FALSE;
    }

    if (len > 0) {
        file_r->eof = Z_STRLEN_P(return_value) < len;
    }
    Z_STRVAL_P(return_value)[Z_STRLEN_P(return_value)] = 0;
    Z_TYPE_P(return_value) = IS_STRING;
}
/* }}} */

/* {{{ proto int cephfs_fwrite(resource fp, string str [, int length])
   Binary-safe file write */
PHP_FUNCTION(cephfs_fwrite)
{
    php_cephfs_file *file_r;
    zval *zfile;
    char *str;
    int str_len;
    long len = 0;
    int num_bytes;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|l", &zfile, &str, &str_len, &len) == FAILURE) {
        RETURN_FALSE;
    }
    if (ZEND_NUM_ARGS() == 2) {
        num_bytes = str_len;
    } else {
        num_bytes = MAX(0, MIN((int)len, str_len));
    }

    if (!num_bytes) {
        RETURN_LONG(0);
    }

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    ret = ceph_write(file_r->mount, file_r->fd, str, num_bytes, -1);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_LONG(ret);
}
/* }}} */

/* {{{ proto bool cephfs_fchmod(resource fp, int mode)
   Change file mode */
PHP_FUNCTION(cephfs_fchmod)
{
    php_cephfs_file *file_r;
    zval *zfile;
    long mode;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zfile, &mode) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    ret = ceph_fchmod(file_r->mount, file_r->fd, (mode_t) mode);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_fchown(resource fp, int size)
   Change file owner */
PHP_FUNCTION(cephfs_fchown)
{
    php_do_cephfs_chown(INTERNAL_FUNCTION_PARAM_PASSTHRU, CEPHFS_HANDLE);
}
/* }}} */

/* {{{ proto bool cephfs_fchgrp(resource fp, int size)
   Change file group */
PHP_FUNCTION(cephfs_fchgrp)
{
    php_do_cephfs_chgrp(INTERNAL_FUNCTION_PARAM_PASSTHRU, CEPHFS_HANDLE);
}
/* }}} */

/* {{{ proto bool cephfs_ftruncate(resource fp, int size)
   Truncate file to 'size' length */
PHP_FUNCTION(cephfs_ftruncate)
{
    php_cephfs_file *file_r;
    zval *zfile;
    long size;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zfile, &size) == FAILURE) {
        RETURN_FALSE;
    }

    // TODO: need here to check size < 0, or ceph_ftruncate do this

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    ret = ceph_ftruncate(file_r->mount, file_r->fd, size);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool cephfs_fsync(resource fp, bool dataonly)
   Synchronizes file */
PHP_FUNCTION(cephfs_fsync)
{
    php_cephfs_file *file_r;
    zval *zfile;
    zend_bool dataonly;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rb", &zfile, &dataonly) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    ret = ceph_fsync(file_r->mount, file_r->fd, dataonly);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto array cephfs_fstat(resource fp)
   Stat() on a filehandle */
PHP_FUNCTION(cephfs_fstat)
{
    php_cephfs_file *file_r;
    zval *zfile;
    struct stat stat_sb;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zfile) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    ret = ceph_fstat(file_r->mount, file_r->fd, &stat_sb);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    php_cephfs_stat_return(stat_sb, return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto int cephfs_get_file_stripe_unit(resource fp)
   Get the file striping unit */
PHP_FUNCTION(cephfs_get_file_stripe_unit)
{
    php_cephfs_file *file_r;
    zval *zfile;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zfile) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    ret = ceph_get_file_stripe_unit(file_r->mount, file_r->fd);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_LONG(ret);
}
/* }}} */

/* {{{ proto int cephfs_get_file_pool(resource fp)
   Get the file pool id */
PHP_FUNCTION(cephfs_get_file_pool)
{
    php_cephfs_file *file_r;
    zval *zfile;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zfile) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    ret = ceph_get_file_pool(file_r->mount, file_r->fd);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_LONG(ret);
}
/* }}} */

#ifdef HAVE_CEPH_GET_FILE_POOL_NAME
/* {{{ proto string cephfs_get_file_pool_name(resource fp)
   Get the name of the pool a file is stored in */
PHP_FUNCTION(cephfs_get_file_pool_name)
{
    php_cephfs_file *file_r;
    zval *zfile;
    char name[PHP_CEPHFS_POOL_NAME_MAX_LENGTH];
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zfile) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    ret = ceph_get_file_pool_name(file_r->mount, file_r->fd, name, sizeof(name));
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_STRINGL(name, strlen(name), 1);
}
/* }}} */
#endif

/* {{{ proto int cephfs_get_file_replication(resource fp)
   Get the file replication factor */
PHP_FUNCTION(cephfs_get_file_replication)
{
    php_cephfs_file *file_r;
    zval *zfile;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zfile) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    ret = ceph_get_file_replication(file_r->mount, file_r->fd);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_LONG(ret);
}
/* }}} */

#ifdef HAVE_CEPH_GET_POOL_ID
/* {{{ proto int cephfs_get_pool_id(resource handle, string pool_name)
   Get the id of the named pool */
PHP_FUNCTION(cephfs_get_pool_id)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    char *pool_name;
    int pool_name_len;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zmount, &pool_name, &pool_name_len) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_get_pool_id(mount_r->mount, pool_name);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_LONG(ret);
}
/* }}} */
#endif

#ifdef HAVE_CEPH_GET_POOL_REPLICATION
/* {{{ proto int cephfs_get_pool_replication(resource handle, int pool_id)
   Get the pool replication factor */
PHP_FUNCTION(cephfs_get_pool_replication)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    long pool_id;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zmount, &pool_id) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_get_pool_replication(mount_r->mount, pool_id);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_LONG(ret);
}
/* }}} */
#endif

/* inet_ntop should be used instead of inet_ntoa */
int inet_ntoa_lock = 0;

/* {{{ proto array cephfs_get_file_stripe_address(resource fp, int offset)
   Get the OSD address where the primary copy of a file stripe is located */
PHP_FUNCTION(cephfs_get_file_stripe_address)
{
    php_cephfs_file *file_r;
    zval *zfile;
    long offset;
    struct sockaddr_storage *addr = NULL;
    int naddr = 0;
    int n, i;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zfile, &offset) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(file_r, php_cephfs_file*, &zfile, -1, PHP_CEPHFS_FILE_RES_NAME, le_cephfs_file);

    do {
        if (naddr == 0) {
            naddr = 2;
        } else {
            if(naddr*2 < naddr) {
                /* overflow */
                efree(addr);
                RETURN_FALSE;
            }
            naddr *= 2;
        }
        addr = (struct sockaddr_storage *) safe_erealloc(addr, naddr, sizeof(struct sockaddr_storage), 0);
        n = ceph_get_file_stripe_address(file_r->mount, file_r->fd, offset, addr, naddr);
        if (n < 0 && n != -ERANGE) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-n));
            efree(addr);
            RETURN_FALSE;
        }
    } while (n == -ERANGE);

    array_init(return_value);

    for (i = 0; i < n; i++) {
        struct sockaddr_storage *ss = &addr[i];
        struct sockaddr_in *sin;
        struct sockaddr_in6    *sin6;
        char addr6[INET6_ADDRSTRLEN+1];
        struct sockaddr_un *s_un;
        char *buf = NULL;
        switch (ss->ss_family) {
#if HAVE_IPV6
            case AF_INET6:
                sin6 = (struct sockaddr_in6 *) ss;
                inet_ntop(AF_INET6, &sin6->sin6_addr, addr6, INET6_ADDRSTRLEN);
                add_next_index_string(return_value, addr6, 1);
                break;
#endif
            case AF_INET:
                sin = (struct sockaddr_in *) ss;
                while (inet_ntoa_lock == 1);
                inet_ntoa_lock = 1;
                buf = inet_ntoa(sin->sin_addr);
                inet_ntoa_lock = 0;
                add_next_index_string(return_value, buf, 1);
                break;

            case AF_UNIX:
                s_un = (struct sockaddr_un *) ss;
                add_next_index_string(return_value, s_un->sun_path, 1);
                break;

            default:
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unsupported address family %d", ss->ss_family);
                zend_hash_destroy(Z_ARRVAL_P(return_value));
                efree(Z_ARRVAL_P(return_value));
                RETVAL_FALSE;
                break;
        }
    }

    if (n) {
        efree(addr);
    }
}
/* }}} */

#ifdef HAVE_CEPH_GET_STRIPE_UNIT_GRANULARITY
/* {{{ proto int cephfs_get_stripe_unit_granularity(resource handle)
   Get the file layout stripe unit granularity */
PHP_FUNCTION(cephfs_get_stripe_unit_granularity)
{
    php_cephfs_mount *mount_r;
    zval *zmount;
    int ret;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zmount) == FAILURE) {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(mount_r, php_cephfs_mount*, &zmount, -1, PHP_CEPHFS_MOUNT_RES_NAME, le_cephfs_mount);

    ret = ceph_get_stripe_unit_granularity(mount_r->mount);
    if (ret < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(-ret));
        RETURN_FALSE;
    }

    RETURN_LONG(ret);
}
/* }}} */
#endif

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(cephfs)
{
    /* TODO: real destructor, hint: ftp ext */
    le_cephfs_mount = zend_register_list_destructors_ex(NULL, NULL, PHP_CEPHFS_MOUNT_RES_NAME, module_number);
    le_cephfs_dirp = zend_register_list_destructors_ex(NULL, NULL, PHP_CEPHFS_DIR_RES_NAME, module_number);
    le_cephfs_file = zend_register_list_destructors_ex(NULL, NULL, PHP_CEPHFS_FILE_RES_NAME, module_number);

    ceph_version(&major, &minor, &extra);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(cephfs)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(cephfs)
{
    char compiled_ver[16];

    sprintf(compiled_ver, "%d.%d.%d", major, minor, extra);

    php_info_print_table_start();
    php_info_print_table_row(2, "Cephfs", "enabled");
    php_info_print_table_row(2, "Cephfs extension version", PHP_CEPHFS_EXTVER);
    php_info_print_table_row(2, "libcephfs version", compiled_ver);
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ cephfs_module_entry
 */
zend_module_entry cephfs_module_entry = {
    STANDARD_MODULE_HEADER,
    PHP_CEPHFS_EXTNAME,
    cephfs_functions,
    PHP_MINIT(cephfs),
    PHP_MSHUTDOWN(cephfs),
    NULL,
    NULL,
    PHP_MINFO(cephfs),
    PHP_CEPHFS_EXTVER,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_CEPHFS
ZEND_GET_MODULE(cephfs)
#endif
