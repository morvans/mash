<?php

/**
 * @file
 *   Core mash commands.
 */

/**
 * Implementation of hook_mash_command().
 *
 * In this hook, you specify which commands your
 * mash module makes available, what it does and
 * description.
 *
 * Notice how this structure closely resembles how
 * you define menu hooks.
 *
 * @return
 *   An associative array describing your command(s).
 */
function core_mash_command() {
  $items = array();

  $items['help'] = array(
    'description' => 'Print this help message. See `mash help help` for more options.',
    'bootstrap' => MASH_BOOTSTRAP_MASH, // No bootstrap.
    'options' => array(
      'sort' => 'Sort commands in alphabetical order. mash waits for full bootstrap before printing any commands when this option is used.',
      'filter' => array(
        'description' => 'Restrict command list to those commands defined in the specified file. Omit value to choose from a list of names.',
        'example-value' => 'category',
        'value' => 'optional',
      ),
      'html' => 'Print help for all commands in HTML format.',
      'pipe' => 'A list of available commands, one per line.',
    ),
    'arguments' => array(
      'command' => 'A command name, or command alias.',
    ),
    'examples' => array(
      'mash' => 'List all commands.',
      'mash --filter=devel_generate' => 'Show only commands defined in devel_generate.mash.inc',
      'mash help pm-download' => 'Show help for one command.',
      'mash help dl' => 'Show help for one command using an alias.',
    ),
    'topics' => array('docs-readme'),
  );
  $items['version'] = array(
    'description' => 'Show mash version.',
    'bootstrap' => MASH_BOOTSTRAP_MASH, // No bootstrap.
    'options' => array(
      'pipe' => 'Print just the version number, and nothing else.',
      'self-update' => 'Check for pending updates to Mash itself. Set to 0 to disable.',
    ),
  );
//  $items['self-update'] = array(
//    'description' => 'Check to see if there is a newer Mash release available.',
//    'bootstrap' => MASH_BOOTSTRAP_MASH, // No bootstrap.
//    'aliases' => array('selfupdate'),
//  );
  $items['core-cron'] = array(
    'description' => 'Run all cron hooks in all active modules for specified site.',
    'aliases' => array('cron'),
  );
//  $items['updatedb'] = array(
//    'description' => 'Apply any database updates required (as with running update.php).',
//    'bootstrap' => MASH_BOOTSTRAP_MAGENTO_SITE,
//    'aliases' => array('updb'),
//  );
//  $items['core-config'] = array(
//    'description' => 'Edit mashrc, site alias, and Magento settings.php files.',
//    'bootstrap' => MASH_BOOTSTRAP_MAX,
//    'arguments' => array(
//      'filter' => 'A substring for filtering the list of files. Omit this argument to choose from loaded files.',
//    ),
//    'options' => array(
//      'bg' => 'Run editor in the background. Does not work with editors such as `vi` that run in the terminal.',
//    ),
//    'examples' => array(
//      'mash core-config' => 'Pick from a list of config/alias/settings files. Open selected in editor.',
//      'mash --bg core-config' => 'Return to shell prompt as soon as the editor window opens.',
//      'mash core-config etc' => 'Edit the global configuration file.',
//      'mash core-config demo.alia' => 'Edit a particular alias file.',
//      'mash core-config sett' => 'Edit settings.php for the current Magento site.',
//      'mash core-config --choice=2' => 'Edit the second file in the choice list.',
//    ),
//    'aliases' => array('conf', 'config'),
//  );
  $items['core-status'] = array(
    'description' => 'Provides a birds-eye view of the current Magento installation, if any.',
    'bootstrap' => MASH_BOOTSTRAP_MAX,
    'aliases' => array('status', 'st'),
    'examples' => array(
      'mash core-status version' => 'Show all status lines that contain version information.',
      'mash core-status --pipe' => 'A list key=value items separated by line breaks.',
      'mash core-status mash-version --pipe' => 'Emit just the mash version with no label.',
    ),
    'arguments' => array(
      'item' => 'Optional.  The status item line(s) to display.',
    ),
    'options' => array(
      'show-passwords' => 'Show database password.',
      'full' => 'Show all mash aliases in the report, even if there are a lot.',
    ),
    'topics' => array('docs-readme'),
  );
//  $items['core-requirements'] = array(
//    'description' => 'Provides information about things that may be wrong in your Magento installation, if any.',
//    'aliases' => array('status-report','rq'),
//    'examples' => array(
//      'mash core-requirements' => 'Show all status lines from the Status Report admin page.',
//      'mash core-requirements --severity=2' => 'Show only the red lines from the Status Report admin page.',
//      'mash core-requirements --pipe' => 'Print out a short report in the format "identifier: severity", where severity 2=error, 1=warning, and 0/-1=OK',
//    ),
//    'options' => array(
//      'severity' => array(
//        'description' => 'Only show status report messages with a severity greater than or equal to the specified value.',
//        'value' => 'required',
//        'example-value' => '3',
//      ),
//      'ignore' => 'Comma-separated list of requirements to remove from output. Run with --pipe to see key values to use.',
//    ),
//  );
  $items['php-eval'] = array(
    'description' => 'Evaluate arbitrary php code after bootstrapping Magento (if available).',
    'examples' => array(
      'mash php-eval "variable_set(\'hello\', \'world\');"' => 'Sets the hello variable using Magento API.',
    ),
    'arguments' => array(
      'code' => 'PHP code',
    ),
    'required-arguments' => TRUE,
    'allow-additional-options' => TRUE,
    'bootstrap' => MASH_BOOTSTRAP_MAX,
    'aliases' => array('eval', 'ev'),
  );
  $items['php-script'] = array(
    'description' => "Run php script(s).",
    'examples' => array(
      'mash php-script scratch' => 'Run scratch.php script. See commands/core directory.',
      'mash php-script example --script-path=/path/to/scripts:/another/path' => 'Run script from specified paths',
      'mash php-script' => 'List all available scripts.',
      '' => '',
      "#!/usr/bin/env mash\n<?php\nvariable_set('key', mash_shift());" => "Execute php code with a full Magento bootstrap directly from a shell script.",
    ),
    'arguments' => array(
      'filename' => 'Optional. The file you wish to execute (without extension). If omitted, list files ending in .php in the current working directory and specified script-path. Some might not be real mash scripts. Beware.',
    ),
    'options' => array(
      'script-path' => array(
        'description' => "Additional paths to search for scripts, separated by : (Unix-based systems) or ; (Windows).",
        'example-value' => '~/scripts',
      ),
    ),
    'allow-additional-options' => TRUE,
    'bootstrap' => MASH_BOOTSTRAP_MAX,
    'aliases' => array('scr'),
    'topics' => array('docs-examplescript', 'docs-scripts'),
  );
//  $items['core-execute'] = array(
//    'description' => 'Execute a shell command. Usually used with a site alias.',
//    'bootstrap' => MASH_BOOTSTRAP_MASH, // No bootstrap.
//    'arguments' => array(
//      'command' => 'The shell command to be executed.',
//    ),
//    'options' => mash_shell_exec_proc_build_options(),
//    'required-arguments' => TRUE,
//    'allow-additional-options' => TRUE,
//    'handle-remote-commands' => TRUE,
//    'strict-option-handling' => TRUE,
//    'examples' => array(
//      'mash core-execute git pull origin rebase' => 'Retrieve latest code from git',
//    ),
//    'aliases' => array('exec', 'execute'),
//    'topics' => array('docs-aliases'),
//  );
//  $items['core-rsync'] = array(
//    'description' => 'Rsync the Magento tree to/from another server using ssh.',
//    'bootstrap' => MASH_BOOTSTRAP_MASH, // No bootstrap.
//    'arguments' => array(
//      'source' => 'May be rsync path or site alias. See rsync documentation and example.aliases.mashrc.php.',
//      'destination' => 'May be rsync path or site alias. See rsync documentation and example.aliases.mashrc.php.',
//    ),
//    'options' => array(
//      'mode' => 'The unary flags to pass to rsync; --mode=rultz implies rsync -rultz.  Default is -akz.',
//      'RSYNC-FLAG' => 'Most rsync flags passed to mash sync will be passed on to rsync.  See rsync documentation.',
//      'exclude-conf' => 'Excludes settings.php from being rsynced.  Default.',
//      'include-conf' => 'Allow settings.php to be rsynced. Default is to exclude settings.php.',
//      'include-vcs' => 'Include special version control directories (e.g. .svn).  Default is to exclude vcs files.',
//      'exclude-files' => 'Exclude the files directory.',
//      'exclude-sites' => 'Exclude all directories in "sites/" except for "sites/all".',
//      'exclude-other-sites' => 'Exclude all directories in "sites/" except for "sites/all" and the site directory for the site being synced.  Note: if the site directory is different between the source and destination, use --exclude-sites followed by "mash rsync @from:%site @to:%site"',
//      'exclude-paths' => 'List of paths to exclude, seperated by : (Unix-based systems) or ; (Windows).',
//      'include-paths' => 'List of paths to include, seperated by : (Unix-based systems) or ; (Windows).',
//    ),
//    'strict-option-handling' => TRUE,
//    'examples' => array(
//      'mash rsync @dev @stage' => 'Rsync Magento root from dev to stage (one of which must be local).',
//      'mash rsync ./ @stage:%files/img' => 'Rsync all files in the current directory to the \'img\' directory in the file storage folder on stage.',
//    ),
//    'aliases' => array('rsync'),
//    'topics' => array('docs-aliases'),
//  );
//  $items['site-install'] = array(
//    'description' => 'Install Magento along with modules/themes/configuration using the specified install profile.',
//    'arguments' => array(
//      'profile' => 'the install profile you wish to run. defaults to \'default\' in D6, \'standard\' in D7+',
//      'key=value...' => 'any additional settings you wish to pass to the profile. Fully supported on D7+, partially supported on D6 (single step configure forms only). The key is in the form [form name].[parameter name] on D7 or just [parameter name] on D6.',
//    ),
//    'options' => array(
//      'db-url' => array(
//        'description' => 'A Magento 6 style database URL. Only required for initial install - not re-install.',
//        'example-value' => 'mysql://root:pass@127.0.0.1/db',
//        ),
//      'db-prefix' => 'An optional table prefix to use for initial install.  Can be a key-value array of tables/prefixes in a mashrc file (not the command line).',
//      'db-su' => array(
//        'description' => 'Account to use when creating a new database. Must have Grant permission (mysql only). Optional.',
//        'example-value' => 'root',
//      ),
//      'db-su-pw' => array(
//        'description' => 'Password for the "db-su" account. Optional.',
//        'example-value' => 'pass',
//      ),
//      'account-name' => 'uid1 name. Defaults to admin',
//      'account-pass' => 'uid1 pass. Defaults to a randomly generated password. If desired, set a fixed password in mashrc.php.',
//      'account-mail' => 'uid1 email. Defaults to admin@example.com',
//      'locale' => array(
//        'description' => 'A short language code. Sets the default site language. Language files must already be present. You may use download command to get them.',
//        'example-value' => 'en-GB',
//      ),
//      'clean-url'=> 'Defaults to 1',
//      'site-name' => 'Defaults to Site-Install',
//      'site-mail' => 'From: for system mailings. Defaults to admin@example.com',
//      'sites-subdir' => array(
//        'description' => "Name of directory under 'sites' which should be created. Only needed when the subdirectory does not already exist. Defaults to 'default'",
//        'value' => 'required',
//        'example-value' => 'directory_name',
//      ),
//    ),
//    'examples' => array(
//      'mash site-install expert --locale=uk' => '(Re)install using the expert install profile. Set default language to Ukranian.',
//      'mash site-install --db-url=mysql://root:pass@localhost:port/dbname' => 'Install using the specified DB params.',
//      'mash site-install --db-url=sqlite://sites/example.com/files/.ht.sqlite' => 'Install using SQLite (D7+ only).',
//      'mash site-install --account-name=joe --account-pass=mom' => 'Re-install with specified uid1 credentials.',
//      'mash site-install standard install_configure_form.site_default_country=FR my_profile_form.my_settings.key=value' => 'Pass additional arguments to the profile (D7 example shown here - for D6, omit the form id).',
//    ),
//    'bootstrap' => MASH_BOOTSTRAP_MAGENTO_ROOT,
//    'aliases' => array('si'),
//  );
  $items['magento-directory'] = array(
    'description' => 'Return path to a given module/theme directory.',
    'arguments' => array(
      'target' => 'A module/theme name, or special names like root, files, private, or an alias : path alias string such as @alias:%files. Defaults to root.',
    ),
    'options' => array(
      'component' => "The portion of the evaluated path to return.  Defaults to 'path'; 'name' returns the site alias of the target.",
      'local' => "Reject any target that specifies a remote site.",
    ),
    'examples' => array(
      'cd `mash dd devel`' => 'Navigate into the devel module directory',
      'cd `mash dd` ' => 'Navigate to the root of your Magento site',
      'cd `mash dd files`' => 'Navigate to the files directory.',
      'mash dd @alias:%files' => 'Print the path to the files directory on the site @alias.',
      'edit `mash dd devel`/devel.module' => "Open devel module in your editor (customize 'edit' for your editor)",
    ),
    'aliases' => array('dd'),
    'bootstrap' => MASH_BOOTSTRAP_MASH,
  );

//  $items['batch-process'] = array(
//    'description' => 'Process operations in the specified batch set',
//    'hidden' => TRUE,
//    'arguments' => array(
//      'batch-id' => 'The batch id that will be processed.',
//    ),
//    'required-arguments' => TRUE,
//    'bootstrap' => MASH_BOOTSTRAP_MAGENTO_LOGIN,
//  );
//
//  $items['updatedb-batch-process'] = array(
//    'description' => 'Perform update functions',
//    'hidden' => TRUE,
//    'arguments' => array(
//      'batch-id' => 'The batch id that will be processed',
//    ),
//    'required-arguments' => TRUE,
//    'bootstrap' => MASH_BOOTSTRAP_MAGENTO_SITE,
//  );
//  $items['core-global-options'] = array(
//    'description' => 'All global options',
//    'hidden' => TRUE,
//    'topic' => TRUE,
//    'bootstrap' => MASH_BOOTSTRAP_MASH,
//  );
//  $items['core-quick-magento'] = array(
//    'description' => 'Download, install, serve and login to Magento with minimal configuration and dependencies.',
//    'bootstrap' => MASH_BOOTSTRAP_MASH,
//    'aliases' => array('qd'),
//    'arguments' => array(
//      'site' => 'Short name for the site to be created - used as a directory name and as sqlite file name. Optional - if omitted timestamped "quick-magento" directory will be used instead.',
//      'projects' => 'A list of projects to download into the new site. If projects contain extensions (modules or themes) with the same name they will be enabled by default. See --enable option to control this behaviour further.',
//    ),
//    'examples' => array(
//      'mash qd' => 'Download and install stable release of Magento into a timestamped directory, start server, and open the site logged in as admin.',
//      'mash qd --profile=minimal --dev --cache --core=magento-8.x --yes' => 'Fire up dev release of Magento site with minimal install profile.',
//      'mash qd testsite devel --server=:8081/admin --browser=firefox --cache --yes' => 'Fire up stable release (using the cache) of Magento site called "testsite", download and enable devel module, start a server on port 8081 and open /admin in firefox.',
//      'mash qd commercesite --core=commerce_kickstart --profile=commerce_kickstart --cache --yes --watchdog' => 'Download and install the "Commerce Kickstart" distribution/install profile, display watchdog messages on the server console.',
//    ),
//  );
  // Add in options/engines.
  //mash_core_quick_magento_options($items);
  return $items;
}

