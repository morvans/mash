<?php

/**
 * @file
 *
 * Drush bootstrapping code.
 *
 * Functions here are used to bootstrap mash and then maybe Magento to
 * a level sufficient so that we can run commands. This is also where
 * third party extensions can bootstrap mash without going through
 * mash.php (this is used in the unit tests for example).
 */

/**
 * @name Drush bootstrap phases
 * @{
 * Sequential Drush bootstrapping phases.
 */

/**
 * No bootstrap.
 *
 * This constant is only used to indicate that the bootstrap process has
 * not started yet.  It is not possible to have no bootstrap.
 */
define('MASH_BOOTSTRAP_NONE', -1);

/**
 * Use mash_bootstrap_max instead of mash_bootstrap_to_phase
 *
 * This constant is only usable as the value of the 'bootstrap'
 * item of a command object, or as the parameter to
 * mash_bootstrap_to_phase.  It is not a real bootstrap state.
 */
define('MASH_BOOTSTRAP_MAX', -2);

/**
 * Only bootstrap Drush, without any Magento specific code.
 *
 * Any code that operates on the Drush installation, and not specifically
 * any Magento directory, should bootstrap to this phase.
 */
define('MASH_BOOTSTRAP_MASH', 0);

/**
 * Set up and test for a valid magento root, either through the -r/--root options,
 * or evaluated based on the current working directory.
 *
 * Any code that interacts with an entire Magento installation, and not a specific
 * site on the Magento installation should use this bootstrap phase.
 */
define('MASH_BOOTSTRAP_MAGENTO_ROOT',  1);

/**
 * Set up a Magento site directory and the correct environment variables to
 * allow Magento to find the configuration file.
 *
 * If no site is specified with the -l / --uri options, Drush will assume the
 * site is 'default', which mimics Magento's behaviour.
 *
 * If you want to avoid this behaviour, it is recommended that you use the
 * MASH_BOOTSTRAP_MAGENTO_ROOT bootstrap phase instead.
 *
 * Any code that needs to modify or interact with a specific Magento site's
 * settings.php file should bootstrap to this phase.
 */
define('MASH_BOOTSTRAP_MAGENTO_SITE', 2);

/**
 * Load the settings from the Magento sites directory.
 *
 * This phase is analagous to the MAGENTO_BOOTSTRAP_CONFIGURATION bootstrap phase in Magento
 * itself, and this is also the first step where Magento specific code is included.
 *
 * This phase is commonly used for code that interacts with the Magento install API,
 * as both install.php and update.php start at this phase.
 */
define('MASH_BOOTSTRAP_MAGENTO_CONFIGURATION', 3);

/**
 * Connect to the Magento database using the database credentials loaded
 * during the previous bootstrap phase.
 *
 * This phase is analogous to the MAGENTO_BOOTSTRAP_DATABASE bootstrap phase in
 * Magento.
 *
 * Any code that needs to interact with the Magento database API needs to
 * be bootstrapped to at least this phase.
 */
define('MASH_BOOTSTRAP_MAGENTO_DATABASE', 4);

/**
 * Fully initialize Magento.
 *
 * This is analogous to the MAGENTO_BOOTSTRAP_FULL bootstrap phase in
 * Magento.
 *
 * Any code that interacts with the general Magento API should be
 * bootstrapped to this phase.
 */
define('MASH_BOOTSTRAP_MAGENTO_FULL', 5);

/**
 * Log in to the initialiased Magento site.
 *
 * This is the default bootstrap phase all commands will try to reach,
 * unless otherwise specified.
 *
 * This bootstrap phase is used after the site has been
 * fully bootstrapped.
 *
 * This phase will log you in to the magento site with the username
 * or user ID specified by the --user/ -u option.
 *
 * Use this bootstrap phase for your command if you need to have access
 * to information for a specific user, such as listing nodes that might
 * be different based on who is logged in.
 */
define('MASH_BOOTSTRAP_MAGENTO_LOGIN', 6);

/**
 * Helper function listing phases.
 *
 * For commands that need to iterate through the phases, such as help
 */
function _mash_bootstrap_phases($function_names = FALSE, $init_phases_only = FALSE) {
  static $functions = array(
    MASH_BOOTSTRAP_MASH                  => '_mash_bootstrap_mash',
    MASH_BOOTSTRAP_MAGENTO_ROOT            => '_mash_bootstrap_magento_root',
    MASH_BOOTSTRAP_MAGENTO_SITE            => '_mash_bootstrap_magento_site',
    MASH_BOOTSTRAP_MAGENTO_CONFIGURATION   => '_mash_bootstrap_magento_configuration',
    MASH_BOOTSTRAP_MAGENTO_DATABASE        => '_mash_bootstrap_magento_database',
    MASH_BOOTSTRAP_MAGENTO_FULL            => '_mash_bootstrap_magento_full',
    MASH_BOOTSTRAP_MAGENTO_LOGIN           => '_mash_bootstrap_magento_login');

  $result = array();
  if ($init_phases_only) {
    foreach (array(MASH_BOOTSTRAP_MASH, MASH_BOOTSTRAP_MAGENTO_FULL) as $phase) {
      $result[$phase] = $functions[$phase];
    }
  }
  else {
    $result = $functions;
  }
  if (!$function_names) {
    $result = array_keys($result);
  }
  return $result;
}

