<?php

/**
 * @defgroup filesystemfunctions Filesystem convenience functions.
 * @{
 */

/**
 * Behavior for mash_copy_dir() and mash_move_dir() when destinations exist.
 */
define('FILE_EXISTS_ABORT', 0);
define('FILE_EXISTS_OVERWRITE', 1);
define('FILE_EXISTS_MERGE', 2);

 /**
  * Determines whether the provided path is absolute or not
  * on the specified O.S. -- starts with "/" on *nix, or starts
  * with "[A-Z]:\" or "[A-Z]:/" on Windows.
  */
function mash_is_absolute_path($path, $os = NULL) {
  // Relative paths will never start with a '/', even on Windows,
  // so it is safe to just call all paths that start with a '/'
  // absolute.  This simplifies things for Windows with CYGWIN / MINGW / CWRSYNC,
  // where absolute paths sometimes start c:\path and sometimes
  // start /cygdrive/c/path.
  if ($path[0] == '/') {
    return TRUE;
  }
  if (mash_is_windows($os)) {
    return preg_match('@^[a-zA-Z]:[\\\/]@', $path);
  }
  return FALSE;
}

/**
 * If we are going to pass a path to exec or proc_open,
 * then we need to fix it up under CYGWIN or MINGW.  In
 * both of these environments, PHP works with absolute paths
 * such as "C:\path".  CYGWIN expects these to be converted
 * to "/cygdrive/c/path" and MINGW expects these to be converted
 * to "/c/path"; otherwise, the exec will not work.
 *
 * This call does nothing if the parameter is not an absolute
 * path, or we are not running under CYGWIN / MINGW.
 *
 * UPDATE:  It seems I was mistaken; this is only necessary if we
 * are using cwRsync.  We do not need to correct every path to
 * exec or proc_open (thank god).
 */
function mash_correct_absolute_path_for_exec($path, $os = NULL) {
  if (mash_is_windows() && mash_is_absolute_path($path, "WINNT")) {
    if (mash_is_mingw($os)) {
      $path = preg_replace('/(\w):/', '/${1}', str_replace('\\', '/', $path));
    }
    elseif (mash_is_cygwin($os)) {
      $path = preg_replace('/(\w):/', '/cygdrive/${1}', str_replace('\\', '/', $path));
    }
  }
  return $path;
}

/**
 * Remove the trailing DIRECTORY_SEPARATOR from a path.
 * Will actually remove either / or \ on Windows.
 */
function mash_trim_path($path, $os = NULL) {
  if (mash_is_windows($os)) {
    return rtrim($path, '/\\');
  }
  else {
    return rtrim($path, '/');
  }
}

/**
 * Makes sure the path has only path separators native for the current operating system
 */
function mash_normalize_path($path) {
  if (mash_is_windows()) {
    $path = str_replace('/', '\\',  strtolower($path));
  }
  else {
    $path = str_replace('\\', '/', $path);
  }
  return mash_trim_path($path);
}

/**
 * Calculates a single md5 hash for all files a directory (incuding subdirectories)
 */
function mash_dir_md5($dir) {
  $flist = mash_scan_directory($dir, '/./', array('.', '..'), 0, TRUE, 'filename', 0, TRUE);
  $hashes = array();
  foreach ($flist as $f) {
    $sum = array();
    exec('cksum ' . escapeshellarg($f->filename), $sum);
    $hashes[] = trim(str_replace(array($dir), array(''), $sum[0]));
  }
  sort($hashes);
  return md5(implode("\n", $hashes));
}

/**
 * Deletes the provided file or folder and everything inside it.
 *
 * Usually respects read-only files and folders. To do a forced delete use
 * mash_delete_tmp_dir() or set the parameter $forced.
 *
 * @param $dir
 *   The file or directory to delete.
 * @param $force
 *   Try whatever possible to delete the directory - also read-only ones.
 * @return
 *   FALSE on failure, TRUE if everything was deleted.
 */