/**
 * Command argument complete callback.
 *
 * @return
 *   Array of available command names.
 */
function core_help_complete() {
  return array('values' => array_keys(mash_get_commands()));
}

/**
 * Command argument complete callback.
 *
 * @return
 *   Array of available profile names.
 */
function core_site_install_complete() {
  $max = mash_bootstrap_max(MASH_BOOTSTRAP_MAGENTO_ROOT);
  if ($max >= MASH_BOOTSTRAP_MAGENTO_ROOT) {
    return array('values' => array_keys(mash_find_profiles(MAGENTO_ROOT)));
  }
}

/**
 * Command argument complete callback.
 *
 * @return
 *  Array of available site aliases.
 */
function core_core_rsync_complete() {
  return array('values' => array_keys(_mash_sitealias_all_list()));
}

/**
 * @defgroup engines Engine types
 * @{
 */

/**
 * Implementation of hook_mash_engine_type_info().
 */
function core_mash_engine_type_info() {
  return array(
    'magento' => array(
    ),
  );
}

function core_mash_engine_magento() {
  $engines = array();
  $engines['xml_format'] = array();
  $engines['module'] = array();
  $engines['block'] = array();
  $engines['route'] = array();
  return $engines;
}

/**
 * @} End of "Engine types".
 */

/**
 * Command handler. Execute update.php code from mash.
 */
