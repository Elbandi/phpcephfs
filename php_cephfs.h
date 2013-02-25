#ifndef PHP_CEPHFS_H
#define PHP_CEPHFS_H

#define PHP_CEPHFS_EXTNAME  "cephfs"
#define PHP_CEPHFS_EXTVER   "0.1"

#include "cephfs/libcephfs.h"
#ifdef ZTS
#include "TSRM.h"
#endif

PHPAPI extern int php_stream_parse_fopen_modes(const char *mode, int *open_flags);

#define PHP_CEPHFS_POOL_NAME_MAX_LENGTH 128
#define PHP_CEPHFS_MOUNT_RES_NAME "CEPHFS Mount"
#define PHP_CEPHFS_DIR_RES_NAME "CEPHFS Dir"
#define PHP_CEPHFS_FILE_RES_NAME "CEPHFS File"

typedef struct _php_cephfs_mount {
    struct ceph_mount_info *mount;
} php_cephfs_mount;

typedef struct _php_cephfs_dir_result {
    struct ceph_mount_info *mount;
    struct ceph_dir_result *dirp;
} php_cephfs_dir_result;

typedef struct _php_cephfs_file {
    struct ceph_mount_info *mount;
    int fd;
    off_t size;
    zend_bool eof;
} php_cephfs_file;

PHP_MINIT_FUNCTION(cephfs);
PHP_MSHUTDOWN_FUNCTION(cephfs);
PHP_MINFO_FUNCTION(cephfs);

PHP_FUNCTION(cephfs_create);
PHP_FUNCTION(cephfs_shutdown);
PHP_FUNCTION(cephfs_mount);
PHP_FUNCTION(cephfs_unmount);
PHP_FUNCTION(cephfs_is_mounted);

PHP_FUNCTION(cephfs_conf_read_file);
PHP_FUNCTION(cephfs_conf_set);
PHP_FUNCTION(cephfs_conf_get);

PHP_FUNCTION(cephfs_statfs);
PHP_FUNCTION(cephfs_sync_fs);
PHP_FUNCTION(cephfs_getcwd);
PHP_FUNCTION(cephfs_chdir);

PHP_FUNCTION(cephfs_opendir);
PHP_FUNCTION(cephfs_closedir);
PHP_FUNCTION(cephfs_rewinddir);
PHP_FUNCTION(cephfs_readdir);
PHP_FUNCTION(cephfs_telldir);
PHP_FUNCTION(cephfs_seekdir);
PHP_FUNCTION(cephfs_scandir);

PHP_FUNCTION(cephfs_mkdir);
PHP_FUNCTION(cephfs_rmdir);

PHP_FUNCTION(cephfs_link);
PHP_FUNCTION(cephfs_readlink);
PHP_FUNCTION(cephfs_symlink);

PHP_FUNCTION(cephfs_unlink);
PHP_FUNCTION(cephfs_rename);
PHP_FUNCTION(cephfs_stat);
PHP_FUNCTION(cephfs_lstat);
//ceph_setattr
PHP_FUNCTION(cephfs_chmod);
PHP_FUNCTION(cephfs_chown);
PHP_FUNCTION(cephfs_lchown);
PHP_FUNCTION(cephfs_chgrp);
PHP_FUNCTION(cephfs_lchgrp);
PHP_FUNCTION(cephfs_touch);
PHP_FUNCTION(cephfs_truncate);
PHP_FUNCTION(cephfs_mknod);

PHP_FUNCTION(cephfs_fopen);
PHP_FUNCTION(cephfs_fclose);
PHP_FUNCTION(cephfs_fseek);
PHP_FUNCTION(cephfs_feof);
PHP_FUNCTION(cephfs_fread);
PHP_FUNCTION(cephfs_fwrite);
PHP_FUNCTION(cephfs_fchmod);
PHP_FUNCTION(cephfs_fchown);
PHP_FUNCTION(cephfs_fchgrp);
PHP_FUNCTION(cephfs_ftruncate);
PHP_FUNCTION(cephfs_fsync);
PHP_FUNCTION(cephfs_fstat);
/*
ceph_getxattr
ceph_lgetxattr
ceph_listxattr
ceph_llistxattr
ceph_removexattr
ceph_lremovexattr
ceph_setxattr
ceph_lsetxattr
*/
PHP_FUNCTION(cephfs_get_file_stripe_unit);
PHP_FUNCTION(cephfs_get_file_pool);
#ifdef HAVE_CEPH_GET_FILE_POOL_NAME
PHP_FUNCTION(cephfs_get_file_pool_name);
#endif
PHP_FUNCTION(cephfs_get_file_replication);
#ifdef HAVE_CEPH_GET_POOL_ID
PHP_FUNCTION(cephfs_get_pool_id);
#endif
#ifdef HAVE_CEPH_GET_POOL_REPLICATION
PHP_FUNCTION(cephfs_get_pool_replication);
#endif
PHP_FUNCTION(cephfs_get_file_stripe_address);
#ifdef HAVE_CEPH_GET_STRIPE_UNIT_GRANULARITY
PHP_FUNCTION(cephfs_get_stripe_unit_granularity);
#endif

extern zend_module_entry cephfs_module_entry;
#define phpext_cephfs_ptr &cephfs_module_entry;

#endif