function mash_delete_dir($dir, $force = FALSE) {
  // Allow to delete symlinks even if the target doesn't exist.
  if (!is_link($dir) && !file_exists($dir)) {
    return TRUE;
  }
  if (!is_dir($dir)) {
    if ($force) {
      // Force deletion of items with readonly flag.
      @chmod($dir, 0777);
    }
    return unlink($dir);
  }
  foreach (scandir($dir) as $item) {
    if ($item == '.' || $item == '..') {
      continue;
    }
    if ($force) {
      @chmod($dir, 0777);
    }
    if (!mash_delete_dir($dir . '/' . $item, $force)) {
      return FALSE;
    }
  }
  if ($force) {
    // Force deletion of items with readonly flag.
    @chmod($dir, 0777);
  }
  return rmdir($dir);
}

/**
 * Deletes the provided file or folder and everything inside it.
 * This function explicitely tries to delete read-only files / folders.
 *
 * @param $dir
 *   The directory to delete
 * @return
 *   FALSE on failure, TRUE if everything was deleted
 */
function mash_delete_tmp_dir($dir) {
  return mash_delete_dir($dir, TRUE);
}

/**
 * Copy $src to $dest.
 *
 * @param $src
 *   The directory to copy.
 * @param $dest
 *   The destination to copy the source to, including the new name of
 *   the directory.  To copy directory "a" from "/b" to "/c", then
 *   $src = "/b/a" and $dest = "/c/a".  To copy "a" to "/c" and rename
 *   it to "d", then $dest = "/c/d".
 * @param $overwrite
 *   Action to take if destination already exists.
 *     - FILE_EXISTS_OVERWRITE - completely removes existing directory.
 *     - FILE_EXISTS_ABORT - aborts the operation.
 *     - FILE_EXISTS_MERGE - Leaves existing files and directories in place.
 * @return
 *   TRUE on success, FALSE on failure.
 */
function mash_copy_dir($src, $dest, $overwrite = FILE_EXISTS_ABORT) {
  // Preflight based on $overwrite if $dest exists.
  if (file_exists($dest)) {
    if ($overwrite === FILE_EXISTS_OVERWRITE) {
      mash_op('mash_delete_dir', $dest, TRUE);
    }
    elseif ($overwrite === FILE_EXISTS_ABORT) {
      return mash_set_error('MASH_DESTINATION_EXISTS', dt('Destination directory !dest already exists.', array('!dest' => $dest)));
    }
    elseif ($overwrite === FILE_EXISTS_MERGE) {
      // $overwrite flag may indicate we should merge instead.
      mash_log(dt('Merging existing !dest directory', array('!dest' => $dest)));
    }
  }
  // $src readable?
  if (!is_readable($src)) {
    return mash_set_error('MASH_SOURCE_NOT_EXISTS', dt('Source directory !src is not readable or does not exist.', array('!src' => $src)));
  }
  // $dest writable?
  if (!is_writable(dirname($dest))) {
    return mash_set_error('MASH_DESTINATION_NOT_WRITABLE', dt('Destination directory !dest is not writable.', array('!dest' => dirname($dest))));
  }
  // Try to do a recursive copy.
  if (@mash_op('_mash_recursive_copy', $src, $dest)) {
    return TRUE;
  }

  return mash_set_error('MASH_COPY_DIR_FAILURE', dt('Unable to copy !src to !dest.', array('!src' => $src, '!dest' => $dest)));
}

/**
 * Internal function called by mash_copy_dir; do not use directly.
 */