function mash_core_updatedb() {
  if (mash_get_context('MASH_SIMULATE')) {
    mash_log(dt('updatedb command does not support --simulate option.'), 'ok');
    return TRUE;
  }

  mash_include_engine('magento', 'update', mash_magento_major_version());
  if (update_main() === FALSE) {
    return FALSE;
  }

  if (mash_magento_major_version() <= 6) {
    // Clear all caches. We just performed major surgery.
    mash_magento_cache_clear_all();
  }
  else {
    // Should be unnecessary on D7.
    // On D7 site-upgrade, this cache_clear was leading to:
    // Call to undefined function field_read_fields() in field_sql_storage.install line 17
  }

  mash_log(dt('Finished performing updates.'), 'ok');
}

/**
 * Implementation of hook_mash_help().
 *
 * This function is called whenever a mash user calls
 * 'mash help <name-of-your-command>'
 *
 * @param
 *   A string with the help section (prepend with 'mash:')
 *
 * @return
 *   A string with the help text for your command.
 */
function core_mash_help($section) {
  switch ($section) {
    case 'meta:core:title':
      return dt("Core mash commands");
    case 'mash:help':
      return dt("Mash provides an extensive help system that describes both mash commands and topics of general interest.  Use `mash help --filter` to present a list of command categories to view, and `mash topic` for a list of topics that go more in-depth on how to use and extend mash.");
    case 'mash:php-script':
      return dt("Runs the given php script(s) after a full Magento bootstrap. A useful alternative to eval command when your php is lengthy or you can't be bothered to figure out bash quoting. If you plan to share a script with others, consider making a full mash command instead, since that's more self-documenting.  Mash provides commandline options to the script via mash_get_option('option-name'), and commandline arguments can be accessed either via mash_get_arguments(), which returns all arguments in an array, or mash_shift(), which removes the next argument from the list and returns it.");
    case 'mash:rsync':
      return dt("Sync the entire magento directory or a subdirectory to a <destination> using ssh. Excludes reserved files and directories for supported VCSs. Useful for pushing copies of your tree to a staging server, or retrieving a files directory from a remote site. Relative paths start from the Magento root directory if a site alias is used; otherwise they start from the current working directory.");
    case 'mash:magento-directory':
      return dt("Return the filesystem path for modules/themes and other key folders.");
    case 'error:MASH_MAGENTO_DB_ERROR':
      $message = dt("Mash was not able to start (bootstrap) the Magento database.\n");
      $message .= dt("Hint: This may occur when Mash is trying to:\n");
      $message .= dt(" * bootstrap a site that has not been installed or does not have a configured database. In this case you can select another site with a working database setup by specifying the URI to use with the --uri parameter on the command line. See `mash topic docs-aliases` for details.\n");
      $message .= dt(" * connect the database through a socket. The socket file may be wrong or the php-cli may have no access to it in a jailed shell. See http://magento.org/node/1428638 for details.\n");
      $message .= dt("\nMash was attempting to connect to: \n!credentials\n", array('!credentials' => _core_site_credentials()));
      return $message;
    case 'error:MASH_MAGENTO_BOOTSTRAP_ERROR':
      $message = dt("Mash was not able to start (bootstrap) Magento.\n");
      $message .= dt("Hint: This error can only occur once the database connection has already been successfully initiated, therefore this error generally points to a site configuration issue, and not a problem connecting to the database.\n");
      $message .= dt("\nMash was attempting to connect to: \n!credentials\n", array('!credentials' => _core_site_credentials()));
      return $message;
      break;
  }
}