/**
 * @} End of Drush bootstrap phases.
 */

/**
 * Bootstrap Drush to the desired phase.
 *
 * This function will sequentially bootstrap each
 * lower phase up to the phase that has been requested.
 *
 * @param phase
 *   The bootstrap phase to bootstrap to.
 *   Any of the following constants :
 *     MASH_BOOTSTRAP_MASH                = Only Drush.
 *     MASH_BOOTSTRAP_MAGENTO_ROOT          = Find a valid Magento root.
 *     MASH_BOOTSTRAP_MAGENTO_SITE          = Find a valid Magento site.
 *     MASH_BOOTSTRAP_MAGENTO_CONFIGURATION = Load the site's settings.
 *     MASH_BOOTSTRAP_MAGENTO_DATABASE      = Initialize the database.
 *     MASH_BOOTSTRAP_MAGENTO_FULL          = Initialize Magento fully.
 *     MASH_BOOTSTRAP_MAGENTO_LOGIN         = Log into Magento with a valid user.
 */
function mash_bootstrap($phase, $phase_max = FALSE) {
  static $phases;
  if (!$phases) {
    $phases = _mash_bootstrap_phases(TRUE);
  }
  static $phase_index = 0;

  mash_set_context('MASH_BOOTSTRAPPING', TRUE);
  while ($phase >= $phase_index && isset($phases[$phase_index])) {
    if (mash_bootstrap_validate($phase_index)) {
      $current_phase = $phases[$phase_index];
      if (function_exists($current_phase) && !mash_get_error()) {
        mash_log(dt("Mash bootstrap phase : !function()", array('!function' => $current_phase)), 'bootstrap');
        $current_phase();

        // Find any command files that are available during this bootstrap phase.
        _mash_find_commandfiles($phase_index, $phase_max);
      }
      mash_set_context('MASH_BOOTSTRAP_PHASE', $phase_index);
    }
    else {
      $errors = mash_get_context('MASH_BOOTSTRAP_ERRORS', array());
      foreach ($errors as $code => $message) {
        mash_set_error($code, $message);
      }
    }
    unset($phases[$phase_index++]);
  }
  mash_set_context('MASH_BOOTSTRAPPING', FALSE);

  return !mash_get_error();
}

/**
 * Determine whether a given bootstrap phase has been completed
 *
 * This function name has a typo which makes me laugh so we choose not to
 * fix it. Take a deep breath, and smile. See
 * http://en.wikipedia.org/wiki/HTTP_referer
 *
 *
 * @param phase
 *   The bootstrap phase to test
 *
 * @returns
 *   TRUE if the specified bootstrap phase has completed.
 */
function mash_has_boostrapped($phase) {
  $phase_index = mash_get_context('MASH_BOOTSTRAP_PHASE');

  return isset($phase_index) && ($phase_index >= $phase);
}

/**
 * Validate whether a bootstrap phase can be reached.
 *
 * This function will validate the settings that will be used
 * during the actual bootstrap process, and allow commands to
 * progressively bootstrap to the highest level that can be reached.
 *
 * This function will only run the validation function once, and
 * store the result from that execution in a local static. This avoids
 * validating phases multiple times.
 *
 * @param phase
 *   The bootstrap phase to validate to.
 *   Any of the following constants :
 *     MASH_BOOTSTRAP_MASH                = Only Drush.
 *     MASH_BOOTSTRAP_MAGENTO_ROOT          = Find a valid Magento root.
 *     MASH_BOOTSTRAP_MAGENTO_SITE          = Find a valid Magento site.
 *     MASH_BOOTSTRAP_MAGENTO_CONFIGURATION = Load the site's settings.
 *     MASH_BOOTSTRAP_MAGENTO_DATABASE      = Initialize the database.
 *     MASH_BOOTSTRAP_MAGENTO_FULL          = Initialize Magento fully.
 *     MASH_BOOTSTRAP_MAGENTO_LOGIN         = Log into Magento with a valid user.
 *
 * @return
 *   True if bootstrap is possible, False if the validation failed.
 *
 */
function mash_bootstrap_validate($phase) {
  static $phases;
  static $result_cache = array();
  if (!$phases) {
    $phases = _mash_bootstrap_phases(TRUE);
  }
  static $phase_index = 0;
  // Check to see if anyone has changed --root or --uri
  _mash_bootstrap_select_magento_site();
  if (!array_key_exists($phase, $result_cache)) {
    mash_set_context('MASH_BOOTSTRAP_ERRORS', array());
    mash_set_context('MASH_BOOTSTRAP_VALUES', array());

    while ($phase >= $phase_index && isset($phases[$phase_index])) {
      $current_phase = $phases[$phase_index] . '_validate';
      if (function_exists($current_phase)) {
        $result_cache[$phase_index] = $current_phase();
      }
      else {
        $result_cache[$phase_index] = TRUE;
      }
      mash_set_context('MASH_BOOTSTRAP_VALIDATION_PHASE', $phase_index);
      unset($phases[$phase_index++]);
    }
  }
  return $result_cache[$phase];
}

/**
 * Bootstrap to the specified phase.
 *
 * @param $max_phase_index
 *   Only attempt bootstrap to the specified level.
 */
