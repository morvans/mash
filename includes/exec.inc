<?php

/*
 * @file
 *   Functions for executing system commands. (e.g. exec(), system(), ...).
 */

/**
 * @defgroup commandwrappers Functions to execute commands.
 * @{
 */

/**
 * Calls 'system()' function, passing through all arguments unchanged.
 *
 * This should be used when calling possibly mutative or destructive functions
 * (e.g. unlink() and other file system functions) so that can be suppressed
 * if the simulation mode is enabled.
 *
 * @param $exec
 *   The shell command to execute.  Parameters should already be escaped.
 * @return
 *   The result code from system():  0 == success.
 *
 * @see mash_shell_exec()
 */
function mash_op_system($exec) {
  if (mash_get_context('MASH_VERBOSE') || mash_get_context('MASH_SIMULATE')) {
    mash_print("Calling system($exec);", 0, STDERR);
  }
  if (mash_get_context('MASH_SIMULATE')) {
    return 0;
  }

  // Throw away output.  Use mash_shell_exec() to capture output.
  system($exec, $result_code);

  return $result_code;
}

/**
 * Executes a shell command at a new working directory.
 * The old cwd is restored on exit.
 *
 * @param $effective_wd
 *   The new working directory to execute the shell command at.
 * @param $cmd
 *   The command to execute. May include placeholders used for sprintf.
 * @param ...
 *   Values for the placeholders specified in $cmd. Each of these will be passed through escapeshellarg() to ensure they are safe to use on the command line.
 * @return
 *   TRUE on success, FALSE on failure
 */
function mash_shell_cd_and_exec($effective_wd, $cmd) {
  $args = func_get_args();

  $effective_wd = array_shift($args);
  $cwd = getcwd();
  mash_op('chdir', $effective_wd);
  $result = call_user_func_array('mash_shell_exec', $args);
  mash_op('chdir', $cwd);
  return $result;
}

/**
 * Executes a shell command.
 * Output is only printed if in verbose mode.
 * Output is stored and can be retrieved using mash_shell_exec_output().
 * If in simulation mode, no action is taken.
 *
 * @param $cmd
 *   The command to execute. May include placeholders used for sprintf.
 * @param ...
 *   Values for the placeholders specified in $cmd. Each of these will be passed through escapeshellarg() to ensure they are safe to use on the command line.
 * @return
 *   TRUE on success, FALSE on failure
 */
function mash_shell_exec($cmd) {
  return _mash_shell_exec(func_get_args());
}

/**
 * Executes a command in interactive mode.
 *
 * @see mash_shell_exec.
 */
function mash_shell_exec_interactive($cmd) {
  return _mash_shell_exec(func_get_args(), TRUE);
}

/**
 * Internal function: executes a shell command on the
 * local machine.  This function should not be used
 * in instances where ssh is utilized to execute a
 * command remotely; otherwise, remote operations would
 * fail if executed from a Windows machine to a remote
 * Linux server.
 *
 * @param $args
 *   The command and its arguments.
 * @param $interactive
 *   Whether to run in
 *
 * @return
 *   TRUE on success, FALSE on failure
 *
 * @see mash_shell_exec.
 */
function _mash_shell_exec($args, $interactive = FALSE) {
  // Do not change the command itself, just the parameters.
  for ($x = 1; $x < sizeof($args); $x++) {
    $args[$x] = mash_escapeshellarg($args[$x]);
  }
  // Important: we allow $args to take one of two forms here.  If
  // there is only one item in the array, it is the already-escaped
  // command string, but otherwise sprintf is used.  In the case
  // of pre-escaped strings, sprintf will fail if any of the escaped
  // parameters contain '%', so we must not call sprintf unless necessary.
  if (count($args) == 1) {
    $command = $args[0];
  }
  else {
    $command = call_user_func_array('sprintf', $args);
  }

  if (mash_get_context('MASH_VERBOSE') || mash_get_context('MASH_SIMULATE')) {
    mash_print('Executing: ' . $command, 0, STDERR);
  }
  if (!mash_get_context('MASH_SIMULATE')) {
    if ($interactive) {
      $result = mash_shell_proc_open($command);
      return ($result == 0) ? TRUE : FALSE;
    }
    else {
      exec($command . ' 2>&1', $output, $result);
      _mash_shell_exec_output_set($output);

      if (mash_get_context('MASH_DEBUG')) {
        foreach ($output as $line) {
          mash_print($line, 2);
        }
      }

      // Exit code 0 means success.
      return ($result == 0);
    }
  }
  else {
    return TRUE;
  }
}

/**
 * Build an SSH string including an optional fragment of bash. Commands that use
 * this should also merge mash_shell_proc_build_options() into their
 * command options. @see ssh_mash_command().
 *
 * @param array $site
 *   A site alias record.
 * @param string $command
 *   An optional bash fragment.
 * @param string $cd
 *   An optional directory to change into before executing the $command. Set to
 *   boolean TRUE to change into $site['root'] if available.
 * @return string
 *   A string suitable for execution with mash_shell_remote_exec().
 */
