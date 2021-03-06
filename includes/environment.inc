<?php

/**
 * @file
 * Functions used by mash to query the environment and
 * setting the current configuration.
 *
 * Bootstrapping now occurs in bootstrap.inc.
 *
 * @see includes/bootstrap.inc
 */

/**
 * Supported version of Console Table. This is displayed in the manual install help.
 */
define('MASH_TABLE_VERSION', '1.1.3');

/**
 * Base URL for automatic file download of PEAR packages.
 */
define('MASH_PEAR_BASE_URL', 'http://download.pear.php.net/package/');

/**
 * Log PHP errors to the Mash log. This is in effect until Magento's error
 * handler takes over.
 */
function mash_error_handler($errno, $message, $filename, $line, $context) {
  // E_DEPRECATED was added in PHP 5.3. Magento 6 will not fix all the
  // deprecated errors, but suppresses them. So we suppress them as well.
  if (defined('E_DEPRECATED')) {
    $errno = $errno & ~E_DEPRECATED;
  }

  // "error_reporting" is usually set in php.ini, but may be changed by
  // mash_errors_on() and mash_errors_off().
  if ($errno & error_reporting()) {
    // By default we log notices.
    $type = mash_get_option('php-notices', 'notice');

    // Bitmask value that constitutes an error needing to be logged.
    $error = E_ERROR | E_PARSE | E_CORE_ERROR | E_COMPILE_ERROR | E_USER_ERROR;
    if ($errno & $error) {
      $type = 'error';
    }

    // Bitmask value that constitutes a warning being logged.
    $warning = E_WARNING | E_CORE_WARNING | E_COMPILE_WARNING | E_USER_WARNING;
    if ($errno & $warning) {
      $type = 'warning';
    }

    mash_log($message . ' ' . basename($filename) . ':' . $line, $type);

    return TRUE;
  }
}

/*
 * Returns a localizable message about php.ini that
 * varies depending on whether the php_ini_loaded_file()
 * is available or not.
 */
function _mash_php_ini_loaded_file_message() {
  if (function_exists('php_ini_loaded_file')) {
    return dt('Please check your configuration settings in !phpini or in your mash.ini file; see examples/example.mash.ini for details.', array('!phpini' => php_ini_loaded_file()));
  }
  else {
    return dt('Please check your configuration settings in your php.ini file or in your mash.ini file; see examples/example.mash.ini for details.');
  }
}

/**
 * Evalute the environment after an abnormal termination and
 * see if we can determine any configuration settings that the user might
 * want to adjust.
 */
function _mash_postmortem() {
  // Make sure that the memory limit has been bumped up from the minimum default value of 32M.
  $php_memory_limit = mash_memory_limit();
  if (($php_memory_limit > 0) && ($php_memory_limit <= 32*MASH_MAGENTO_KILOBYTE*MASH_MAGENTO_KILOBYTE)) {
    mash_set_error('MASH_MEMORY_LIMIT', dt('Your memory limit is set to !memory_limit; mash needs as much memory to run as Magento.  !php_ini_msg', array('!memory_limit' => $php_memory_limit / (MASH_MAGENTO_KILOBYTE*MASH_MAGENTO_KILOBYTE) . 'M', '!php_ini_msg' => _mash_php_ini_loaded_file_message())));
  }
}

/**
 * Evaluate the environment before command bootstrapping
 * begins.  If the php environment is too restrictive, then
 * notify the user that a setting change is needed and abort.
 */