function mash_bootstrap_to_phase($max_phase_index) {
  // If $max_phase_index is MASH_BOOTSTRAP_MAX, then
  // we will bootstrap as far as we can.  mash_bootstrap_max
  // is different than mash_bootstrap_to_phase in that
  // it is not an error if MASH_BOOTSTRAP_LOGIN is not reached.
  if ($max_phase_index == MASH_BOOTSTRAP_MAX) {
    mash_bootstrap_max();
    return TRUE;
  }

  mash_log(dt("Bootstrap to phase !phase.", array('!phase' => $max_phase_index)), 'bootstrap');
  $phases = _mash_bootstrap_phases();
  $result = TRUE;

  // Try to bootstrap to the maximum possible level, without generating errors
  foreach ($phases as $phase_index) {
    if ($phase_index > $max_phase_index) {
      // Stop trying, since we achieved what was specified.
      break;
    }

    if (mash_bootstrap_validate($phase_index)) {
      if ($phase_index > mash_get_context('MASH_BOOTSTRAP_PHASE')) {
        $result = mash_bootstrap($phase_index, $max_phase_index);
      }
    }
    else {
      $result = FALSE;
      break;
    }
  }

  return $result;
}

/**
 * Bootstrap to the highest level possible, without triggering any errors.
 *
 * @param $max_phase_index
 *   Only attempt bootstrap to the specified level.
 *
 *  @return int
 *    The maximum phase to which we bootstrapped.
 */
function mash_bootstrap_max($max_phase_index = FALSE) {
  $phases = _mash_bootstrap_phases();
  $phase_index = MASH_BOOTSTRAP_MASH;
  if (!$max_phase_index) {
    $max_phase_index = count($phases);
  }

  // Try to bootstrap to the maximum possible level, without generating errors
  foreach ($phases as $phase_index) {
    if ($phase_index > $max_phase_index) {
      // Stop trying, since we achieved what was specified.
      break;
    }

    if (mash_bootstrap_validate($phase_index)) {
      if ($phase_index > mash_get_context('MASH_BOOTSTRAP_PHASE')) {
        mash_bootstrap($phase_index, $max_phase_index);
      }
    }
    else {
      break;
    }
  }

  return mash_get_context('MASH_BOOTSTRAP_PHASE');
}

/**
 * Bootstrap the specified site alias.  The site alias must
 * be a valid alias to a local site.
 *
 * @param $site_record
 *   The alias record for the given site alias.
 *   @see mash_sitealias_get_record().
 * @param $max_phase_index
 *   Only attempt bootstrap to the specified level.
 * @returns TRUE if attempted to bootstrap, or FALSE
 *   if no bootstrap attempt was made.
 */
function mash_bootstrap_max_to_sitealias($site_record, $max_phase_index = NULL) {
  if ((array_key_exists('root', $site_record) && !array_key_exists('remote-host', $site_record))) {
    mash_sitealias_set_alias_context($site_record);
    mash_bootstrap_max($max_phase_index);
    return TRUE;
  }
  return FALSE;
}

/**
 * Helper function to collect any errors that occur during the bootstrap process.
 * Always returns FALSE, for convenience.
 */
function mash_bootstrap_error($code, $message = null) {
  $errors = mash_get_context('MASH_BOOTSTRAP_ERRORS');
  $errors[$code] = $message;
  mash_set_context('MASH_BOOTSTRAP_ERRORS', $errors);
  return FALSE;
}

/**
 * Validate that Drush is running in a suitable environment.
 */
function _mash_bootstrap_mash_validate() {
  // @todo _mash_environment_php_ini_checks() always returns TRUE.
  $return = _mash_environment_check_php_ini();
  if ($return !== TRUE) {
    return $return;
  }

  if (mash_environment_lib() === FALSE) {
    return FALSE;
  }

  if (mash_environment_table_lib() === FALSE) {
    return FALSE;
  }

  return TRUE;
}

/**
 * Initial Drush bootstrap phase.
 *
 * During the initialization of Drush,
 * this is the first step where all we are
 * aware of is Drush itself.
 *
 * In this step we will register the shutdown function,
 * parse the command line arguments and store them in their
 * related contexts.
 *
 * Configuration files (mashrc.php) that are
 *   a) Specified on the command line
 *   b) Stored in the root directory of mash.php
 *   c) Stored in the home directory of the system user.
 *
 * Additionally the MASH_QUIET and MASH_BACKEND contexts,
 * will be evaluated now, as they need to be set very early in
 * the execution flow to be able to take affect/
 */
