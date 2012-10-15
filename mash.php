#!/usr/bin/env php
<?php

/**
 * @file
 * mash is a PHP script implementing a command line shell for Magento.
 *
 * @requires PHP CLI 5.3.2, or newer.
 */

require(dirname(__FILE__) . '/includes/bootstrap.inc');

mash_bootstrap_prepare();
exit(mash_main());

/**
 * The main Drush function.
 *
 * - Runs "early" option code, if set (see global options).
 * - Parses the command line arguments, configuration files and environment.
 * - Prepares and executes a Magento bootstrap, if possible,
 * - Dispatches the given command.
 *
 * function_exists('mash_main') may be used by modules to detect whether
 * they are being called from mash.  See http://magento.org/node/1181308
 * and http://magento.org/node/827478
 *
 * @return
 *   Whatever the given command returns.
 */
function mash_main() {
  $return = '';
  if ($file = mash_get_option('early', FALSE)) {
    require_once($file);
    $function = 'mash_early_' . basename($file, '.inc');
    if (function_exists($function)) {
      if ($return = $function()) {
        // If the function returns FALSE, we continue and attempt to bootstrap
        // as normal. Otherwise, we exit early with the returned output.
        if ($return === TRUE) {
          $return = '';
        }
        mash_bootstrap_finish();
        return $return;
      }
    }
  }

  // Process initial global options such as --debug.
  _mash_bootstrap_global_options();

  $return = '';
  mash_bootstrap_to_phase(MASH_BOOTSTRAP_MASH);
  if (!mash_get_error()) {
    // Do any necessary preprocessing operations on the command,
    // perhaps handling immediately.
    $command_handled = mash_preflight_command_dispatch();
    if (!$command_handled) {
      $return = _mash_bootstrap_and_dispatch();
    }
  }
  mash_bootstrap_finish();

  // After this point the mash_shutdown function will run,
  // exiting with the correct exit code.
  return $return;
}

function _mash_bootstrap_and_dispatch() {
  $phases = _mash_bootstrap_phases(FALSE, TRUE);

  $return = '';
  $command_found = FALSE;
  _mash_bootstrap_output_prepare();
  foreach ($phases as $phase) {
    if (mash_bootstrap_to_phase($phase)) {
      $command = mash_parse_command();
      if (is_array($command)) {
        $bootstrap_result = mash_bootstrap_to_phase($command['bootstrap']);
        mash_enforce_requirement_bootstrap_phase($command);
        mash_enforce_requirement_core($command);
        mash_enforce_requirement_magento_dependencies($command);
        mash_enforce_requirement_mash_dependencies($command);

        if ($bootstrap_result && empty($command['bootstrap_errors'])) {
          mash_log(dt("Found command: !command (commandfile=!commandfile)", array('!command' => $command['command'], '!commandfile' => $command['commandfile'])), 'bootstrap');

          $command_found = TRUE;
          // Dispatch the command(s).
          $return = mash_dispatch($command);

          // prevent a '1' at the end of the output
          if ($return === TRUE) {
            $return = '';
          }

          if (mash_get_context('MASH_DEBUG') && !mash_get_context('MASH_QUIET')) {
            mash_print_timers();
          }
          mash_log(dt('Peak memory usage was !peak', array('!peak' => mash_format_size(memory_get_peak_usage()))), 'memory');
          break;
        }
      }
    }
    else {
      break;
    }
  }

  if (!$command_found) {
    // If we reach this point, command doesn't fit requirements or we have not
    // found either a valid or matching command.

    // If no command was found check if it belongs to a disabled module.
    if (!$command) {
      $command = mash_command_belongs_to_disabled_module();
    }

    // Set errors related to this command.
    $args = implode(' ', mash_get_arguments());
    if (isset($command) && is_array($command)) {
      foreach ($command['bootstrap_errors'] as $key => $error) {
        mash_set_error($key, $error);
      }
      mash_set_error('MASH_COMMAND_NOT_EXECUTABLE', dt("The mash command '!args' could not be executed.", array('!args' => $args)));
    }
    elseif (!empty($args)) {
      mash_set_error('MASH_COMMAND_NOT_FOUND', dt("The mash command '!args' could not be found.  Run `mash cache-clear mash` to clear the commandfile cache if you have installed new extensions.", array('!args' => $args)));
    }
    // Set errors that occurred in the bootstrap phases.
    $errors = mash_get_context('MASH_BOOTSTRAP_ERRORS', array());
    foreach ($errors as $code => $message) {
      mash_set_error($code, $message);
    }
  }
  return $return;
}

/**
 * Check if the given command belongs to a disabled module
 *
 * @return
 *   Array with a command-like bootstrap error or FALSE if Magento was not
 * bootstrapped fully or the command does not belong to a diabled module.
 */
function mash_command_belongs_to_disabled_module() {
  if (mash_has_boostrapped(MASH_BOOTSTRAP_MAGENTO_FULL)) {
    _mash_find_commandfiles(MASH_BOOTSTRAP_MAGENTO_SITE, MASH_BOOTSTRAP_MAGENTO_CONFIGURATION);
    $commands = mash_get_commands();
    $command_name = array_shift(mash_get_arguments());
    if (isset($commands[$command_name])) {
      // We found it. Load its module name and set an error.
      if (is_array($commands[$command_name]['magento dependencies']) && count($commands[$command_name]['magento dependencies'])) {
        $modules = implode(', ', $commands[$command_name]['magento dependencies']);
      } else {
        // The command does not define Magento dependencies. Derive them.
        $command_files = mash_get_context('MASH_COMMAND_FILES', array());
        $command_path = $commands[$command_name]['path'] . DIRECTORY_SEPARATOR . $commands[$command_name]['commandfile'] . '.mash.inc';
        $modules = array_search($command_path, $command_files);
      }
      return array(
        'bootstrap_errors' => array(
          'MASH_COMMAND_DEPENDENCY_ERROR' =>
        dt('Command !command needs the following module(s) enabled to run: !dependencies.', array('!command' => $command_name, '!dependencies' => $modules)),
        )
      );
    }
  }

  return FALSE;
}