// TODO: consolidate with SQL commands?
function _core_site_credentials() {
  $status_table = _core_site_status_table();
  return _core_site_credential_table($status_table);
}

function _core_site_credential_table($status_table) {
  $credentials = '';
  foreach ($status_table as $key => $value) {
    $credentials .= sprintf("  %-18s: %s\n", $key, $value);
  }
  return $credentials;
}

function _core_site_credential_list($status_table) {
  $credentials = '';
  foreach ($status_table as $key => $value) {
    if (isset($value)) {
      $credentials .= sprintf("%s=%s\n", strtolower(str_replace(' ', '_', $key)), $value);
    }
  }
  return $credentials;
}

function _core_path_aliases($project = '') {
  $paths = array();
  if ($magento_root = mash_get_context('MASH_MAGENTO_ROOT')) {
    $paths['%root'] = $magento_root;
    if ($site_root = mash_get_context('MASH_MAGENTO_SITE_ROOT')) {
      $paths['%site'] = $site_root;
      if (is_dir($modules_path = conf_path() . '/modules')) {
        $paths['%modules'] = $modules_path;
      }
      else {
        $paths['%modules'] = 'sites/all/modules';
      }
      if (is_dir($themes_path = conf_path() . '/themes')) {
        $paths['%themes'] = $themes_path;
      }
      else {
        $paths['%themes'] = 'sites/all/themes';
      }
      if (mash_magento_major_version() >= 7) {
        if (mash_get_context('MASH_BOOTSTRAP_PHASE') >= MASH_BOOTSTRAP_MAGENTO_SITE) {
          $paths['%files'] = variable_get('file_public_path', conf_path() . '/files');
          $private_path = variable_get('file_private_path', FALSE);
          if ($private_path !== FALSE) {
            $paths['%private'] = $private_path;
          }
        }
      }
      elseif (function_exists('file_directory_path')) {
        $paths['%files'] = file_directory_path();
      }
      if (function_exists('file_directory_temp')) {
        $paths['%temp'] = file_directory_temp();
      }
      // If the 'project' parameter was specified, then search
      // for a project (or a few) and add its path to the path list
      if (!empty($project)) {
        foreach(explode(',', $project) as $target) {
          $path = mash_core_find_project_path($target);
          if(isset($path)) {
            $paths['%' . $target] = $path;
          }
        }
      }
    }
  }

  // Add in all of the global paths from $options['path-aliases']
  $paths = array_merge($paths, mash_get_option('path-aliases', array()));

  return $paths;
}