function _mash_bootstrap_mash() {
  // Create an alias '@none' to represent no Magento site
  _mash_sitealias_cache_alias('none', array('root' => '', 'uri' => ''));

  // Set the terminal width, used for wrapping table output.
  // Normally this is exported using tput in the mash script.
  // If this is not present we do an additional check using stty here.
  // On Windows in CMD and PowerShell is this exported using mode con.
  if (!($columns = getenv('COLUMNS'))) {
    // Trying to export the columns using stty.
    exec('stty size 2>&1', $columns_output, $columns_status);
    if (!$columns_status) $columns = preg_replace('/\d+\s(\d+)/', '$1', $columns_output[0], -1, $columns_count);

    // If stty fails and Drush us running on Windows are we trying with mode con.
    if (($columns_status || !$columns_count) && mash_is_windows()) {
      $columns_output = array();
      exec('mode con', $columns_output, $columns_status);
      if (!$columns_status && is_array($columns_output)) {
        $columns = (int)preg_replace('/\D/', '', $columns_output[4], -1, $columns_count);
      }
    }

    // Failling back to default columns value
    if (empty($columns)) {
      $columns = 80;
    }
  }
  // If a caller wants to reserve some room to add additional
  // information to the mash output via post-processing, the
  // --reserve-margin flag can be used to declare how much
  // space to leave out.  This only affects mash functions
  // such as mash_print_table that wrap the output.
  $columns -= mash_get_option('reserve-margin', 0);
  mash_set_context('MASH_COLUMNS', $columns);

  // Statically define a way to call mash again.
  define('MASH_COMMAND', mash_find_mash());

  // prime the CWD cache
  mash_cwd();

  // Set up base environment for system-wide file locations.
  _mash_bootstrap_base_environment();

  // Load a mashrc.php file in the mash.php's directory.
  mash_load_config('mash');

  // Load a mashrc.php file in the $ETC_PREFIX/etc/mash directory.
  mash_load_config('system');

  // Load a mashrc.php file at ~/.mashrc.php.
  mash_load_config('user');

  // Load a mashrc.php file in the ~/.mash directory.
  mash_load_config('home.mash');

  // Load a custom config specified with the --config option.
  mash_load_config('custom');

  // Process the site alias that specifies which instance
  // of mash (local or remote) this command will operate on.
  // We must do this after we load our config files (so that
  // site aliases are available), but before the rest
  // of the mash and magento root bootstrap phases are
  // done, since site aliases may set option values that
  // affect these phases.
  mash_sitealias_check_arg();

  // Load the config options from Magento's sites/all/mash directory, even prior to bootstrapping the root
  mash_load_config('magento');

  // Similarly, load the Magento site configuration options upfront.
  mash_load_config('site');

  // If applicable swaps in shell alias value (or executes it).
  mash_shell_alias_replace();

  // If mash_load_config defined a site alias that did not
  // exist before, then sitealias check arg might now match
  // against one of those aliases.
  if (mash_sitealias_check_arg() === TRUE) {
    $remote_host = mash_get_option('remote-host');
    if (!isset($remote_host)) {
      // Load the config files for the "new" site.
      mash_load_config('magento');
      mash_load_config('site');
    }
  }
  // Check to see if we 'use'd a site alias with site-set
  mash_sitealias_check_site_env();

  _mash_bootstrap_global_options();
}

function _mash_bootstrap_output_prepare() {
  $backend = mash_set_context('MASH_BACKEND', mash_get_option('backend'));

  // Pipe implies quiet.
  $quiet = mash_set_context('MASH_QUIET', mash_get_option(array('quiet', 'pipe')));

  mash_set_context('MASH_PIPE', mash_get_option('pipe'));

  if ($backend) {
    // Load options passed as a JSON encoded string through STDIN.
    $stdin_options = _mash_backend_get_stdin();
    if (is_array($stdin_options)) {
      mash_set_context('stdin', $stdin_options);
    }
    // Add an output buffer handler to collect output/pass through backend
    // packets. Using a chunksize of 2 ensures that each line is flushed
    // straight away.
    if ($quiet) {
      // Pass through of backend packets, discard regular output.
      ob_start('mash_backend_output_discard', 2);
    }
    else {
      // Collect output.
      ob_start('mash_backend_output_collect', 2);
    }
  }

  // In non-backend quiet mode we start buffering and discards it on command
  // completion.
  if ($quiet && !$backend) {
    ob_start();
  }
}

/**
 * Determine which Magento site will be selected.
 *
 * The Magento site itself will be bootstrapped at a later
 * phase; at this time, we set context variables to
 * point to the magento root, site URI and site configuration
 * path that will be used when needed.
 *
 * These early context variables are used to find
 * mash configuration and alias files stored with the
 * site to be bootstrapped.
 */
function _mash_bootstrap_select_magento_site() {
  $magento_root = mash_get_option('root');
  if (!isset($magento_root)) {
    $magento_root = mash_locate_root();
  }
  mash_set_context('MASH_SELECTED_MAGENTO_ROOT', $magento_root);
  //mash_set_context('MASH_SELECTED_MAGENTO_SITES_ALL_MASH', $magento_root . '/sites/all/mash');
  $uri = _mash_bootstrap_selected_uri();
  mash_set_context('MASH_SELECTED_URI', $uri);
  //mash_set_context('MASH_SELECTED_MAGENTO_SITE_CONF_PATH', mash_conf_path($uri));

  if (!empty($magento_root) && !empty($uri)) {
    // Create an alias '@self'
    _mash_sitealias_cache_alias('self', array('root' => $magento_root, 'uri' => $uri));
  }
}

/**
 * Sets up basic environment that controls where Drush looks for files on a
 * system-wide basis. Important to call for "early" functions that need to
 * work with unit tests.
 */