function mash_shell_proc_build($site, $command = '', $cd = NULL) {
  // Build up the command. TODO: We maybe refactor this soon.
  $hostname = mash_remote_host($site);
  $ssh_options = mash_sitealias_get_option($site, 'ssh-options', "-o PasswordAuthentication=no");
  $os = mash_os($site);
  if (mash_sitealias_get_option($site, 'tty')) {
    $ssh_options .= ' -t';
  }

  $cmd = "ssh " . $ssh_options . " " . $hostname;

  if ($cd === TRUE) {
    if (array_key_exists('root', $site)) {
      $cd = $site['root'];
    }
    else {
      $cd = FALSE;
    }
  }
  if ($cd) {
    $command = 'cd ' . mash_escapeshellarg($cd, $os) . ' && ' . $command;
  }

  if (!empty($command)) {
    if (!mash_get_option('escaped', FALSE)) {
      $cmd .= " " . mash_escapeshellarg($command, $os);
    }
    else {
      $cmd .= " $command";
    }
  }

  return $cmd;
}

/*
 * Execute bash command using proc_open().
 *
 * @returns
 *   Exit code from launched application
 *     0 no error
 *     1 general error
 *     127 command not found
 */
function mash_shell_proc_open($cmd) {
  if (mash_get_context('MASH_VERBOSE') || mash_get_context('MASH_SIMULATE')) {
    mash_print("Calling proc_open($cmd);", 0, STDERR);
  }
  if (!mash_get_context('MASH_SIMULATE')) {
    $process = proc_open($cmd, array(0 => STDIN, 1 => STDOUT, 2 => STDERR), $pipes);
    $proc_status = proc_get_status($process);
    $exit_code = proc_close($process);
    return ($proc_status["running"] ? $exit_code : $proc_status["exitcode"] );
  }
  return 0;
}

/**
 * Used by definition of ssh and other commands that call into mash_shell_proc_build()
 * to declare their options.
 */
function mash_shell_exec_proc_build_options() {
  return array(
   'ssh-options' => 'A string of extra options that will be passed to the ssh command (e.g. "-p 100")',
    'tty' => 'Create a tty (e.g. to run an interactive program).',
    'escaped' => 'Command string already escaped; do not add additional quoting.',
  );
}

/**
 * Determine the appropriate os value for the
 * specified site record
 *
 * @returns
 *   NULL for 'same as local machine', 'Windows' or 'Linux'.
 */
function mash_os($site_record = NULL) {
  // Default to $os = NULL, meaning 'same as local machine'
  $os = NULL;
  // If the site record has an 'os' element, use it
  if (isset($site_record) && array_key_exists('os', $site_record)) {
    $os = $site_record['os'];
  }
  // Otherwise, we will assume that all remote machines are Linux
  // (or whatever value 'remote-os' is set to in mashrc.php).
  elseif (isset($site_record) && array_key_exists('remote-host', $site_record) && !empty($site_record['remote-host'])) {
    $os = mash_get_option('remote-os', 'Linux');
  }

  return $os;
}

/**
 * Determine the remote host (username@hostname.tld) for
 * the specified site.
 */
function mash_remote_host($site, $prefix = '') {
  $hostname = mash_escapeshellarg(mash_sitealias_get_option($site, 'remote-host', '', $prefix), "LOCAL");
  $username = mash_escapeshellarg(mash_sitealias_get_option($site, 'remote-user', '', $prefix), "LOCAL");
  return $username . (empty($username) ? '' : '@') . $hostname;
}

/**
 * Make an attempt to simply wrap the arg with the
 * kind of quote characters it does not already contain.
 * If it contains both kinds, or if it contains no quote
 * characters, then this function reverts to mash_escapeshellarg.
 *
 * Note that this routine is only useful in certain very
 * specific circumstances (e.g. core-cli), as in general,
 * Windows -must- use double-quotes to escape a shell arg.
 */
function mash_wrap_with_quotes($arg) {
  $has_double = strpos($arg, '"') !== FALSE;
  $has_single = strpos($arg, "'") !== FALSE;
  // If there are both kinds of quotes ($has_double == TRUE && $has_single == TRUE)
  // or there are neither kind of quotes ($has_double == FALSE && $has_single == FALSE)
  // then we call mash_escapeshellarg.  The relations above logically reduce to:
  if ($has_double == $has_single) {
    return mash_escapeshellarg($arg);
  }
  elseif ($has_double) {
    return "'" . $arg . "'";
  }
  else {
    return '"' . $arg . '"';
  }
}

/**
 * Platform-dependent version of escapeshellarg().
 * Given the target platform, return an appropriately-escaped
 * string.  The target platform may be omitted for args that
 * are /known/ to be for the local machine.
 */