function _mash_recursive_copy($src, $dest) {
  // all subdirectories and contents:
  if(is_dir($src)) {
    if (!mash_mkdir($dest, TRUE)) {
      return FALSE;
    }
    $dir_handle = opendir($src);
    while($file = readdir($dir_handle)) {
      if ($file != "." && $file != "..") {
        if (_mash_recursive_copy("$src/$file", "$dest/$file") !== TRUE) {
          return FALSE;
        }
      }
    }
    closedir($dir_handle);
  }
  elseif (is_link($src)) {
    symlink(readlink($src), $dest);
  }
  elseif (copy($src, $dest) !== TRUE) {
    return FALSE;
  }

  // Preserve execute permission.
  if (!is_link($src) && !mash_is_windows()) {
    // Get execute bits of $src.
    $execperms = fileperms($src) & 0111;
    // Apply execute permissions if any.
    if ($execperms > 0) {
      $perms = fileperms($dest) | $execperms;
      chmod($dest, $perms);
    }
  }

  return TRUE;
}

/**
 * Move $src to $dest.
 *
 * If the php 'rename' function doesn't work, then we'll do copy & delete.
 *
 * @param $src
 *   The directory to move.
 * @param $dest
 *   The destination to move the source to, including the new name of
 *   the directory.  To move directory "a" from "/b" to "/c", then
 *   $src = "/b/a" and $dest = "/c/a".  To move "a" to "/c" and rename
 *   it to "d", then $dest = "/c/d" (just like php rename function).
 * @param $overwrite
 *   If TRUE, the destination will be deleted if it exists.
 * @return
 *   TRUE on success, FALSE on failure.
 */
function mash_move_dir($src, $dest, $overwrite = FALSE) {
  // Preflight based on $overwrite if $dest exists.
  if (file_exists($dest)) {
    if ($overwrite) {
      mash_op('mash_delete_dir', $dest, TRUE);
    }
    else {
      return mash_set_error('MASH_DESTINATION_EXISTS', dt('Destination directory !dest already exists.', array('!dest' => $dest)));
    }
  }
  // $src readable?
  if (!mash_op('is_readable', $src)) {
    return mash_set_error('MASH_SOURCE_NOT_EXISTS', dt('Source directory !src is not readable or does not exist.', array('!src' => $src)));
  }
  // $dest writable?
  if (!mash_op('is_writable', dirname($dest))) {
    return mash_set_error('MASH_DESTINATION_NOT_WRITABLE', dt('Destination directory !dest is not writable.', array('!dest' => dirname($dest))));
  }
  // Try rename. It will fail if $src and $dest are not in the same partition.
  if (@mash_op('rename', $src, $dest)) {
    return TRUE;
  }
  // Eventually it will create an empty file in $dest. See
  // http://www.php.net/manual/es/function.rename.php#90025
  elseif (is_file($dest)) {
    mash_op('unlink', $dest);
  }

  // If 'rename' fails, then we will use copy followed
  // by a delete of the source.
  if (mash_copy_dir($src, $dest)) {
    mash_op('mash_delete_dir', $src, TRUE);
    return TRUE;
  }

  return mash_set_error('MASH_MOVE_DIR_FAILURE', dt('Unable to move !src to !dest.', array('!src' => $src, '!dest' => $dest)));
}

/**
 * Cross-platform compatible helper function to recursively create a directory tree.
 *
 * @param path
 *   Path to directory to create.
 * @param required
 *   If TRUE, then mash_mkdir will call mash_set_error on failure.
 *
 * Callers should *always* do their own error handling after calling mash_mkdir.
 * If $required is FALSE, then a different location should be selected, and a final
 * error message should be displayed if no usable locations can be found.
 * @see mash_directory_cache().
 * If $required is TRUE, then the execution of the current command should be
 * halted if the required directory cannot be created.
 */