function _mash_environment_check_php_ini() {
  $ini_checks = array('safe_mode' => '', 'open_basedir' => '', 'disable_functions' => array('exec', 'system'), 'disable_classes' => '');

  // Test to insure that certain php ini restrictions have not been enabled
  $prohibited_list = array();
  foreach ($ini_checks as $prohibited_mode => $disallowed_value) {
    $ini_value = ini_get($prohibited_mode);
    $invalid_value = FALSE;
    if (empty($disallowed_value)) {
      $invalid_value = !empty($ini_value);
    }
    else {
      foreach ($disallowed_value as $test_value) {
        if (strstr($ini_value, $test_value) !== FALSE) {
          $invalid_value = TRUE;
        }
      }
    }
    if ($invalid_value) {
      $prohibited_list[] = $prohibited_mode;
    }
  }
  if (!empty($prohibited_list)) {
    mash_log(dt('The following restricted PHP modes have non-empty values: !prohibited_list. This configuration is incompatible with mash.  !php_ini_msg', array('!prohibited_list' => implode(' and ', $prohibited_list), '!php_ini_msg' => _mash_php_ini_loaded_file_message())), 'error');
  }

  return TRUE;
}

/*
 * Check for the existence of the specified lib directory, and create if needed.
 */
function mash_environment_lib() {
  $lib = mash_get_option('lib', MASH_BASE_PATH . '/lib');
  // We tell mash_mkdir that $lib is not required, because otherwise it
  // will throw an error if the folder exists but is not writable.  It
  // is okay with us if the $lib dir is not writable by the current
  // user, as it may have already been set up by the user who installed Mash.
  mash_mkdir($lib, FALSE);
  if (!is_dir($lib)) {
    return FALSE;
  }
}

function mash_environment_table_lib() {
  // Try using the PEAR installed version of Console_Table.
  $tablefile = 'Console/Table.php';
  if (@file_get_contents($tablefile, FILE_USE_INCLUDE_PATH) === FALSE) {
    $lib = mash_get_option('lib', MASH_BASE_PATH . '/lib');
    $tablefile = $lib . '/Console_Table-' . MASH_TABLE_VERSION . '/Table.php';
    // If it is not already present, download Console Table.
    if (!mash_file_not_empty($tablefile)) {
      // Attempt to remove the old Console Table file, from the legacy location.
      // TODO: Remove this (and associated .git.ignore) in Mash 6.x.
      $tablefile_legacy = MASH_BASE_PATH . '/includes/table.inc';
      if (mash_file_not_empty($tablefile_legacy)) {
        mash_op('unlink', $tablefile_legacy);
      }

      // Download and extract Console_Table, and confirm success.
      if (mash_lib_fetch(MASH_PEAR_BASE_URL . 'Console_Table-' . MASH_TABLE_VERSION . '.tgz')) {
        // Remove unneccessary package.xml file which ends up in /lib.
        mash_op('unlink', $lib . '/package.xml');
      }
      if (!mash_file_not_empty($tablefile)) {
        return mash_bootstrap_error('MASH_TABLES_LIB_NOT_FOUND', dt("Mash needs a copy of the PEAR Console_Table library in order to function, and the attempt to download this file automatically failed. To continue you will need to download the !version package from http://pear.php.net/package/Console_Table, extract it into !lib directory, such that Table.php exists at !tablefile.", array('!version' => MASH_TABLE_VERSION, '!tablefile' => $tablefile, '!lib' => $lib)));
      }
    }
  }
  require_once $tablefile;
}

/**
 * Returns the current working directory.
 *
 * This is the directory as it was when mash was started, not the
 * directory we are currently in. For that, use getcwd() directly.
 */
function mash_cwd() {
  if ($path = mash_get_context('MASH_OLDCWD')) {
    return $path;
  }
  // We use PWD if available because getcwd() resolves symlinks, which
  // could take us outside of the Magento root, making it impossible to find.
  // $_SERVER['PWD'] isn't set on windows and generates a Notice.
  $path = isset($_SERVER['PWD']) ? $_SERVER['PWD'] : '';
  if (empty($path)) {
    $path = getcwd();
  }

  // Convert windows paths.
  $path = _mash_convert_path($path);

  // Save original working dir case some command wants it.
  mash_set_context('MASH_OLDCWD', $path);

  return $path;
}