function mash_escapeshellarg($arg, $os = NULL) {
  // Short-circuit escaping for simple params (keep stuff readable)
  if (preg_match('|^[a-zA-Z0-9.:/_-]*$|', $arg)) {
    return $arg;
  }
  elseif (mash_is_windows($os)) {
    return _mash_escapeshellarg_windows($arg);
  }
  else {
    return _mash_escapeshellarg_linux($arg);
  }
}

/**
 * Windows version of escapeshellarg().
 */
function _mash_escapeshellarg_windows($arg) {
  // Double up existing backslashes
  $arg = preg_replace('/\\\/', '\\\\\\\\', $arg);

  // Double up double quotes
  $arg = preg_replace('/"/', '""', $arg);

  // Double up percents.
  $arg = preg_replace('/%/', '%%', $arg);

  // Add surrounding quotes.
  $arg = '"' . $arg . '"';

  return $arg;
}

/**
 * Linux version of escapeshellarg().
 *
 * This is intended to work the same way that escapeshellarg() does on
 * Linux.  If we need to escape a string that will be used remotely on
 * a Linux system, then we need our own implementation of escapeshellarg,
 * because the Windows version behaves differently.
 */
function _mash_escapeshellarg_linux($arg) {
  // For single quotes existing in the string, we will "exit"
  // single-quote mode, add a \' and then "re-enter"
  // single-quote mode.  The result of this is that
  // 'quote' becomes '\''quote'\''
  $arg = preg_replace('/\'/', '\'\\\'\'', $arg);

  // Replace "\t", "\n", "\r", "\0", "\x0B" with a whitespace.
  // Note that this replacement makes Mash's escapeshellarg work differently
  // than the built-in escapeshellarg in PHP on Linux, as these characters
  // usually are NOT replaced. However, this was done deliberately to be more
  // conservative when running _mash_escapeshellarg_linux on Windows
  // (this can happen when generating a command to run on a remote Linux server.)
  $arg = str_replace(array("\t", "\n", "\r", "\0", "\x0B"), ' ', $arg);

  // Add surrounding quotes.
  $arg = "'" . $arg . "'";

  return $arg;
}

/**
 * Stores output for the most recent shell command.
 * This should only be run from mash_shell_exec().
 *
 * @param $output
 *   The output of the most recent shell command.
 *   If this is not set the stored value will be returned.
 */
function _mash_shell_exec_output_set($output = FALSE) {
  static $stored_output;
  if ($output === FALSE) return $stored_output;
  $stored_output = $output;
}

/**
 * Returns the output of the most recent shell command as an array of lines.
 */
function mash_shell_exec_output() {
  return _mash_shell_exec_output_set();
}

/**
 * Starts a background browser/tab for the current site or a specified URL.
 *
 * Uses a non-blocking proc_open call, so Mash execution will continue.
 *
 * @param $uri
 *   Optional URI or site path to open in browser. If omitted, or if a site path
 *   is specified, the current site home page uri will be prepended if the sites
 *   hostname resolves.
 * @return
 *   TRUE if browser was opened, FALSE if browser was disabled by the user or a,
 *   default browser could not be found.
 */
function mash_start_browser($uri = NULL, $sleep = FALSE) {
  if (!parse_url($uri, PHP_URL_HOST)) {
    $site = mash_get_context('MASH_URI');
    $host = parse_url($site, PHP_URL_HOST);
    // Validate that the host part of the URL resolves, so we don't attempt to
    // open the browser for http://default or similar invalid hosts.
    $hosterror = gethostbyname($host) == $host;
    $iperror = ip2long($host) && gethostbyaddr($host) == $host;
    if ($hosterror && $iperror) {
      mash_log(dt('!host does not appear to be a resolvable hostname or IP, not starting browser.', array('!host' => $host)), 'warning');
      return FALSE;
    }
    $uri = $site . '/' . ltrim($uri, '/');
  }
  if ($browser = mash_get_option('browser', TRUE)) {
    if ($browser === TRUE) {
      // See if we can find an OS helper to open URLs in default browser.
      if (mash_shell_exec('which xdg-open')) {
        $browser = 'xdg-open';
      }
      else if (mash_shell_exec('which open')) {
        $browser = 'open';
      }
      else if (!mash_has_bash()) {
        $browser = 'start';
      }
      else {
        // Can't find a valid browser.
        $browser = FALSE;
      }
    }
    $prefix = '';
    if ($sleep) {
      $prefix = 'sleep ' . $sleep . ' && ';
    }
    if ($browser && !mash_get_context('MASH_SIMULATE')) {
      $pipes = array();
      mash_log(dt('Opening browser at !uri', array('!uri' => $uri)));
      proc_close(proc_open($prefix . $browser . ' ' . mash_escapeshellarg($uri) . ' 2> ' . mash_bit_bucket() . ' &', array(), $pipes));
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * @} End of "defgroup commandwrappers".
 */