function mash_mkdir($path, $required = TRUE) {
  if (!is_dir($path)) {
    if (mash_mkdir(dirname($path))) {
      if (@mkdir($path)) {
        return TRUE;
      }
      elseif (is_dir($path) && is_writable($path)) {
        // The directory was created by a concurrent process.
        return TRUE;
      }
      else {
        if (!$required) {
          return FALSE;
        }
        if (is_writable(dirname($path))) {
          return mash_set_error('MASH_CREATE_DIR_FAILURE', dt('Unable to create !dir.', array('!dir' => preg_replace('/\w+\/\.\.\//', '', $path))));
        }
        else {
          return mash_set_error('MASH_PARENT_NOT_WRITABLE', dt('Unable to create !newdir in !dir. Please check directory permissions.', array('!newdir' => basename($path), '!dir' => realpath(dirname($path)))));
        }
      }
    }
    return FALSE;
  }
  else {
    if (!is_writable($path)) {
      if (!$required) {
        return FALSE;
      }
      return mash_set_error('MASH_DESTINATION_NOT_WRITABLE', dt('Directory !dir exists, but is not writable. Please check directory permissions.', array('!dir' => realpath($path))));
    }
    return TRUE;
  }
}

/**
 * Save a string to a temporary file. Does not depend on Magento's API.
 * The temporary file will be automatically deleted when mash exits.
 *
 * @param string $data
 * @param string $suffix
 *   Append string to filename. use of this parameter if is discouraged. @see
 *   mash_tempnam().
 * @return string
 *   A path to the file.
 */
function mash_save_data_to_temp_file($data, $suffix = NULL) {
  static $fp;

  $file = mash_tempnam('mash_', NULL, $suffix);
  $fp = fopen($file, "w");
  fwrite($fp, $data);
  $meta_data = stream_get_meta_data($fp);
  $file = $meta_data['uri'];
  fclose($fp);

  return $file;
}

/**
 * Returns the path to a temporary directory.
 *
 * This is a custom version of Magento's file_directory_path().
 * We can't directly rely on sys_get_temp_dir() as this
 * path is not valid in some setups for Mac, and we want to honor
 * an environment variable (used by tests).
 */
function mash_find_tmp() {
  static $temporary_directory;

  if (!isset($temporary_directory)) {
    $directories = array();

    // Operating system specific dirs.
    if (mash_is_windows()) {
      // get user specific and operating system temp folders from system environment variables
      // see http://www.microsoft.com/resources/documentation/windows/xp/all/proddocs/en-us/ntcmds_shelloverview.mspx?mfr=true
      $tempdir = getenv('TEMP');
      if (isset($tempdir)) {
        $directories[] = $tempdir;
      }
      $tmpdir = getenv('TMP');
      if (isset($tmpdir)) {
        $directories[] = $tmpdir;
      }
      $windir = getenv('WINDIR');
      if (isset($windir)) {
        // WINDIR itself is not writable, but it always contains a /Temp dir,
        // which is the system-wide temporary directory on older versions. Newer
        // versions only allow system processes to use it.
        $directories[] = $windir . '/Temp';
      }
    }
    else {
      $directories[] = '/tmp';
    }
    $directories[] = sys_get_temp_dir();

    foreach ($directories as $directory) {
      if (is_dir($directory) && is_writable($directory)) {
        $temporary_directory = $directory;
        break;
      }
    }

    if (empty($temporary_directory)) {
      // If no directory has been found, create one in cwd.
      $temporary_directory = mash_cwd() . '/tmp';
      mash_mkdir($temporary_directory, TRUE);
      if (!is_dir($directory)) {
        return mash_set_error('MASH_UNABLE_TO_CREATE_TMP_DIR', dt("Unable to create a temporary directory."));
      }
      mash_register_file_for_deletion($temporary_directory);
    }
  }

  return $temporary_directory;
}

/**
 * Creates a temporary file, and registers it so that
 * it will be deleted when mash exits.  Whenever possible,
 * mash_save_data_to_temp_file() should be used instead
 * of this function.
 *
 * @param string suffix
 *   Append this suffix to the filename. Use of this parameter is discouraged as
 *   it can break the guarantee of tempname(). See http://www.php.net/manual/en/function.tempnam.php#42052.
 *   Originally added to support Oracle driver.
 */
function mash_tempnam($pattern, $tmp_dir = NULL, $suffix = '') {
  if ($tmp_dir == NULL) {
    $tmp_dir = mash_find_tmp();
  }
  $tmp_file = tempnam($tmp_dir, $pattern);
  mash_register_file_for_deletion($tmp_file);

  return $tmp_file . $suffix;
}