/**
 * Converts a Windows path (dir1\dir2\dir3) into a Unix path (dir1/dir2/dir3).
 * Also converts a cygwin "drive emulation" path (/cygdrive/c/dir1) into a
 * proper drive path, still with Unix slashes (c:/dir1).
 */
function _mash_convert_path($path) {
  $path = str_replace('\\','/', $path);
  $path = preg_replace('/^\/cygdrive\/([A-Za-z])(.*)$/', '\1:\2', $path);

  return $path;
}

/**
 * Returns parent directory.
 *
 * @param string
 *   Path to start from.
 *
 * @return string
 *   Parent path of given path.
 */
function _mash_shift_path_up($path) {
  if (empty($path)) {
    return FALSE;
  }
  $path = explode('/', $path);
  // Move one directory up.
  array_pop($path);
  return implode('/', $path);
}

/**
 * Like Magento conf_path, but searching from beneath.
 * Allows proper site uri detection in site sub-directories.
 *
 * Essentially looks for a settings.php file.  Mash uses this
 * function to find a usable site based on the user's current
 * working directory.
 *
 * @param string
 *   Search starting path. Defaults to current working directory.
 *
 * @return
 *   Current site path (folder containing settings.php) or FALSE if not found.
 */
function mash_site_path($path = NULL) {
  $site_path = FALSE;

  $path = empty($path) ? mash_cwd() : $path;
  // Check the current path.
  if (file_exists($path . '/settings.php')) {
    $site_path = $path;
  }
  else {
    // Move up dir by dir and check each.
    while ($path = _mash_shift_path_up($path)) {
      if (file_exists($path . '/settings.php')) {
        $site_path = $path;
        break;
      }
    }
  }

  $site_root = mash_get_context('MASH_SELECTED_MAGENTO_ROOT');
  if (file_exists($site_root . '/sites/sites.php')) {
    $sites = array();
    // This will overwrite $sites with the desired mappings.
    include($site_root . '/sites/sites.php');
    // We do a reverse lookup here to determine the URL given the site key.
    if ($match = array_search($site_path, $sites)) {
      $site_path = $match;
    }
  }

  // Last resort: try from site root
  if (!$site_path) {
    if ($site_root) {
      if (file_exists($site_root . '/sites/default/settings.php')) {
        $site_path = $site_root . '/sites/default';
      }
    }
  }

  return $site_path;
}

/**
 * This is a copy of Magento's conf_path function, taken from D7 and
 * adjusted slightly to search from the selected Magento Root.
 *
 * Mash uses this routine to find a usable site based on a URI
 * passed in via a site alias record or the --uri commandline option.
 *
 * Mash uses Magento itself (specifically, the Magento conf_path function)
 * to bootstrap the site itself.  If the implementation of conf_path
 * changes, the site should still bootstrap correctly; the only consequence
 * of this routine not working is that mash configuration files
 * (mashrc.php) stored with the site's settimight not be found.
 */