function _mash_bootstrap_base_environment() {
  // Copy ETC_PREFIX and SHARE_PREFIX from environment variables if available.
  // This alters where we check for server-wide config and alias files.
  // Used by unit test suite to provide a clean environment.
  if (getenv('ETC_PREFIX')) mash_set_context('ETC_PREFIX', getenv('ETC_PREFIX'));
  if (getenv('SHARE_PREFIX')) mash_set_context('SHARE_PREFIX', getenv('SHARE_PREFIX'));

  mash_set_context('DOC_PREFIX', MASH_BASE_PATH);
  if (!file_exists(MASH_BASE_PATH . '/README.txt') && file_exists(mash_get_context('SHARE_PREFIX', '/usr') . '/share/doc/mash') . '/README.txt') {
    mash_set_context('DOC_PREFIX', mash_get_context('SHARE_PREFIX', '/usr') . '/share/doc/mash');
  }
  $alias_path =& mash_get_context('ALIAS_PATH');
  $default_prefix_configuration = mash_is_windows() ? getenv('ALLUSERSPROFILE') . '/Drush' : '';
  $default_prefix_commandfile = mash_is_windows() ? getenv('ALLUSERSPROFILE') . '/Drush' : '/usr';
  $site_wide_configuration_dir = mash_get_context('ETC_PREFIX', $default_prefix_configuration) . '/etc/mash';
  $site_wide_commandfile_dir = mash_get_context('SHARE_PREFIX', $default_prefix_commandfile) . '/share/mash/commands';
  $alias_path[] = $site_wide_configuration_dir;
  mash_set_context('MASH_SITE_WIDE_CONFIGURATION', $site_wide_configuration_dir);
  mash_set_context('MASH_SITE_WIDE_COMMANDFILES', $site_wide_commandfile_dir);
  $alias_path[] = dirname(__FILE__) . '/..';
  if(!is_null(mash_server_home())) {
    $alias_path[] = mash_server_home() . '/.mash';
    mash_set_context('MASH_PER_USER_CONFIGURATION', mash_server_home() . '/.mash');
  }
}

function _mash_bootstrap_global_options() {
  // Debug implies verbose
  mash_set_context('MASH_VERBOSE',     mash_get_option(array('verbose', 'debug'), FALSE));
  mash_set_context('MASH_DEBUG', mash_get_option('debug'));

  // Backend implies affirmative unless negative is explicitly specified
  mash_set_context('MASH_NEGATIVE',    mash_get_option('no', FALSE));
  mash_set_context('MASH_AFFIRMATIVE', mash_get_option(array('yes', 'pipe'), FALSE) || (mash_get_context('MASH_BACKEND') && !mash_get_context('MASH_NEGATIVE')));
  mash_set_context('MASH_SIMULATE',    mash_get_option('simulate', FALSE));

  // Suppress colored logging if --nocolor option is explicitly given or if
  // terminal does not support it.
  $nocolor = (mash_get_option('nocolor', FALSE));
  if (!$nocolor) {
    // Check for colorless terminal.  If there is no terminal, then
    // 'tput colors 2>&1' will return "tput: No value for $TERM and no -T specified",
    // which is not numeric and therefore will put us in no-color mode.
    $colors = exec('tput colors 2>&1');
    $nocolor = !($colors === FALSE || (is_numeric($colors) && $colors >= 3));
  }
  mash_set_context('MASH_NOCOLOR', $nocolor);
}

/**
 * Validate the MASH_BOOTSTRAP_MAGENTO_ROOT phase.
 *
 * In this function, we will check if a valid Magento directory is available.
 * We also determine the value that will be stored in the MASH_MAGENTO_ROOT
 * context and MAGENTO_ROOT constant if it is considered a valid option.
 */
function _mash_bootstrap_magento_root_validate() {
  $magento_root = mash_get_context('MASH_SELECTED_MAGENTO_ROOT');

  if (empty($magento_root)) {
    return mash_bootstrap_error('MASH_NO_MAGENTO_ROOT', dt("A Magento installation directory could not be found"));
  }
  if (!$signature = mash_valid_magento_root($magento_root)) {
    return mash_bootstrap_error('MASH_INVALID_MAGENTO_ROOT', dt("The directory !magento_root does not contain a valid Magento installation", array('!magento_root' => $magento_root)));
  }

  mash_bootstrap_value('magento_root', $magento_root);
  define('MASH_MAGENTO_SIGNATURE', $signature);

  return TRUE;
}

/**
 * Bootstrap Drush with a valid Magento Directory.
 *
 * In this function, the pwd will be moved to the root
 * of the Magento installation.
 *
 * The MASH_MAGENTO_ROOT context, MASH_MAGENTO_CORE context, MAGENTO_ROOT, and the
 * MASH_MAGENTO_CORE constants are populated from the value that we determined during
 * the validation phase.
 *
 * We also now load the mashrc.php for this specific platform.
 * We can now include files from the Magento Tree, and figure
 * out more context about the platform, such as the version of Magento.
 */
