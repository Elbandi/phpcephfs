--TEST--
Test cephfs_mkdir() and cephfs_rmdir() functions : error conditions
--FILE--
<?php
/*  Prototype: bool cephfs_mkdir ( resource $handle, string $pathname [, int $mode] );
    Description: Makes directory
	
    Prototype: bool cephfs_rmdir ( resource $handle, string $dirname );
    Description: Removes directory
*/

echo "*** Testing cephfs_mkdir(): error conditions ***\n";
var_dump( cephfs_mkdir() );  // args < expected
var_dump( cephfs_mkdir(1, 2, 3, 4) );  // args > expected
var_dump( cephfs_mkdir(0, "testdir", 0777, false) );  // args > expected

echo "\n*** Testing cephfs_rmdir(): error conditions ***\n";
var_dump( cephfs_rmdir() );  // args < expected
var_dump( cephfs_rmdir(1, 2, 3) );  // args > expected
var_dump( cephfs_rmdir(0, "testdir", "test") );  // args > expected

echo "Done\n";
?>
--EXPECTF--
*** Testing cephfs_mkdir(): error conditions ***

Warning: cephfs_mkdir() expects at least 2 parameters, 0 given in %s on line %d
bool(false)

Warning: cephfs_mkdir() expects at most 3 parameters, 4 given in %s on line %d
bool(false)

Warning: cephfs_mkdir() expects at most 3 parameters, 4 given in %s on line %d
bool(false)

*** Testing cephfs_rmdir(): error conditions ***

Warning: cephfs_rmdir() expects exactly 2 parameters, 0 given in %s on line %d
bool(false)

Warning: cephfs_rmdir() expects exactly 2 parameters, 3 given in %s on line %d
bool(false)

Warning: cephfs_rmdir() expects exactly 2 parameters, 3 given in %s on line %d
bool(false)
Done