function mash_conf_path($server_uri, $require_settings = TRUE) {
  $magento_root = mash_get_context('MASH_SELECTED_MAGENTO_ROOT');
  if(empty($magento_root) || empty($server_uri)) {
    return NULL;
  }
  $parsed_uri = parse_url($server_uri);
  if (is_array($parsed_uri) && !array_key_exists('scheme', $parsed_uri)) {
    $parsed_uri = parse_url('http://' . $server_uri);
  }
  if (!is_array($parsed_uri)) {
    return NULL;
  }
  $server_host = $parsed_uri['host'];
  if (array_key_exists('path', $parsed_uri)) {
    $server_uri = $parsed_uri['path'] . '/index.php';
  }
  else {
    $server_uri = "/index.php";
  }
  $confdir = 'sites';

  $sites = array();
  if (file_exists($magento_root . '/' . $confdir . '/sites.php')) {
    // This will overwrite $sites with the desired mappings.
    include($magento_root . '/' . $confdir . '/sites.php');
  }

  $uri = explode('/', $server_uri);
  $server = explode('.', implode('.', array_reverse(explode(':', rtrim($server_host, '.')))));
  for ($i = count($uri) - 1; $i > 0; $i--) {
    for ($j = count($server); $j > 0; $j--) {
      $dir = implode('.', array_slice($server, -$j)) . implode('.', array_slice($uri, 0, $i));
      if (isset($sites[$dir]) && file_exists($magento_root . '/' . $confdir . '/' . $sites[$dir])) {
        $dir = $sites[$dir];
      }
      if (file_exists($magento_root . '/' . $confdir . '/' . $dir . '/settings.php') || (!$require_settings && file_exists(MAGENTO_ROOT . '/' . $confdir . '/' . $dir))) {
        $conf = "$confdir/$dir";
        return $conf;
      }
    }
  }
  $conf = "$confdir/default";
  return $conf;
}

/**
 * Exhaustive depth-first search to try and locate the Magento root directory.
 * This makes it possible to run mash from a subdirectory of the magento root.
 *
 * @param
 *   Search start path. Defaults to current working directory.
 * @return
 *   A path to magento root, or FALSE if not found.
 */
function mash_locate_root($start_path = NULL) {
  $magento_root = FALSE;

  $start_path = empty($start_path) ? mash_cwd() : $start_path;
  foreach (array(TRUE, FALSE) as $follow_symlinks) {
    $path = $start_path;
    if ($follow_symlinks && is_link($path)) {
      $path = realpath($path);
    }
    // Check the start path.
    if (mash_valid_magento_root($path)) {
      $magento_root = $path;
      break;
    }
    else {
      // Move up dir by dir and check each.
      while ($path = _mash_shift_path_up($path)) {
        if ($follow_symlinks && is_link($path)) {
          $path = realpath($path);
        }
        if (mash_valid_magento_root($path)) {
          $magento_root = $path;
          break 2;
        }
      }
    }
  }

  return $magento_root;
}

/**
 * Checks whether given path qualifies as a Magento root.
 *
 * @param string
 *   Path to check.
 *
 * @return string
 *   The relative path to common.inc (varies by Magento version), or FALSE if not
 *   a Magento root.
 */
function mash_valid_magento_root($path) {
  if (!empty($path) && is_dir($path)) {
    $candidates = array('var/package/Mage_All_Latest-*.xml');
    foreach ($candidates as $candidate) {
      if (file_exists($path . '/' . $candidate)) {
        return $candidate;
      }else{
        foreach(glob($candidate) as $glob_candidate){
          return $glob_candidate;
        }
      }
    }
  }
  return FALSE;
}

/**
 * Tests the currently loaded database credentials to ensure a database connection can be made.
 */