function _core_site_status_table($project = '', $full = FALSE) {
  $phase = mash_get_context('MASH_BOOTSTRAP_PHASE');
  if ($magento_root = mash_get_context('MASH_MAGENTO_ROOT')) {
    $status_table['Magento version'] = mash_magento_version();
    if ($site_root = mash_get_context('MASH_MAGENTO_SITE_ROOT')) {
      $status_table['Site URI'] = mash_get_context('MASH_URI');
      if ($creds = mash_get_context('MASH_DB_CREDENTIALS')) {
        $status_table['Database driver'] = $creds['driver'];
        if (!empty($creds['unix_socket'])) {
          $status_table['Database socket'] = $creds['unix_socket'];
        }
        else {
          $status_table['Database hostname'] = $creds['host'];
        }
        $status_table['Database username'] = $creds['user'];
        $status_table['Database name'] = $creds['name'];
        if (mash_get_option('show-passwords', FALSE)) {
          $status_table['Database password'] = $creds['pass'];
        }
        if ($phase > MASH_BOOTSTRAP_MAGENTO_DATABASE) {
          $status_table['Database'] = dt('Connected');
          if ($phase > MASH_BOOTSTRAP_MAGENTO_FULL) {
            $status_table['Magento bootstrap'] = dt('Successful');
            if ($phase == MASH_BOOTSTRAP_MAGENTO_LOGIN) {
              global $user;
              $username =  ($user->uid) ? $user->name : dt('Anonymous');
              $status_table['Magento user'] = $username;
            }
          }
        }
      }
    }
//    $status_table['Default theme'] = mash_theme_get_default();
//    $status_table['Administration theme'] = mash_theme_get_admin();
  }
  if ($php_ini_files = _mash_core_config_php_ini_files()) {
    $status_table['PHP configuration'] = implode(' ', $php_ini_files);
  }
  $status_table['Mash version'] = MASH_VERSION;
  $status_table['Mash configuration'] = implode(' ', mash_get_context_options('context-path', TRUE));
  $alias_files = _mash_sitealias_find_alias_files();
  if (!empty($alias_files)) {
    if ($full || count($alias_files) < 24) {
      $status_table['Mash alias files'] = implode(' ', $alias_files);
    }
    else {
      $status_table['Mash alias files'] = dt("There are !count alias files. Run with --full to see the full list.", array('!count' => count($alias_files)));
    }
  }

  // None of the Status keys are in dt(); this helps with machine-parsing of status?
  $path_names['root'] = 'Magento root';
  $path_names['site'] = 'Site path';
  $path_names['modules'] = 'Modules path';
  $path_names['themes'] = 'Themes path';
  $path_names['files'] = 'File directory path';
  $path_names['private'] = 'Private file directory path';

  $paths = _core_path_aliases($project);
  if (!empty($paths)) {
    foreach ($paths as $target => $one_path) {
      $name = $target;
      if (substr($name,0,1) == '%') {
        $name = substr($name,1);
      }
      if (array_key_exists($name, $path_names)) {
        $name = $path_names[$name];
      }
      $status_table[$name] = $one_path;
    }
  }

  // Store the paths into the '%paths' index; this will be
  // used by other code, but will not be included in the output
  // of the mash status command.
  $status_table['%paths'] = $paths;

  return $status_table;
}

function magento_cron_run(){

  // Only for urls
  // Don't remove this
  $magento_root = mash_get_context('MASH_SELECTED_MAGENTO_ROOT');
  $_SERVER['SCRIPT_NAME'] = $magento_root . '/index.php';
  $_SERVER['SCRIPT_FILENAME'] = $magento_root . '/index.php';

  umask(0);

  try {
      Mage::getConfig()->init()->loadEventObservers('crontab');
      Mage::app()->addEventArea('crontab');
      Mage::dispatchEvent('default');
  } catch (Exception $e) {
      Mage::printException($e);
  }


}

/**
 * Command callback. Runs cron hooks.
 *
 * This is where the action takes place.
 *
 * In this function, all of Magentos API is (usually) available, including
 * any functions you have added in your own modules/themes.
 *
 * To print something to the terminal window, use mash_print().
 *
 */
function mash_core_cron() {
  if (magento_cron_run()) {
    mash_log(dt('Cron run successful.'), 'success');
  }
  else {
    return mash_set_error('MASH_CRON_FAILED', dt('Cron run failed.'));
  }
}

/**
 * Command callback. Edit mashrc and alias files.
 */
function mash_core_config($filter = NULL) {
  $all = mash_core_config_load();

  // Run in the foreground unless --bg is specified.
  $bg = '';
  if (mash_get_option('bg', FALSE)) {
    $bg = '&';
  }
  // Apply any filter that was supplied.
  if ($filter) {
    foreach ($all as $key => $file) {
      if (strpos($file, $filter) === FALSE) {
        unset($all[$key]);
      }
    }
  }
  $all = mash_map_assoc(array_values($all));

  if (count($all) == 1) {
    return mash_shell_exec_interactive('$EDITOR %s ' . $bg, current($all));
  }
  else {
    $choice = mash_choice($all, 'Enter a number to choose which file to edit.', '!key');
    if ($choice !== FALSE) {
      return mash_shell_exec_interactive('$EDITOR %s ' . $bg, $all[$choice]);
    }
  }
}

/**
 * Command argument complete callback.
 *
 * @return
 *   Array of available configuration files for editing.
 */
function core_core_config_complete() {
  return array('values' => mash_core_config_load(FALSE));
}

function mash_core_config_load($headers = TRUE) {
  $php_header = $php = $rcs_header = $rcs = $aliases_header = $aliases = $magento_header = $magento = array();
  $php = _mash_core_config_php_ini_files();
  if (!empty($php)) {
    if ($headers) {
      $php_header = array('phpini' => '-- PHP ini files --');
    }
  }
  mash_sitealias_load_all();
  if ($rcs = mash_get_context_options('context-path', TRUE)) {
    if ($headers) {
      $rcs_header = array('mashrc' => '-- Mashrc --');
    }
  }
  if ($aliases = mash_get_context('mash-alias-files')) {
    if ($headers) {
      $aliases_header = array('aliases' => '-- Aliases --');
    }
  }
  if ($site_root = mash_get_context('MASH_MAGENTO_SITE_ROOT')) {
    $magento = array_merge(array(realpath($site_root . '/settings.php'), realpath(MAGENTO_ROOT . '/.htaccess')));
    if ($headers) {
      $magento_header = array('magento' => '-- Magento --');
    }
  }
  return array_merge($php_header, $php, $rcs_header, $rcs, $aliases_header, $aliases, $magento_header, $magento);
}