function _mash_bootstrap_magento_root() {
  $magento_root = mash_set_context('MASH_MAGENTO_ROOT', mash_bootstrap_value('magento_root'));
  define('MAGENTO_ROOT', $magento_root);

  chdir($magento_root);
  require_once MAGENTO_ROOT . '/' . '/app/Mage.php';
  $version = mash_set_context('MASH_MAGENTO_VERSION', mash_magento_version());
  $major_version = mash_set_context('MASH_MAGENTO_MAJOR_VERSION', mash_magento_major_version());

  if (version_compare($version, '1.6') < 0) {
    mash_set_error('MASH_MAGENTO_VERSION_UNSUPPORTED', dt('Mash !mash_version does not support Magento !version.', array('!mash_version' => MASH_VERSION, '!version' => $version)));
  }

  $core = MAGENTO_ROOT;
  mash_set_context('MASH_MAGENTO_CORE', $core);
  define('MASH_MAGENTO_CORE', $core);

  _mash_bootstrap_global_options();

  mash_log(dt("Initialized Magento !version root directory at !magento_root", array("!version" => $version, '!magento_root' => $magento_root)));
}

/**
 * Find the URI that has been selected by the --uri / -l option
 * or the cwd.
 */
function _mash_bootstrap_selected_uri() {
  $uri = mash_get_option('uri');
  if (!isset($uri)) {
    $site_path = mash_site_path();
    $elements = explode('/', $site_path);
    $current = array_pop($elements);
    if (!$current) {
      $current = 'default';
    }
    $uri = 'http://'. $current;
  }

  return $uri;
}

/**
 * VALIDATE the MASH_BOOTSTRAP_MAGENTO_SITE phase.
 *
 * In this function we determine the URL used for the command,
 * and check for a valid settings.php file.
 *
 * To do this, we need to set up the $_SERVER environment variable,
 * to allow us to use conf_path to determine what Magento will load
 * as a configuration file.
 */
function _mash_bootstrap_magento_site_validate() {
//  $mash_uri = mash_get_context('MASH_SELECTED_URI');
//
//  // Fake the necessary HTTP headers that Magento needs:
//  if ($mash_uri) {
//    $magento_base_url = parse_url($mash_uri);
//    // If there's no url scheme set, add http:// and re-parse the url
//    // so the host and path values are set accurately.
//    if (!array_key_exists('scheme', $magento_base_url)) {
//      $mash_uri = 'http://' . $mash_uri;
//      $magento_base_url = parse_url($mash_uri);
//    }
//    // Fill in defaults.
//    $magento_base_url += array(
//      'path' => NULL,
//      'host' => NULL,
//      'port' => NULL,
//    );
//    $_SERVER['HTTP_HOST'] = $magento_base_url['host'];
//
//    if ($magento_base_url['port']) {
//      $_SERVER['HTTP_HOST'] .= ':' . $magento_base_url['port'];
//    }
//    $_SERVER['SERVER_PORT'] = $magento_base_url['port'];
//
//    if (array_key_exists('path', $magento_base_url)) {
//      $_SERVER['PHP_SELF'] = $magento_base_url['path'] . '/index.php';
//    }
//    else {
//      $_SERVER['PHP_SELF'] = '/index.php';
//    }
//  }
//  else {
//    $_SERVER['HTTP_HOST'] = 'default';
//    $_SERVER['PHP_SELF'] = '/index.php';
//  }
//
//  $_SERVER['REQUEST_URI'] = $_SERVER['SCRIPT_NAME'] = $_SERVER['PHP_SELF'];
//  $_SERVER['REMOTE_ADDR'] = '127.0.0.1';
//  $_SERVER['REQUEST_METHOD']  = NULL;
//
//  $_SERVER['SERVER_SOFTWARE'] = NULL;
//  $_SERVER['HTTP_USER_AGENT'] = NULL;
//
//  $site = mash_bootstrap_value('site', $_SERVER['HTTP_HOST']);

  $conf_path = 'app/etc';
  $conf_file = "./$conf_path/local.xml";
  if (!file_exists($conf_file)) {
    return mash_bootstrap_error('MAGENTO_SITE_SETTINGS_NOT_FOUND', dt("Could not find a Magento local.xml file at !file.",
       array('!file' => $conf_file)));
  }

  return TRUE;
}

/**
 * Called by _mash_bootstrap_magento_site to do the main work
 * of the mash magento site bootstrap.
 */
function _mash_bootstrap_do_magento_site() {
  $mash_uri = mash_get_context('MASH_SELECTED_URI');
  mash_set_context('MASH_URI', $mash_uri);
  $site = mash_set_context('MASH_MAGENTO_SITE', mash_bootstrap_value('site'));
  $conf_path = mash_set_context('MASH_MAGENTO_SITE_ROOT', mash_bootstrap_value('conf_path'));

  mash_log(dt("Initialized Magento site !site at !site_root", array('!site' => $site, '!site_root' => $conf_path)));

  _mash_bootstrap_global_options();
}

/**
 * Initialize a site on the Magento root.
 *
 * We now set various contexts that we determined and confirmed to be valid.
 * Additionally we load an optional mashrc.php file in the site directory.
 */
function _mash_bootstrap_magento_site() {
  _mash_bootstrap_do_magento_site();
}

/**
 * Initialize and load the Magento configuration files.
 *
 * We process and store a normalized set of database credentials
 * from the loaded configuration file, so we can validate them
 * and access them easily in the future.
 *
 * Also override Magento variables as per --variables option.
 */