function mash_valid_db_credentials() {
  return Mage::isInstalled();
// TODO
//  $creds = mash_get_context('MASH_DB_CREDENTIALS');
//  // Do minimal checking that we have the necessary information.
//  if (count($creds) == 0) {
//    return FALSE;
//  }
//  $type = $creds['driver'];
//
//  switch (mash_magento_major_version()) {
//    case 6:
//      // Check availability of db extension in PHP and also Magento support.
//      if (file_exists('./includes/install.'. $type .'.inc')) {
//        require_once './includes/install.'. $type .'.inc';
//        $function = $type .'_is_available';
//        if (!$function()) {
//          mash_log(dt('!type extension for PHP is not installed. Check your php.ini to see how you can enable it.', array('!type' => $type)), 'bootstrap');
//          return FALSE;
//        }
//      }
//      else {
//        mash_log(dt('!type database type is unsupported.', array('!type' => $type)), 'bootstrap');
//        return FALSE;
//      }
//      // Verify connection settings.
//      switch ($type) {
//        case 'mysql':
//          $hostspec = $creds['port'] ? $creds['host'] . ':' . $creds['port'] : $creds['host'];
//          $connection = @mysql_connect($hostspec, $creds['user'], $creds['pass']);
//          if (!$connection || !mysql_select_db($creds['name'])) {
//            mash_log(mysql_error(), 'bootstrap');
//            return FALSE;
//          }
//          break;
//        case 'mysqli':
//          $connection = mysqli_init();
//          @mysqli_real_connect($connection, $creds['host'], $creds['user'], $creds['pass'], $creds['name'], (int)$creds['port']);
//          if (mysqli_connect_errno() > 0) {
//            mash_log(mysqli_connect_error(), 'bootstrap');
//            return FALSE;
//          }
//          break;
//        case 'pgsql':
//          $conn_string = sprintf("host=%s user=%s password=%s dbname=%s", $creds['host'], $creds['user'], $creds['pass'], $creds['name']);
//          if (isset($creds['port'])) {
//            $conn_string .= ' port=' . $creds['port'];
//          }
//          // Copied from d6's database.pgsql.inc:
//          // pg_last_error() does not return a useful error message for database
//          // connection errors. We must turn on error tracking to get at a good error
//          // message, which will be stored in $php_errormsg.
//          $php_errormsg = '';
//          $track_errors_previous = ini_get('track_errors');
//          ini_set('track_errors', 1);
//          $connection = @pg_connect($conn_string);
//          // Restore error tracking setting
//          ini_set('track_errors', $track_errors_previous);
//          if (!$connection) {
//            if (empty($php_errormsg)) {
//              mash_log(dt("Unknown error connecting to pgsql database via !constr", array('!constr' => $conn_string)), 'bootstrap');
//            }
//            else {
//              require_once './includes/unicode.inc';
//              mash_log(decode_entities($php_errormsg), 'bootstrap');
//            }
//            return FALSE;
//          }
//          break;
//      }
//      break;
//    case 7:
//    default:
//      $type = ( $type=='oracle' ? 'oci' : $type); // fix PDO driver name, should go away in Magento 8
//      // Magento >=7 requires PDO and mash requires php 5.3 which ships with PDO
//      // but it may be compiled with --disable-pdo.
//      if (!class_exists('PDO')) {
//        mash_log(dt('PDO support is required.'), 'bootstrap');
//        return FALSE;
//      }
//      // Check the database specific driver is available.
//      if (!in_array($type, PDO::getAvailableDrivers())) {
//        mash_log(dt('!type extension for PHP PDO is not installed. Check your php.ini to see how you can enable it.', array('!type' => $type)), 'bootstrap');
//        return FALSE;
//      }
//      // Build the connection string.
//      if ($type === 'sqlite') {
//        $constr = 'sqlite:' . $creds['name'];
//      }
//      elseif ($type === 'sqlsrv') {
//        $server = $creds['host'];
//        if (!empty($creds['port'])) {
//          $server .= ", " . $creds['port'];
//        }
//        $constr = sprintf("%s:database=%s;server=%s", $type, $creds['name'], $server);
//      }
//      elseif ($type === 'oci') {
//
//        if (empty($creds['port']))
//          $creds['port'] = 1521;
//
//        if ($creds['host'] == 'USETNS')
//          $constr = 'oci:dbname=' . $creds['database'] . ';charset=AL32UTF8';
//        else
//          $constr = 'oci:dbname=//' . $creds['host'] . ':' . $creds['port'] . '/' . $creds['database'] . ';charset=AL32UTF8';
//
//      }
//      else {
//        $constr = sprintf("%s:dbname=%s", $type, $creds['name']);
//        // Use unix_socket if set instead of host:port.
//        if (!empty($creds['unix_socket'])) {
//          $constr .= sprintf(";unix_socket=%s", $creds['unix_socket']);
//        }
//        else {
//          $constr .= sprintf(";host=%s", $creds['host']);
//          if (!empty($creds['port'])) {
//            $constr .= sprintf(";port=%d", $creds['port']);
//          }
//        }
//      }
//      try {
//        $db = new PDO($constr, $creds['user'], $creds['pass']);
//        $db = null;
//      }
//      catch (PDOException $e) {
//        mash_log($e->getMessage(), 'bootstrap');
//        return FALSE;
//      }
//      break;
//  }

  return TRUE;
}