function _mash_core_config_php_ini_files() {
  $ini_files = array();
  $ini_files[] = php_ini_loaded_file();
  foreach (array(MASH_BASE_PATH, '/etc/mash', mash_server_home() . '/.mash') as $ini_dir) {
    if (file_exists($ini_dir . "/php.ini")) {
      $ini_files[] = realpath($ini_dir . "/php.ini");
    }
    if (file_exists($ini_dir . "/mash.ini")) {
      $ini_files[] = realpath($ini_dir . "/mash.ini");
    }
  }
  return $ini_files;
}

/**
 * Command callback. Provides information from the 'Status Reports' admin page.
 */
function mash_core_requirements() {
  include_once MASH_MAGENTO_CORE . '/includes/install.inc';
  $severities = array(
    REQUIREMENT_INFO => t('Info'),
    REQUIREMENT_OK => t('OK'),
    REQUIREMENT_WARNING => t('Warning'),
    REQUIREMENT_ERROR => t('Error'),
  );

  magento_load_updates();

  $requirements = module_invoke_all('requirements', 'runtime');
  $ignore_requirements = mash_get_option_list('ignore');
  foreach ($ignore_requirements as $ignore) {
    unset($requirements[$ignore]);
  }
  ksort($requirements);

  $min_severity = mash_get_option('severity', -1);
  $rows[] = array('Title', 'Severity', 'Description');
  foreach($requirements as $key => $info) {
    $severity = array_key_exists('severity', $info) ? $info['severity'] : -1;
    if ($severity >= $min_severity) {
      mash_print_pipe($key . ': ' . $severity . "\n");
      $severity = $severities[$severity];
      $description = array_key_exists('value', $info) ? strip_tags($info['value']) : '';
      if (array_key_exists('description', $info) && !empty($info['description'])) {
        if (!empty($description)) {
          $description .= "\n";
        }
        $description .= strip_tags($info['description']);
      }
      $rows[] = array($info['title'], $severity, $description);
    }
  }
  if (count($rows) > 1) {
    mash_print_table($rows, TRUE, array(1 => 8));
  }
  return $requirements;
}

/**
 * Command callback. Provides a birds-eye view of the current Magento
 * installation.
 */
function mash_core_status() {
  $status_table = _core_site_status_table(mash_get_option('project',''), mash_get_option('full'));
  // If args are specified, filter out any entry that is not named
  // (in other words, only show lines named by one of the arg values)
  $args = func_get_args();
  if (!empty($args)) {
    foreach ($status_table as $key => $value) {
      if (!_mash_core_is_named_in_array($key, $args)) {
        unset($status_table[$key]);
      }
    }
  }
  $return = $status_table;
  unset($status_table['%paths']);
  // Print either an ini-format list or a formatted ASCII table
  if (mash_get_option('pipe')) {
    if (count($status_table) == 1) {
      $first_value = array_shift($status_table);
      mash_print_pipe($first_value);
    }
    else {
      mash_print_pipe(_core_site_credential_list($status_table));
    }
  }
  else {
    unset($status_table['Modules path']);
    unset($status_table['Themes path']);
    mash_print_table(mash_key_value_to_array_table($status_table));
  }
  return $return;
}

// Command callback. Show all global options. Exposed via topic command.
function mash_core_global_options() {
  mash_print(dt('These options are applicable to most mash commands.'));
  mash_print();
  $fake = mash_global_options_command(FALSE);
  $global_option_rows = mash_format_help_section($fake, 'options');
  mash_print_table($global_option_rows);
  mash_print();
  mash_print("See also: `mash topic docs-strict-options`");
}

function _mash_core_is_named_in_array($key, $the_array) {
  $is_named = FALSE;

  $simplified_key = str_replace(array(' ', '_', '-'), array('', '', ''), $key);

  foreach ($the_array as $name) {
    if (stristr($simplified_key, str_replace(array(' ', '_', '-'), array('', '', ''), $name))) {
      $is_named = TRUE;
    }
  }

  return $is_named;
}

/**
 * Callback for core-quick-magento command.
 */
function mash_core_quick_magento() {
  $requests = FALSE;
  $args = func_get_args();
  $name = mash_get_option('use-name');
  mash_set_option('backend', TRUE);
  if (mash_get_option('use-existing', FALSE)) {
    $root = mash_get_option('root', FALSE);
    if (!$root) {
      return mash_set_error('QUICK_MAGENTO_NO_ROOT_SPECIFIED', 'Must specify site with --root when using --use-existing.');
    }
    if (empty($name)) {
      $name = basename($root);
    }
    $base = dirname($root);
  }
  else {
    if (!empty($args) && empty($name)) {
      $name = array_shift($args);
    }
    if (empty($name)) {
      $name = 'quick-magento-' . gmdate('YmdHis', $_SERVER['REQUEST_TIME']);
    }
    $base = getcwd() . '/' . $name;
    mash_set_option('destination', $base);
    $core = mash_get_option('core', 'magento');
    mash_set_option('magento-project-rename', $core);
    if (mash_invoke('pm-download', array($core)) === FALSE) {
      return mash_set_error('QUICK_MAGENTO_CORE_DOWNLOAD_FAIL', 'Magento core download/extract failed.');
    }
    mash_set_option('root', $base . '/' . $core);
  }
  if (!mash_get_option('db-url', FALSE)) {
    mash_set_option('db-url', 'sqlite:' . $base . '/' . $name . '.sqlite');
  }
  if (!mash_bootstrap_to_phase(MASH_BOOTSTRAP_MAGENTO_ROOT)) {
    return mash_set_error('QUICK_MAGENTO_ROOT_LOCATE_FAIL', 'Unable to locate Magento root directory.');
  }
  if (!empty($args)) {
    $requests = pm_parse_arguments($args, FALSE);
  }
  if ($requests) {
    // Unset --destination, so that downloads go to the site directories.
    mash_unset_option('destination');
    if (mash_invoke('pm-download', $requests) === FALSE) {
      return mash_set_error('QUICK_MAGENTO_PROJECT_DOWNLOAD_FAIL', 'Project download/extract failed.');
    }
  }
  mash_invoke('site-install', array(mash_get_option('profile')));
  // Log in with the admin user.
  // TODO: If site-install is given a sites-subdir other than 'default',
  // then it will bootstrap to MASH_BOOTSTRAP_MAGENTO_SITE get the installer
  // to recognize the desired site directory. This somehow interferes
  // with our desire to bootstrap to MASH_BOOTSTRAP_MAGENTO_LOGIN here.
  // We could do the last few steps in a new process iff uri is not 'default'.
  mash_set_option('user', '1');
  if (!mash_bootstrap_to_phase(MASH_BOOTSTRAP_MAGENTO_LOGIN)) {
    return mash_set_error('QUICK_MAGENTO_INSTALL_FAIL', 'Magento core install failed.');
  }
  $enable = pm_parse_arguments(mash_get_option('enable', $requests));
  if (!empty($enable)) {
    if (mash_invoke('pm-enable', $enable) === FALSE) {
     return mash_set_error('QUICK_MAGENTO_PROJECT_ENABLE_FAIL', 'Project enable failed.');
    }
  }
  mash_print(dt('Login URL: ') . mash_invoke('user-login'));
  if ($server = mash_get_option('server', '/')) {
    mash_invoke('runserver', array($server));
  }
}