function _mash_bootstrap_magento_configuration() {
  global $conf;

  Mage::app('admin')->setUseSessionInUrl(false);
  Mage::getConfig()->init();


//  // Unset magento error handler and restore mash's one.
//  if (mash_magento_major_version() >= 7) {
//    restore_error_handler();
//  }

//  // Force Magento6 not to store queries since we are not outputting them.
//  // Don't run poormanscron during Drush request (D7+).
//  $override = array(
//    'dev_query' => FALSE,
//    'cron_safe_threshold' => 0,
//  );
//  $current_override = mash_get_option_list('variables');
//  foreach ($current_override as $name => $value) {
//    if (is_numeric($name) && (strpos($value, '=') !== FALSE)) {
//      list($name, $value) = explode('=', $value, 2);
//    }
//    $override[$name] = $value;
//  }
//  $conf = is_array($conf) ? array_merge($conf, $override) : $conf;

  // Populate the MASH_DB_CREDENTIALS with the fields loaded from the configuration.
// TODO
//  $creds = array();
//  switch (mash_magento_major_version()) {
//     case 6:
//       if (!empty($GLOBALS['db_url'])) {
//         $url = $GLOBALS['db_url'];
//         if (is_array($url)) {
//           $url = $url['default'];
//         }
//         $parts = parse_url($url);
//         $parts += array('pass' => '', 'port' => '');
//         $creds['driver'] = $parts['scheme'];
//         $creds['user'] = urldecode($parts['user']);
//         $creds['host'] = $parts['host'];
//         $creds['port'] = $parts['port'];
//         $creds['pass'] = urldecode($parts['pass']);
//         $creds['name'] = trim($parts['path'], '/');
//       }
//       break;
//     case 7:
//     default:
//       if (!empty($GLOBALS['databases']['default']['default'])) {
//         $conn = $GLOBALS['databases']['default']['default'];
//         // Fill in defaults to prevent notices.
//         $conn += array(
//           'username' => NULL,
//           'host' => NULL,
//           'port' => NULL,
//           'password' => NULL,
//           'database' => NULL,
//           'unix_socket' => NULL,
//         );
//         $creds['driver'] = $conn['driver'];
//         $creds['user'] = $conn['username'];
//         $creds['unix_socket'] = $conn['unix_socket'];
//         $creds['host'] = $conn['host'];
//         $creds['port'] = $conn['port'];
//         $creds['name'] = $conn['database'];
//         $creds['pass'] = $conn['password'];
//       }
//       break;
//  }
//
//  mash_set_context('MASH_DB_CREDENTIALS', $creds);
}

/**
 * Validate the MASH_BOOTSTRAP_MAGENTO_DATABASE phase
 *
 * Attempt to making a working database connection using the
 * database credentials that were loaded during the previous
 * phase.
 */
function _mash_bootstrap_magento_database_validate() {
  if (!mash_valid_db_credentials()) {
    return mash_bootstrap_error('MASH_MAGENTO_DB_ERROR');
  }
  return TRUE;
}

/**
 * Boostrap the Magento database.
 */
function _mash_bootstrap_magento_database() {
  mash_log(dt("Successfully connected to the Magento database."), 'bootstrap');
//  magento_bootstrap(MAGENTO_BOOTSTRAP_DATABASE);
}

/**
 * Attempt to load the full Magento system.
 */
function _mash_bootstrap_magento_full() {
  if (!mash_get_context('MASH_QUIET', FALSE)) {
    ob_start();
  }
  //magento_bootstrap(MAGENTO_BOOTSTRAP_FULL);
  Mage::app()->setUseSessionInUrl(false);
  if (!mash_get_context('MASH_QUIET', FALSE)) {
    ob_end_clean();
  }

//  // Unset magento error handler and restore mash's one.
//  if (mash_magento_major_version() == 6) {
//    restore_error_handler();
//  }
//
//  // If needed, prod module_implements() to recognize our system_watchdog() implementation.
//  $dogs = module_implements('watchdog');
//  if (!in_array('system', $dogs)) {
//    // Note that this resets module_implements cache.
//    module_implements('watchdog', FALSE, TRUE);
//  }

  _mash_log_magento_messages();
}

/**
 * Log into the bootstrapped Magento site with a specific
 * username or user id.
 */
function _mash_bootstrap_magento_login() {
  $mash_user = mash_set_context('MASH_USER', mash_get_option('user', 0));

  mash_magento_login($mash_user);
  _mash_log_magento_messages();
}

/**
 * Helper function to store any context settings that are being validated.
 */
function mash_bootstrap_value($context, $value = null) {
  $values =& mash_get_context('MASH_BOOTSTRAP_VALUES', array());

  if (!is_null($value)) {
    $values[$context] = $value;
  }

  if (array_key_exists($context, $values)) {
    return $values[$context];
  }

  return null;
}

/**
 * Prepare mash for bootstrap
 *
 * All pre-flight checks and includes necessary to begin the bootstrap
 * process.
 *
 * Ran before mash_main().
 *
 * @see mash_main()
 * @see mash.php
 */