/**
 * Creates a temporary directory and return its path.
 */
function mash_tempdir() {
  $tmp_dir = mash_trim_path(mash_find_tmp());
  $tmp_dir .= '/' . 'mash_tmp_' . uniqid(time() . '_');

  mash_mkdir($tmp_dir);
  mash_register_file_for_deletion($tmp_dir);

  return $tmp_dir;
}

/**
 * Any file passed in to this function will be deleted
 * when mash exits.
 */
function mash_register_file_for_deletion($file = NULL) {
  static $registered_files = array();

  if (isset($file)) {
    if (empty($registered_files)) {
      register_shutdown_function('_mash_delete_registered_files');
    }
    $registered_files[] = $file;
  }

  return $registered_files;
}

/**
 * Delete all of the registered temporary files.
 */
function _mash_delete_registered_files() {
  $files_to_delete = mash_register_file_for_deletion();

  foreach ($files_to_delete as $file) {
    // We'll make sure that the file still exists, just
    // in case someone came along and deleted it, even
    // though they did not need to.
    if (file_exists($file)) {
      if (is_dir($file)) {
        mash_delete_dir($file, TRUE);
      }
      else {
        @chmod($dir, 0777); // Make file writeable
        unlink($file);
      }
    }
  }
}

/**
 * Decide where our backup directory should go
 *
 * @param string $subdir
 *   The name of the desired subdirectory(s) under mash-backups.
 *   Usually a database name.
 */
function mash_preflight_backup_dir($subdir = NULL) {
  $backup_dir = mash_get_context('MASH_BACKUP_DIR', mash_get_option('backup-location'));

  if (empty($backup_dir)) {

    // Try to use db name as subdir if none was provided.
    if (empty($subdir)) {
      $subdir = 'unknown';
      if ($creds = mash_get_context('MASH_DB_CREDENTIALS')) {
        $subdir = $creds['name'];
      }
    }

    // Save the date to be used in the backup directory's path name.
    $date = gmdate('YmdHis', $_SERVER['REQUEST_TIME']);

    $backup_dir = mash_get_option('backup-dir', mash_server_home() . '/' . 'mash-backups');
    $backup_dir = mash_trim_path($backup_dir) . '/' . $subdir . '/' . $date;
    mash_set_context('MASH_BACKUP_DIR', $backup_dir);
  }
  return $backup_dir;
}

/**
 * Prepare a backup directory
 */
function mash_prepare_backup_dir($subdir = NULL) {
  $backup_dir = mash_preflight_backup_dir($subdir);
  $backup_parent = dirname($backup_dir);

  $magento_root = mash_get_context('MASH_MAGENTO_ROOT');
  $magento_root .= '/';

  if ((!empty($magento_root)) && (strpos($backup_parent, $magento_root) === 0)) {
    return mash_set_error('MASH_PM_BACKUP_FAILED', dt('It\'s not allowed to store backups inside the Magento root directory.'));
  }
  if (!file_exists($backup_parent)) {
    if (!mash_mkdir($backup_parent, TRUE)) {
      return mash_set_error('MASH_PM_BACKUP_FAILED', dt('Unable to create backup directory !dir.', array('!dir' => $backup_parent)));
    }
  }
  if (!is_writable($backup_parent)) {
    return mash_set_error('MASH_PM_BACKUP_FAILED', dt('Backup directory !dir is not writable.', array('!dir' => $backup_parent)));
  }

  if (!mash_mkdir($backup_dir, TRUE)) {
    return FALSE;
  }
  return $backup_dir;
}

/**
 * Test to see if a file exists and is not empty
 */