/**
 * Include options and engines for core-quick-magento command, aggregated from
 * other command options that are available. We prefix option descriptons,
 * to make the long list more navigable.
 *
 * @param $items
 *   The core commandfile command array, by reference. Used to include
 *   site-install options and add options and engines for core-quick-magento.
 */
function mash_core_quick_magento_options(&$items) {
  $options = array(
    'core' => 'Magento core to download. Defaults to "magento" (latest stable version).',
    'use-existing' => 'Use an existing Magento root, specified with --root. Overrides --core.',
    'profile' => 'The install profile to use. Defaults to standard.',
    'enable' => 'Specific extensions (modules or themes) to enable. By default, extensions with the same name as requested projects will be enabled automatically.',
    'server' => 'Host IP address and port number to bind to and path to open in web browser (hyphen to clear a default path), all elements optional. See runserver examples for shorthand.',
    'no-server' => 'Avoid starting runserver (and browser) for the created Magento site.',
    'browser' => 'Optional name of a browser to open site in. If omitted the OS default browser will be used. Set --no-browser to disable.',
    'use-name' => array('hidden' => TRUE, 'description' => 'Overrides "name" argument.'),
  );
  $pm = pm_mash_command();
  foreach ($pm['pm-download']['options'] as $option => $description) {
    if (is_array($description)) {
      $description = $description['description'];
    }
    $options[$option] = 'Download option: ' . $description;
  }
  // Unset a few options that are not usable here, as we control them ourselves
  // or they are otherwise implied by the environment.
  unset($options['destination']);
  unset($options['magento-project-rename']);
  unset($options['default-major']);
  unset($options['use-site-dir']);
  foreach ($items['site-install']['options'] as $option => $description) {
    if (is_array($description)) {
      $description = $description['description'];
    }
    $options[$option] = 'Site install option: ' . $description;
  }
  unset($options['sites-subdir']);
  $runserver = runserver_mash_command();
  foreach ($runserver['runserver']['options'] as $option => $description) {
    $options[$option] = 'Runserver option: ' . $description;
  }
  unset($options['user']);
  $items['core-quick-magento']['options'] = $options;
  $items['core-quick-magento']['engines'] = $pm['pm-download']['engines'];
}

/**
 * Command callback. Runs "naked" php scripts
 * and mash "shebang" scripts ("#!/usr/bin/env mash").
 *
 * @params
 *   Command arguments, optional. First argument is site name, remaining
 *   argument(s) are contrib modules to install.
 */
function mash_core_php_script() {
  $found = FALSE;
  $script = NULL;
  if ($args = func_get_args()) {
    $script = $args[0];
  }

  if ($script == '-') {
    eval(stream_get_contents(STDIN));
  }
  elseif (file_exists($script)) {
    $found = $script;
  }
  else {
    // Array of paths to search for scripts
    $searchpath['DIR'] = dirname(__FILE__);
    $searchpath['cwd'] = mash_cwd();

    // Additional script paths, specified by 'script-path' option
    if ($script_path = mash_get_option('script-path', FALSE)) {
      foreach (explode(PATH_SEPARATOR, $script_path) as $path) {
        $searchpath[] = $path;
      }
    }
    mash_log(dt('Searching for scripts in ') . implode(',', $searchpath), 'debug');

    if (!isset($script)) {
      // List all available scripts.
      $all = array();
      foreach($searchpath as $key => $path) {
        $recurse = !(($key == 'cwd') || ($path == '/'));
        $all = array_merge( $all , array_keys(mash_scan_directory($path, '/\.php$/', array('.', '..', 'CVS'), NULL, $recurse)) );
      }
      mash_print(implode("\n", $all));
    }
    else {
      // Execute the specified script.
      foreach($searchpath as $path) {
        $script_filename = $path . '/' . $script;
        if (file_exists($script_filename . '.php')) {
          $script_filename .= '.php';
        }
        if (file_exists($script_filename)) {
          $found = $script_filename;
          break;
        }
        $all[] = $script_filename;
      }
      if (!$found) {
        return mash_set_error('MASH_TARGET_NOT_FOUND', dt('Unable to find any of the following: @files', array('@files' => implode(', ', $all))));
      }
    }
  }

  if ($found) {
    // Set the MASH_SHIFT_SKIP to two; this will cause
    // mash_shift to skip the next two arguments the next
    // time it is called.  This allows scripts to get all
    // arguments, including the 'php-script' and script
    // pathname, via mash_get_arguments(), or it can process
    // just the arguments that are relevant using mash_shift().
    mash_set_context('MASH_SHIFT_SKIP', 2);
    if (_mash_core_eval_shebang_script($found) === FALSE) {
      include($found);
    }
  }
}