/**
 * Determine a proper way to call mash again
 *
 * This check if we were called directly or as an argument to some
 * wrapper command (php and sudo are checked now).
 *
 * Calling ./mash.php directly yields the following environment:
 *
 * _SERVER["argv"][0] => ./mash.php
 *
 * Calling php ./mash.php also yields the following:
 *
 * _SERVER["argv"][0] => ./mash.php
 *
 * Note that the $_ global is defined only in bash and therefore cannot
 * be relied upon.
 *
 * The MASH_COMMAND constant is initialised to the value of this
 * function when environment.inc is loaded.
 *
 * @see MASH_COMMAND
 */
function mash_find_mash() {
  $mash = realpath($_SERVER['argv']['0']);
  // TODO: On Windows, if we leave $mash as-is, then callbacks will
  // be done just as we were called by the batch file:  php.exe C:\path\mash.php
  // We could also convert mash.php to mash.bat to run the batch file again,
  // but this works just as well.
  return $mash;
}

/**
 * Verify that we are running PHP through the command line interface.
 *
 * This function is useful for making sure that code cannot be run via the web server,
 * such as a function that needs to write files to which the web server should not have
 * access to.
 *
 * @return
 *   A boolean value that is true when PHP is being run through the command line,
 *   and false if being run through cgi or mod_php.
 */
function mash_verify_cli() {
  return (php_sapi_name() == 'cli' || (is_numeric($_SERVER['argc']) && $_SERVER['argc'] > 0));
}

/**
 * Build a mash command suitable for use for mash to call itself
 * e.g. in backend_invoke.
 */
function mash_build_mash_command($mash_path = NULL, $php = NULL, $os = NULL, $remote_command = FALSE) {
  $os = _mash_get_os($os);
  $additional_options = '';
  if (is_null($mash_path)) {
    if (!$remote_command) {
      $mash_path = MASH_COMMAND;
    }
    else {
      $mash_path = mash_is_windows($os) ? 'mash.bat' : 'mash';
    }
  }
  // If the path to mash points to mash.php, then we will need to
  // run it via php rather than direct execution.  By default, we
  // will use 'php' unless something more specific was passed in
  // via the --php flag.
  if (substr($mash_path, -4) == ".php") {
    if (!isset($php)) {
      $php = mash_get_option('php');
      if (!isset($php)) {
        $php = 'php';
      }
    }
    if (isset($php) && ($php != "php")) {
      $additional_options .= ' --php=' . mash_escapeshellarg($php, $os);
    }
    // We will also add in the php options from --php-options
    $php = mash_escapeshellarg($php, $os) . ' ';
    $php_options = mash_get_option('php-options','');
    if (!empty($php_options)) {
      $php .= $php_options . ' ';
      $additional_options .= ' --php-options=' . mash_escapeshellarg($php_options, $os);
    }
  }
  else {
    $php = '';
  }
  return $php . mash_escapeshellarg($mash_path, $os) . $additional_options;
}

/**
 * Check if the operating system is Windows.
 * This will return TRUE under DOS, Powershell
 * Cygwin and MSYSGIT shells, so test for the
 * Windows variant FIRST if you care.
 */
function mash_is_windows($os = NULL) {
  return _mash_test_os($os, array("WIN","CYGWIN","CWRSYNC","MINGW"));
}

/**
 * Check if the operating system is Winodws
 * running some variant of cygwin -- either
 * Cygwin or the MSYSGIT shell.  If you care
 * which is which, test mingw first.
 */