function mash_file_not_empty($file_to_test) {
  if (file_exists($file_to_test)) {
    clearstatcache();
    $stat = stat($file_to_test);
    if ($stat['size'] > 0) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * Finds all files that match a given mask in a given directory.
 * Directories and files beginning with a period are excluded; this
 * prevents hidden files and directories (such as SVN working directories
 * and GIT repositories) from being scanned.
 *
 * @param $dir
 *   The base directory for the scan, without trailing slash.
 * @param $mask
 *   The regular expression of the files to find.
 * @param $nomask
 *   An array of files/directories to ignore.
 * @param $callback
 *   The callback function to call for each match.
 * @param $recurse_max_depth
 *   When TRUE, the directory scan will recurse the entire tree
 *   starting at the provided directory.  When FALSE, only files
 *   in the provided directory are returned.  Integer values
 *   limit the depth of the traversal, with zero being treated
 *   identically to FALSE, and 1 limiting the traversal to the
 *   provided directory and its immediate children only, and so on.
 * @param $key
 *   The key to be used for the returned array of files. Possible
 *   values are "filename", for the path starting with $dir,
 *   "basename", for the basename of the file, and "name" for the name
 *   of the file without an extension.
 * @param $min_depth
 *   Minimum depth of directories to return files from.
 * @param $include_dot_files
 *   If TRUE, files that begin with a '.' will be returned if they
 *   match the provided mask.  If FALSE, files that begin with a '.'
 *   will not be returned, even if they match the provided mask.
 * @param $depth
 *   Current depth of recursion. This parameter is only used internally and should not be passed.
 *
 * @return
 *   An associative array (keyed on the provided key) of objects with
 *   "path", "basename", and "name" members corresponding to the
 *   matching files.
 */
function mash_scan_directory($dir, $mask, $nomask = array('.', '..', 'CVS'), $callback = 0, $recurse_max_depth = TRUE, $key = 'filename', $min_depth = 0, $include_dot_files = FALSE, $depth = 0) {
  $key = (in_array($key, array('filename', 'basename', 'name')) ? $key : 'filename');
  $files = array();

  if (is_string($dir) && is_dir($dir) && $handle = opendir($dir)) {
    while (FALSE !== ($file = readdir($handle))) {
      if (!in_array($file, $nomask) && (($include_dot_files && (!preg_match("/\.\+/",$file))) || ($file[0] != '.'))) {
        if (is_dir("$dir/$file") && (($recurse_max_depth === TRUE) || ($depth < $recurse_max_depth))) {
          // Give priority to files in this folder by merging them in after any subdirectory files.
          $files = array_merge(mash_scan_directory("$dir/$file", $mask, $nomask, $callback, $recurse_max_depth, $key, $min_depth, $include_dot_files, $depth + 1), $files);
        }
        elseif ($depth >= $min_depth && preg_match($mask, $file)) {
          // Always use this match over anything already set in $files with the same $$key.
          $filename = "$dir/$file";
          $basename = basename($file);
          $name = substr($basename, 0, strrpos($basename, '.'));
          $files[$$key] = new stdClass();
          $files[$$key]->filename = $filename;
          $files[$$key]->basename = $basename;
          $files[$$key]->name = $name;
          if ($callback) {
            mash_op($callback, $filename);
          }
        }
      }
    }

    closedir($handle);
  }

  return $files;
}

/**
 * Simple helper function to append data to a given file.
 *
 * @param string $file
 *   The full path to the file to append the data to.
 * @param string $data
 *   The data to append.
 *
 * @return boolean
 *   TRUE on success, FALSE in case of failure to open or write to the file.
 */
function mash_file_append_data($file, $data) {
  if (!$fd = fopen($file, 'a+')) {
    mash_set_error(dt("ERROR: fopen(@file, 'ab') failed", array('@file' => $file)));
    return FALSE;
  }
  if (!fwrite($fd, $data)) {
    mash_set_error(dt("ERROR: fwrite(@file) failed", array('@file' => $file)) . '<pre>' . $data);
    return FALSE;
  }
  return TRUE;
}

/**
 * @} End of "defgroup filesystemfunctions".
 */