function mash_core_php_eval($command) {
  return eval($command . ';');
}

/*
 * Evaluate a script that begins with #!mash php-script
 */
function _mash_core_eval_shebang_script($script_filename) {
  $found = FALSE;
  $fp = fopen($script_filename, "r");
  if ($fp !== FALSE) {
    $line = fgets($fp);
    if (_mash_is_mash_shebang_line($line)) {
      $first_script_line = '';
      while ($line = fgets($fp)) {
        $line = trim($line);
        if ($line == '<?php') {
          $found = TRUE;
          break;
        }
        elseif (!empty($line)) {
          $first_script_line = $line . "\n";
          break;
        }
      }
      $script = stream_get_contents($fp);
      // Pop off the first two arguments, the
      // command (php-script) and the path to
      // the script to execute, as a service
      // to the script.
      eval($first_script_line . $script);
      $found = TRUE;
    }
    fclose($fp);
  }
  return $found;
}


/**
 * Process sets from the specified batch.
 *
 * This is the default batch processor that will be used if the $command parameter
 * to mash_backend_batch_process() has not been specified.
 */
function mash_core_batch_process($id) {
  mash_batch_command($id);
}

/**
 * Process outstanding updates during updatedb.
 *
 * This is a batch processing command that makes use of the mash_backend_invoke
 * api.
 *
 * This command includes the version specific update engine, which correctly
 * initialises the environment to be able to successfully handle minor and major
 * upgrades.
 */
function mash_core_updatedb_batch_process($id) {
  mash_include_engine('magento', 'update', mash_magento_major_version());
  _update_batch_command($id);
}

/**
 * Given a target (e.g. @site:%modules), return the evaluated directory path.
 *
 * @param $target
 *   The target to evaluate.  Can be @site or /path or @site:path
 *   or @site:%pathalias, etc. (just like rsync parameters)
 * @param $component
 *   The portion of the evaluated path to return.  Possible values:
 *   'path' - the full path to the target (default)
 *   'name' - the name of the site from the path (e.g. @site1)
 *   'user-path' - the part after the ':' (e.g. %modules)
 *   'root' & 'uri' - the Magento root and URI of the site from the path
 *   'path-component' - The ':' and the path
 */
function _mash_core_directory($target = 'root', $component = 'path', $local_only = FALSE) {
  // Normalize to a sitealias in the target.
  $normalized_target = $target;
  if (strpos($target, ':') === FALSE) {
    if (substr($target, 0, 1) != '@') {
      // mash_sitealias_evaluate_path() requires bootstrap to database.
      if (!mash_bootstrap_to_phase(MASH_BOOTSTRAP_MAGENTO_DATABASE)) {
        return mash_set_error('MAGENTO_SITE_NOT_FOUND', dt('You need to specify an alias or run this command within a magento site.'));
      }
      $normalized_target = '@self:';
      if (substr($target, 0, 1) != '%') {
        $normalized_target .= '%';
      }
      $normalized_target .= $target;
    }
  }
  $additional_options = array();
  $values = mash_sitealias_evaluate_path($normalized_target, $additional_options, $local_only);
  if (isset($values[$component])) {
    // Hurray, we found the destination.
    return $values[$component];
  }
}

/**
 * Command callback.
 */
function mash_core_magento_directory($target = 'root') {
  $path = _mash_core_directory($target, mash_get_option('component', 'path'), mash_get_option('local', FALSE));

  // If _mash_core_directory is working right, it will turn
  // %blah into the path to the item referred to by the key 'blah'.
  // If there is no such key, then no replacement is done.  In the
  // case of the dd command, we will consider it an error if
  // any keys are -not- replaced in _mash_core_directory.
  if ($path && (strpos($path, '%') === FALSE)) {
    mash_print($path);
    return $path;
  }
  else {
    return mash_set_error('MASH_TARGET_NOT_FOUND', dt("Target '!target' not found.", array('!target' => $target)));
  }
}

/**
 * Called for `mash version` or `mash --version`
 */
function mash_core_version() {
  mash_print(dt("mash version !version", array('!version' => MASH_VERSION)));
  mash_print_pipe(MASH_VERSION);
  // Next check to see if there is a newer mash.
  if (!mash_get_context('MASH_PIPE') && mash_get_option('self-update', TRUE)) {
    mash_check_self_update();
  }
}

function mash_core_self_update() {
  mash_check_self_update();
}

function mash_core_find_project_path($target) {
  $path = mash_db_result(mash_db_select('system', array('filename'), 'name = :name', array(':name' => $target)));
  if ($path) {
    $path = dirname($path);
    return MAGENTO_ROOT . '/' . $path;
  }
}

/**
 * Command callback. Execute specified shell code. Often used by shell aliases
 * that start with !.
 */
function mash_core_execute() {
  // Get all of the args and options that appear after the command name.
  $args = mash_get_original_cli_args_and_options();
  for ($x = 0; $x < sizeof($args); $x++) {
    // escape all args except for command separators.
    if (!in_array($args[$x], array('&&', '||', ';'))) {
      $args[$x] = mash_escapeshellarg($args[$x]);
    }
  }
  $cmd = implode(' ', $args);
  if ($alias = mash_get_context('MASH_TARGET_SITE_ALIAS')) {
    $site = mash_sitealias_get_record($alias);
    if (!empty($site['remote-host'])) {
      // Remote, so execute an ssh command with a bash fragment at the end.
      $exec = mash_shell_proc_build($site, $cmd, TRUE);
      return mash_shell_proc_open($exec);
    }
  }
  // Must be a local command.
  return mash_shell_proc_open($cmd);
}