function mash_bootstrap_prepare() {
  define('MASH_BASE_PATH', dirname(dirname(__FILE__)));

  require_once MASH_BASE_PATH . '/includes/environment.inc';
  require_once MASH_BASE_PATH . '/includes/command.inc';
  require_once MASH_BASE_PATH . '/includes/mash.inc';
  require_once MASH_BASE_PATH . '/includes/backend.inc';
//  require_once MASH_BASE_PATH . '/includes/batch.inc';
  require_once MASH_BASE_PATH . '/includes/context.inc';
  require_once MASH_BASE_PATH . '/includes/sitealias.inc';
  require_once MASH_BASE_PATH . '/includes/exec.inc';
  require_once MASH_BASE_PATH . '/includes/magento.inc';
  require_once MASH_BASE_PATH . '/includes/output.inc';
  require_once MASH_BASE_PATH . '/includes/cache.inc';
  require_once MASH_BASE_PATH . '/includes/filesystem.inc';
//  require_once MASH_BASE_PATH . '/includes/dbtng.inc';

  // Terminate immediately unless invoked as a command line script
  if (!mash_verify_cli()) {
    die('mash is designed to run via the command line.');
  }

  // Check supported version of PHP.
  define('MASH_MINIMUM_PHP', '5');
  if (version_compare(phpversion(), MASH_MINIMUM_PHP) < 0) {
    die('Your command line PHP installation is too old. Mash requires at least PHP ' . MASH_MINIMUM_PHP . "\n");
  }

  $mash_info = mash_read_mash_info();
  define('MASH_VERSION', $mash_info['mash_version']);
  $version_parts = explode('.', MASH_VERSION);
  define('MASH_MAJOR_VERSION', $version_parts[0]);
  define('MASH_MINOR_VERSION', $version_parts[1]);

  define('MASH_REQUEST_TIME', microtime(TRUE));

  mash_set_context('argc', $GLOBALS['argc']);
  mash_set_context('argv', $GLOBALS['argv']);

  // Set an error handler and a shutdown function
  set_error_handler('mash_error_handler');
  register_shutdown_function('mash_shutdown');

  mash_set_context('MASH_BOOTSTRAP_PHASE', MASH_BOOTSTRAP_NONE);

  // We need some global options/arguments processed at this early stage.
  mash_parse_args();
}

/**
 * Cleanup our bootstrap.
 *
 * This is ran after we have bootstrapped and dispatched properly in
 * mash_main().
 *
 * @see mash_main()
 */
function mash_bootstrap_finish() {
  // We set this context to let the shutdown function know we reached the end of mash_main();
  mash_set_context("MASH_EXECUTION_COMPLETED", TRUE);
}

/**
 * Shutdown function for use while Magento is bootstrapping and to return any
 * registered errors.
 *
 * The shutdown command checks whether certain options are set to reliably
 * detect and log some common Magento initialization errors.
 *
 * If the command is being executed with the --backend option, the script
 * will return a json string containing the options and log information
 * used by the script.
 *
 * The command will exit with '1' if it was successfully executed, and the
 * result of mash_get_error() if it wasn't.
 */
function mash_shutdown() {
  // Mysteriously make $user available during sess_write(). Avoids a NOTICE.
  global $user;

  if (!mash_get_context('MASH_EXECUTION_COMPLETED', FALSE) && !mash_get_context('MASH_USER_ABORT', FALSE)) {
    $php_error_message = '';
    if ($error = error_get_last()) {
      $php_error_message = "\n" . dt('Error: !message in !file, line !line', array('!message' => $error['message'], '!file' => $error['file'], '!line' => $error['line']));
    }
    // We did not reach the end of the mash_main function,
    // this generally means somewhere in the code a call to exit(),
    // was made. We catch this, so that we can trigger an error in
    // those cases.
    mash_set_error("MASH_NOT_COMPLETED", dt("Drush command terminated abnormally due to an unrecoverable error.!message", array('!message' => $php_error_message)));
    // Attempt to give the user some advice about how to fix the problem
    _mash_postmortem();
  }

  $phase = mash_get_context('MASH_BOOTSTRAP_PHASE');
  if (mash_get_context('MASH_BOOTSTRAPPING')) {
    switch ($phase) {
      case MASH_BOOTSTRAP_MAGENTO_FULL :
        ob_end_clean();
        _mash_log_magento_messages();
        mash_set_error('MASH_MAGENTO_BOOTSTRAP_ERROR');
        break;
    }
  }

  if (mash_get_context('MASH_BACKEND', FALSE)) {
    mash_backend_output();
  }
  elseif (mash_get_context('MASH_QUIET', FALSE)) {
    ob_end_clean();
    // If we are in pipe mode, emit the compact representation of the command, if available.
    if (mash_get_context('MASH_PIPE')) {
      mash_pipe_output();
    }
  }

  /**
   * For now, mash skips end of page processing on D7. Doing so could write
   * cache entries to module_implements and lookup_cache that don't match web requests.
   */
   if (mash_magento_major_version() >= 7 && function_exists('magento_page_footer')) {
     magento_page_footer();
   }

  // this way mash_return_status will always be the last shutdown function (unless other shutdown functions register shutdown functions...)
  // and won't prevent other registered shutdown functions (IE from numerous cron methods) from running by calling exit() before they get a chance.
  register_shutdown_function('mash_return_status');
}

function mash_return_status() {
  exit((mash_get_error()) ? MASH_FRAMEWORK_ERROR : MASH_SUCCESS);
}