function mash_is_cygwin($os = NULL) {
  return _mash_test_os($os, array("CYGWIN","CWRSYNC","MINGW"));
}

function mash_is_mingw($os = NULL) {
  return _mash_test_os($os, array("MINGW"));
}

/*
 * Return tar executable name specific for the current OS
 */
function mash_get_tar_executable() {
  return mash_is_windows() ? "bsdtar.exe" : "tar";
}

/**
 * Checks if the operating system has bash.
 */
function mash_has_bash($os = NULL) {
  return mash_is_cygwin($os) || !mash_is_windows($os);
}

/*
 * Checks operating system and returns
 * supported bit bucket folder.
 */
function mash_bit_bucket() {
  if (mash_has_bash()) {
    return '/dev/null';
  }
  else {
    return 'nul';
  }
}

/**
 * Return the OS we are running under.
 *
 * @return string
 *   Linux
 *   WIN* (e.g. WINNT)
 *   CYGWIN
 *   MINGW* (e.g. MINGW32)
 */
function _mash_get_os($os = NULL) {
  // The special os "CWRSYNC" can be used to indicate that we are testing
  // a path that will be passed as an argument to cwRsync, which requires
  // that the path be converted to /cygdrive/c/path, even on DOS or Powershell.
  // The special os "RSYNC" can be used to indicate that we want to assume
  // "CWRSYNC" when cwrsync is installed, or default to the local OS otherwise.
  if (strtoupper($os) == "RSYNC") {
    $os = _mash_get_os("LOCAL");
    // For now we assume that cwrsync is always installed on Windows, and never installed son any other platform.
    return mash_is_windows($os) ? "CWRSYNC" : $os;
  }
  // We allow "LOCAL" to document, in instances where some parameters are being escaped
  // for use on a remote machine, that one particular parameter will always be used on
  // the local machine (c.f. mash_backend_invoke).
  if (isset($os) && ($os != "LOCAL")) {
    return $os;
  }
  if (_mash_test_os(getenv("MSYSTEM"), array("MINGW"))) {
    return getenv("MSYSTEM");
  }
  // QUESTION: Can we differentiate between DOS and POWERSHELL? They appear to have the same environment.
  // At the moment, it does not seem to matter; they behave the same from PHP.
  // At this point we will just return PHP_OS.
  return PHP_OS;
}

function _mash_test_os($os, $os_list_to_check) {
  $os = _mash_get_os($os);
  foreach ($os_list_to_check as $test) {
    if (strtoupper(substr($os, 0, strlen($test))) == strtoupper($test)) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * Read the mash info file.
 */
function mash_read_mash_info() {
  $mash_info_file = dirname(__FILE__) . '/../mash.info';

  return parse_ini_file($mash_info_file);
}

/**
 * Make a determination whether or not the given
 * host is local or not.
 *
 * @param host
 *   A hostname, 'localhost' or '127.0.0.1'.
 * @return
 *   True if the host is local.
 */
function mash_is_local_host($host) {
  // In order for this to work right, you must use 'localhost' or '127.0.0.1'
  // or the machine returned by 'uname -n' for your 'remote-host' entry in
  // your site alias.
  if (($host == 'localhost') || ($host == '127.0.0.1')) {
    return TRUE;
  }

  // We'll start out by asking for uname -n via php_uname, since
  // that is most portable.  On Linux, uname -n returns the contents of
  // /etc/hostname, which, according to `man hostname`, should never
  // contain the fqdn.  We will therefore also try `hostname -f`, and
  // see if we can get the fqdn from that.
  $uname = php_uname('n');
  if ((strpos($uname,'.') === FALSE) && (!mash_is_windows())) {
    $hostname = exec('hostname -f');
    if (!empty($hostname)) {
      $uname = $hostname;
    }
  }
  // We will require an exact match if we have the fqdn for this
  // host; if we only have the short name, then we will consider it
  // to be a match if it matches the first part of the remote-host
  // item (up to the first dot).
  return (strpos($uname,'.') !== FALSE) ? ($host == $uname) : substr($host . '.',0,strlen($uname)+1) == $uname . '.';
}

/**
 * Return the user's home directory.
 */
function mash_server_home() {
  // Cannot use $_SERVER superglobal since that's empty during Mash_UnitTestCase
  // getenv('HOME') isn't set on windows and generates a Notice.
  $home = getenv('HOME');
  if (empty($home)) {
    if (!empty($_SERVER['HOMEDRIVE']) && !empty($_SERVER['HOMEPATH'])) {
      // home on windows
      $home = $_SERVER['HOMEDRIVE'] . $_SERVER['HOMEPATH'];
    }
  }
  return empty($home) ? NULL : $home;
}

/*
 * The path to the global cache directory.
 *
 * @param subdir
 *   Return the specified subdirectory inside the global
 *   cache directory instead.  The subdirectory is created.
 */
function mash_directory_cache($subdir = '') {
  $cache_locations = array();
  if (getenv('CACHE_PREFIX')) {
    $cache_locations[getenv('CACHE_PREFIX')] = 'cache';
  }
  $home = mash_server_home();
  if ($home) {
    $cache_locations[$home] = '.mash/cache';
  }
  $cache_locations[mash_find_tmp()] = 'mash/' . getenv('USER') . '/cache';
  foreach ($cache_locations as $base => $dir) {
    if (!empty($base) && is_writable($base)) {
      $cache_dir = $base . '/' . $dir;
      if (!empty($subdir)) {
        $cache_dir .= '/' . $subdir;
      }
      if (mash_mkdir($cache_dir)) {
        return $cache_dir;
      }
      else {
        // If the base directory is writable, but the cache directory
        // is not, then we will get an error. The error will be displayed,
        // but we will still call mash_clear_error so that we can go
        // on and try the next location to see if we can find a cache
        // directory somewhere.
        mash_clear_error();
      }
    }
  }
  return mash_set_error('MASH_NO_WRITABLE_CACHE', dt('Mash must have a writable cache directory; please insure that one of the following locations is writable: @locations',
    array('@locations' => implode(',', array_keys($cache_locations)))));
}

/**
 * Get complete information for all available extensions (modules and themes).
 *
 * @return
 *   An array containing info for all available extensions.
 */
function mash_get_extensions($include_hidden = TRUE) {
  mash_include_engine('magento', 'environment');
  $extensions = array_merge(mash_get_modules($include_hidden), mash_get_themes($include_hidden));
  foreach ($extensions as $name => $extension) {
    $extensions[$name]->label = $extension->info['name'].' ('.$extension->name.')';
    if (empty($extension->info['package'])) {
      $extensions[$name]->info['package'] = dt('Other');
    }
  }
  return $extensions;
}

/**
 * Test compatibility of a extension with version of magento core and php.
 *
 * @param $file Extension object as returned by system_rebuild_module_data().
 * @return TRUE if the extension is incompatible.
 */
function mash_extension_check_incompatibility($file) {
  if (!isset($file->info['core'])
       || $file->info['core'] != MAGENTO_CORE_COMPATIBILITY
       || version_compare(phpversion(), $file->info['php']) < 0) {
    return TRUE;
  }

  return FALSE;
}

/**
 *
 */
function mash_magento_required_modules($modules) {
  mash_include_engine('magento', 'environment');
  return _mash_magento_required_modules($modules);
}

/**
 * Return the default theme.
 *
 * @return
 *  Machine name of the default theme.
 */
function mash_theme_get_default() {
  return variable_get('theme_default', 'garland');
}

/**
 * Return the administration theme.
 *
 * @return
 *  Machine name of the administration theme.
 */
function mash_theme_get_admin() {
  return variable_get('admin_theme', mash_theme_get_default());
}
